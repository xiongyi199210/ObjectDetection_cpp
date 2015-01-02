//   Record of revisions:
//   Date      Programmer  Discription of change
//   ====      ==========  =====================
// 17/11/2014   Xiong Yi     Original code
// Object detection using exemplar and Hough voting.
// For more details, please refear to: "Li H, Lin Z, Brandt J, et al. Efficient Boosted Exemplar-based Face Detection[J]."
#include "stdafx.h"
#include "iostream"  
#include <opencv2/core/core.hpp>
#include <highgui.hpp>
#include "string.h"
#include "Config.h"
#include "ImgRead.h"
#include "bowVocabulary.h"
#include "DDFile.h"

  
using namespace cv;  
using namespace std;
// for config
const char ConfigFile[]= "config.cfg"; 
Config configSettings(ConfigFile);
// for mouse select
bool select_flag = false;
Rect select;
Point origin;
Size imgsize;

void get_heatMap( Mat &heatMap, vector<Point> &faceCentral ){
	//Mat km = getGaussianKernel
	//filter2D( heatMap, heatMap, heatMap.depth(), km );
	GaussianBlur( heatMap, heatMap, Size(13,13), 1.5, 1.5 );
	// Locate face
	double minV, maxV;
	Point CenTemp;
	minMaxLoc( heatMap, &minV, &maxV, (cv::Point *)0, &CenTemp );
	cout << maxV << ", ";
	cout << CenTemp.x <<  CenTemp.y  << endl;
	faceCentral.push_back( CenTemp );
	return;
}

void onMouse( int event,int x,int y,int,void*)
{
    //Point origin;//不能在这个地方进行定义，因为这是基于消息响应的函数，执行完后origin就释放了，所以达不到效果。
    if(select_flag)
    {
        select.x=MIN(origin.x,x);//不一定要等鼠标弹起才计算矩形框，而应该在鼠标按下开始到弹起这段时间实时计算所选矩形框
        select.y=MIN(origin.y,y);
        select.width=abs(x-origin.x);//算矩形宽度和高度
        select.height=abs(y-origin.y);
		select&=Rect(0,0,imgsize.width,imgsize.height);//保证所选矩形框在视频显示区域之内
    }
    if(event==EVENT_LBUTTONDOWN)
    {
        select_flag=true;//鼠标按下的标志赋真值
        origin=Point(x,y);//保存下来单击是捕捉到的点
        select=Rect(x,y,0,0);//这里一定要初始化，宽和高为(0,0)是因为在opencv中Rect矩形框类内的点是包含左上角那个点的，但是不含右下角那个点
    }
    else if(event==EVENT_LBUTTONUP)
    {
        select_flag=false;
    }
}

void MouseSelect( Mat imgdata, Rect &SelectArea ){
	// This function is used to get a selected area by mouse
	Mat imgTemp;
	imgsize = Size( imgdata.cols, imgdata.rows );
	while(select_flag){
		imgdata.copyTo( imgTemp );
		rectangle(imgTemp,select,Scalar(255,0,0),3,8,0);//能够实时显示在画矩形窗口时的痕迹
		imshow("Select_figure",imgTemp);
		waitKey(30); // highgui dosen't give enough time for imshow to draw a frame on displayer, all we can do is add a waitKey(30)
	}
	SelectArea = select;
} 

int ManualAnnotation( int classID, int method, string file_path, int begin_index, int stopPoint ){
	// This function is used to annotation target object by manual
	// method: 1 for read in by path in file
	// output path
	string output_path = "D:/c_lab/Data_base/face_final/color/";
	// new ddf
	DDFile fs( "E:/GitHub/ObjectDetection_cpp/ObjectDetection/output/facePath2.ddf", "E:/GitHub/ObjectDetection_cpp/ObjectDetection/output/faceIndex2.ddf" );
	if( fs.Init() )
		cout << "Loading succeed." << endl;
	else{
		cout << "Loading failed." << endl;
	}
	//建立窗口
    namedWindow("Select_figure");//显示视频原图像的窗口
	//捕捉鼠标
	setMouseCallback( "Select_figure",onMouse,0);
	// Loop, for each image in database
	int image_i=begin_index-1;
	int image_n = 0; // This for real stop point
	if( method == 1 ){
		// base path
		string basePath = "E:/database/originalPics/";
		string format = ".jpg";
		// open file
		fstream file;
		file.open( file_path, ios::in );
		if(!file){
			cout<< endl << "Error: Bad path to ddf file." << endl;
			return -1;
		}
		// temp
		string line;
		Mat image_org;
		while( getline( file, line ) ){
			string imagepath = basePath + line + format;
			image_n++;
			if( image_n>stopPoint ){ // The last stop point
				bool IsAllFaceMarked = false;
				Mat imgdata = imread( imagepath );
				Mat imgTemp = imgdata.clone();
				Mat imgTemp2 = imgdata.clone();
				while(!IsAllFaceMarked){ // Untill all faces in this image had been marked
					bool continueEn = true;
					// Load in images
					//cout << imagepath << endl;
					Rect SelectArea;
					while(1){
						if( select_flag ){
							MouseSelect( imgdata, SelectArea ); // This function will not return until select_flag become false
							imgTemp = imgTemp2.clone();
							rectangle(imgTemp,SelectArea,Scalar(0,0,255),3,8,0);
						}
						imshow("Select_figure",imgTemp);
						int c;
						c = waitKey(30)&0xFF;
						if(c == 'p'){
							IsAllFaceMarked = true;
							break;
						}
						else if( c=='o' ){ // add a new face
							imgTemp2 = imgTemp.clone();
							break;
						}
						else if( c=='l' ){ // delete one
							continueEn = false;
							break;
						}
						else if( c=='k' ){ // skip this figure
							continueEn = false;
							IsAllFaceMarked = true;
							break;
						}
					}
					if( continueEn ){
						image_i++;
						// write image
						Mat imgOut = imgdata( SelectArea );
						// Trans everything to string
						stringstream ss1;
						ss1<<image_i; 
						string si = ss1.str();
						string ImgName = output_path + si + ".jpg";
						imwrite( ImgName, imgOut );
						// write file
						vector<int> imagesize; imagesize.push_back(imgOut.rows);imagesize.push_back(imgOut.cols);imagesize.push_back(3);
						vector<float> centralPoint; centralPoint.push_back(imgOut.rows/2);centralPoint.push_back(imgOut.cols/2);
						fs.InsertNewLine( classID, ImgName, imagesize , centralPoint ); 
						fs.Save();
						cout << "[" << image_n << ", " << image_i <<  "]; ";
					}
					//else
						//image_i--;
				}
			}
		}
		file.close();
	}
	else{
		cout << "Error: Unknown method while read in!" << endl;
		return 0;
	}
	return 1;
}

int ManualAnnotation( unsigned int stopPoint ){
	// This function is used to annotation target object by manual
	// output path
	string output_path = "D:/c_lab/Data_base/face_final/color/";
	//Size targetSize = Size(80, 80);
	// old ddf
	string ddf_path = "D:/c_lab/ObjectDetection/output/facePath.ddf";
	// new ddf
	DDFile fs( "D:/c_lab/ObjectDetection/output/facePath2.ddf", "D:/c_lab/ObjectDetection/output/faceIndex2.ddf" );
	if( fs.Init() )
		cout << "Loading succeed." << endl;
	else{
		cout << "Loading failed." << endl;
	}
	//建立窗口
    namedWindow("Select_figure");//显示视频原图像的窗口
	//捕捉鼠标
	setMouseCallback( "Select_figure",onMouse,0);
	// Loop, for each image in database
	// open file
	fstream file;
	file.open( ddf_path, ios::in );
	if(!file){
		cout<< endl << "Error: Bad path to ddf file." << endl;
		return -1;
	}
	string line, classID, path, size, cenPoint, imageID;
	unsigned int image_i = 0;
	while (getline(file, line)) {
		image_i++;
        stringstream liness(line);
		getline(liness, classID, ')');
		getline(liness, path, ')');
		getline(liness, size, ')');
		getline(liness, cenPoint, ')');
		getline(liness, imageID, ';');
		if( image_i>stopPoint ){ // The last stop point
			bool continueEn = true;
			// Load in images
			Mat imgdata = imread( path );
			Mat imgTemp = imgdata.clone();
			Rect SelectArea;
			while(1){
				if( select_flag ){
					MouseSelect( imgdata, SelectArea ); // This function will not return until select_flag become false
					imgTemp = imgdata.clone();
					rectangle(imgTemp,SelectArea,Scalar(0,0,255),3,8,0);
				}
				imshow("Select_figure",imgTemp);
				int c;
				c = waitKey(30)&0xFF;
				if(c == 'p')
					break;
				else if( c=='l' ){ // delete one
					continueEn = false;
					break;
				}
			}
			if( continueEn ){
				// write image
				Mat imgOut = imgdata( SelectArea );
				// Trans everything to string
				stringstream ss1;
				ss1<<image_i; 
				string si = ss1.str();
				string ImgName = output_path + si + ".jpg";
				imwrite( ImgName, imgOut );
				// write file
				vector<int> imagesize; imagesize.push_back(imgOut.rows);imagesize.push_back(imgOut.cols);imagesize.push_back(3);
				vector<float> centralPoint; centralPoint.push_back(imgOut.rows/2);centralPoint.push_back(imgOut.cols/2);
				fs.InsertNewLine( atoi(classID.c_str()), ImgName, imagesize , centralPoint ); 
				fs.Save();
				cout << image_i << ", ";
			}
			else
				image_i--;
		}
    }
	file.close();
	return 1;
}

int DDFilePrepare( ){
	// This function is used to add sample images into database file
	DDFile fs( "D:/c_lab/ObjectDetection/output/facePath.ddf", "D:/c_lab/ObjectDetection/output/faceIndex.ddf" );
	if( fs.Init() )
		cout << "Loading succeed." << endl;
	else{
		cout << "Loading failed." << endl;
		//return 0;
	}
	// class ID: 1. colorful front face, from VOC2007
	if( fs.InsertNewClass( 1, "D:/cloud_work/Baidu_cloud/data/Object/101_ObjectCategories/Faces_easy/", 435, "image_", 4 ) )
		cout << "VOC2007 face easy data is now in database!" << endl;
	// class ID: 2. colorful rotated face, from one person
	fs.InsertNewClass( 2, "D:/c_lab/Data_base/face_rotated_1/1/", 53 );
	// class ID: 3. colorful rotated face, need fixed, from 11 person
	fs.InsertNewClass( 3, "D:/c_lab/Data_base/face_rotated_2/color/2/2/", 949 );
	// class ID: 4. 

	cout << "All Loading finished." << endl;
	// Save index file
	if( fs.Save() )
		cout << "Saving succeed." << endl;
	else{
		cout << "Saving failed." << endl;
		return 0;
	}

	return 1;
}

int DDFileTest( ){
	DDFile fs( "D:/c_lab/ObjectDetection/output/path.ddf", "D:/c_lab/ObjectDetection/output/index.ddf" );
	if( fs.Init() )
		cout << "Loading succeed." << endl;
	else{
		cout << "Loading failed." << endl;
		//return 0;
	}
	// insert a line
	vector<int> imagesize; imagesize.push_back(80);imagesize.push_back(80);imagesize.push_back(3);
	vector<float> centralPoint; centralPoint.push_back(40.0);centralPoint.push_back(40.0);
	fs.InsertNewLine( 2, "D:/c_lab/Data_base/face_rotated_1/1/1.jpg", imagesize, centralPoint ); 
	cout << "First line has been inserted." << endl;
	// insert another line
	imagesize.clear(); centralPoint.clear();
	imagesize.push_back(70);imagesize.push_back(92);imagesize.push_back(3);
	centralPoint.push_back(35.0);centralPoint.push_back(46.0);
	fs.InsertNewLine( 3, "D:/c_lab/Data_base/face_rotated_1/1/2.jpg", imagesize, centralPoint ); 
	cout << "Second line has been inserted." << endl;

	// insert a new class
	fs.InsertNewClass( 1, "D:/c_lab/Data_base/face_rotated_1/1/", 53 );
	cout << "a whole class has been inserted." << endl;

	// Save index file
	if( fs.Save() )
		cout << "Saving succeed." << endl;
	else{
		cout << "Saving failed." << endl;
		return 0;
	}
	return 1;
}

int match_a_new_image( Mat &imgData, Mat &VotingMap, BowVocabulary &bowVocab, BowVocParams parms ){
	CV_Assert( imgData.size().height!=0 );
	if( bowVocab.IsMatcherTrained() ){
		BowMatchResult result;
		cout << "Finding discriptors..." << endl;
		bowVocab.quantizing( imgData, result );
		cout << "Finished! " << result.keyID.size() << " KeyPoints finded." << endl;
		VotingScore Score;
		cout << "Caculating scores" << endl;
		Score.getScore( result, bowVocab );
		Score.saveScore( parms, result );
		Score.drawVoting( VotingMap, result );
	}

	
	return 1;
}

int match_a_set(  BowVocabulary &bowVocab, BowVocParams parms ){
	vector<string> path;
	path.push_back("D:/cloud_work/Baidu_cloud/data/Object/101_ObjectCategories/Faces/");
	Size2i target_size;
	target_size.width = 120;
	target_size.height= 120;
	Imgread imgread( path, target_size );
	vector<Mat> imgData;
	string format = ".jpg";
	imgread.BeginRead( imgData, "image_", format, 1, 312, 4 );
	int n=1;
	string imName = parms.OutPath + "VotingMap/";
	for( int i=0; i<imgData.size(); i++ ){
		Mat imgdata = imgData[i];
		Mat VotingMap; imgdata.copyTo( VotingMap ); 
		//Mat VotingHeatMap = Mat::zeros( target_img.rows, target_img.cols, CV_32FC1);
		//Mat BoxMap; target_img.copyTo( BoxMap );
		VotingMap = 0.3 * VotingMap; // For a clearly shown
		//if( !match_a_new_image( target_img, VotingMap, VotingHeatMap, bowVocab, params ) )
		if( !match_a_new_image( imgdata, VotingMap, bowVocab, parms ) )
			cout << "The Matcher need be trained first!" << endl;
		resize( VotingMap, VotingMap, Size( 2*VotingMap.cols, 2*VotingMap.rows ) );
		// Trans everything to string
		stringstream ss;
		ss<<n; n++; 
		string si = ss.str();
		string ImgName = imName + si + format;
		cout << ImgName << endl;
		if( !imwrite( ImgName, VotingMap ) )
			cout << "Wrong with saving!" << endl;
	}
	return 1;
}

int match_a_set( string path, string basePath, string format, BowVocabulary &bowVocab, BowVocParams parms ){
	// open ddr file
	fstream file;
	file.open( path, ios::in );
	if(!file){
		cout<< endl << "Error: Bad path to ddf file." << endl;
		return -1;
	}
	// temp
	string line;
	Mat image_org;
	int n=1;
	string imName = parms.OutPath + "VotingMap/";
	while( getline( file, line ) ){
		string imagepath = basePath + line + format;
		Mat imgdata = imread( imagepath );
		Mat VotingMap; imgdata.copyTo( VotingMap ); 
		//Mat VotingHeatMap = Mat::zeros( target_img.rows, target_img.cols, CV_32FC1);
		//Mat BoxMap; target_img.copyTo( BoxMap );
		VotingMap = 0.3 * VotingMap; // For a clearly shown
		//if( !match_a_new_image( target_img, VotingMap, VotingHeatMap, bowVocab, params ) )
		if( !match_a_new_image( imgdata, VotingMap, bowVocab, parms ) )
			cout << "The Matcher need be trained first!" << endl;
		resize( VotingMap, VotingMap, Size( 2*VotingMap.cols, 2*VotingMap.rows ) );
		// Trans everything to string
		stringstream ss;
		ss<<n; n++; 
		string si = ss.str();
		string ImgName = imName + si + format;
		cout << ImgName << endl;
		if( !imwrite( ImgName, VotingMap ) )
			cout << "Wrong with saving!" << endl;
	}
	return 1;
}

int match_a_new_image( Mat &imgData, Mat &VotingMap, Mat &HeatMap, BowVocabulary &bowVocab, BowVocParams parms ){
	// This function is used to match a new input image with the vocabulary
	// Just for shown
	//cout << imgData.size() << endl;
	CV_Assert( imgData.size().height!=0 );
	Size2i targetSize = parms.targetSize;
	// Open log file
	ofstream f_log(  parms.OutPath+"Matching.log" );
	if( !f_log.is_open() )
		cout << "Warning: Can't open log file" << endl;
	if( bowVocab.IsMatcherTrained() ){
		BowMatchResult result;
		f_log << "Finding discriptors..." << endl;
		bowVocab.quantizing( imgData, result );
		f_log << "Finished! " << result.keyID.size() << " KeyPoints finded." << endl;
		for( int k=0; k<result.keyID.size(); k++ ){
			circle(imgData,result.queryMatched[k],5,Scalar(255,0,0));
		}
		// for each matched keypoints
		f_log << "Matching begin..." << endl;
		vector<float> Score;
		for( int keyI=0; keyI<result.keyID.size(); keyI++ ){
			// summrize the main direct
			Point2f V_t = Point2f(0.0, 0.0);
			int n=0;

			int keyID = result.keyID[keyI];
			f_log << "Matched ID: " << keyI << ", " << "Matched keyID: " << keyID << "-------------" << endl;
			// get team frequency in target
			float tf_R = 0.0f;
			for( int k=0; k<result.keyID.size(); k++ ){
				if( keyID==result.keyID[k] ){
					tf_R++;
				}
			}
			f_log << "tf_R = " << tf_R << endl;

			Point2f keyPointInTarget = result.queryMatched[keyI];
			// Get the inverted file keyID point to
			TeamFrequency TF;
			keyPositions Pos;
			if( bowVocab.getTFByIndex( keyID, TF ) + bowVocab.getPositionByIndex( keyID, Pos ) == 2 ){

				// get idf
				float idf = (float)TF.imageID.size();
				idf = log( bowVocab.getNumOfImages()/idf );
				f_log << "idf = " << idf << endl;

				// get team frequency in vocabulary
				float tf_e = 0.0f;
				for( int k=0; k<TF.TF.size(); k++ ){
					tf_e += TF.TF[k];
				}
				f_log << "tf_e = " << tf_e << endl;

				//cout << TF.imageID.size() << endl;
				int posI = 0;
				for( int imageI=0; imageI<TF.imageID.size(); imageI++ ){  // For each image
					for( int posIt=0; posIt<TF.TF[imageI]; posI++, posIt++ ){  // For each keypoint in image[imageI]
						//float x_transform = 1.39548*(targetSize.width/2 - Pos.x_y[posI].x); // x = B + ( C - A ), A is the point in exemplar, C is the center, B is the keyPoint in target image
						//float y_transform = 1.39548*(targetSize.height/2 - Pos.x_y[posI].y);
						//float x_transform = 1.39548*(Pos.x_y[posI].x); // x = B + ( C - A ), A is the point in exemplar, C is the center, B is the keyPoint in target image
						//float y_transform = 1.39548*(Pos.x_y[posI].y);
						float x_transform = (Pos.x_y[posI].x); // x = B + ( C - A ), A is the point in exemplar, C is the center, B is the keyPoint in target image
						float y_transform = (Pos.x_y[posI].y);
						//float x_transform = (targetSize.width/2 - Pos.x_y[posI].x);
						//float y_transform = (targetSize.height/2 - Pos.x_y[posI].y);

						// score normal
						float noram_trans = sqrtf( x_transform*x_transform + y_transform*y_transform );
						V_t.x += x_transform/noram_trans;
						V_t.y += y_transform/noram_trans;
						n++;

						Point2f VotingPosition = Point2f( keyPointInTarget.x + x_transform, keyPointInTarget.y + y_transform );
						//cout << VotingPosition << endl;
						// Check if Voting Position is out of range
						if( VotingPosition .x<0 || VotingPosition .x>=imgData.cols || VotingPosition .y<0 || VotingPosition .y>=imgData.rows )
							continue;
						// draw voting position
						circle(VotingMap,keyPointInTarget,5,Scalar(255,0,0));
						line( VotingMap, keyPointInTarget, VotingPosition, Scalar(255,150,255) );
						circle(VotingMap,VotingPosition,5,Scalar(0,0,255));
						// caculate voting value
						float weight = idf * idf/( tf_R * tf_e );
						HeatMap.at<float>( VotingPosition.y, VotingPosition.x ) += weight;
						f_log <<"weight = " <<  weight << endl;
					}
				}
			}
			// caculate score
			float S;
			S = ( V_t.x * V_t.x + V_t.y * V_t.y ) / n - 1;
			Score.push_back(S);
			f_log << "Score of this point: " << S << endl;
			// text
			// Trans everything to string
			stringstream ss;
			ss<<keyI; 
			string si = ss.str();
			putText( VotingMap, si, keyPointInTarget, FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 255) );
		}
		f_log << "Matching End" << endl;
		f_log.close();
	}
	else
		return 0;
	return 1;
}

int get_invertedFile( vector<Mat> &imgData, BowVocabulary &bowVocab, BowVocParams &params ){
	// This function is used to get inverted File, either by loading or recaculating.
	// inverted File is used for quickly searching
	if( !bowVocab.loadInvFile( params.InvertedFilePath ) ){
		cout << "The inverted file didn't exist, try to get a new one." << endl;
		if( bowVocab.seqFile2invFile( ) ){
			// rewrite Vocabulary
			bowVocab.saveVocabulary( params.SavePath );
			if( !bowVocab.saveInvertedFile( params.InvertedFilePath ) ){
				cout << "Saving failed!! Can't save the inverted file." << endl;
			}
			else{
				cout << "Successful Saving!" << endl;
			}
		}
		else{
			cout << "Can't generate inverted file, pleas offer another set" << endl;
			return 0;
		}
	}
	else
		cout << "Loading Inverted File Successfully!" << endl;
	
	return 1;
}

int get_sequentialFile( vector<Mat> &imgData, BowVocabulary &bowVocab, BowVocParams &params ){
	// This function is used to get sequential File, either by loading or recaculating.
	// Sequential File is used for exemplar selection or generate the inverted file
	if( !bowVocab.quantizingExemplars( params.SequentialFilePath ) ){
		cout << "The sequential file didn't exist, try to get a new one." << endl;
		if( bowVocab.quantizingExemplars( imgData ) ){
			if( !bowVocab.saveSequentialFile( params.SequentialFilePath ) ){
				cout << "Saving failed!! Can't save the sequential file." << endl;
			}
			else{
				cout << "Successful Saving!" << endl;
			}
		}
		else{
			cout << "Can't generate sequential file, pleas offer another set" << endl;
			return 0;
		}
	}
	else
		cout << "Loading Sequential File Successfully!" << endl;
	
	return 1;
}

int get_vocabulary( vector<Mat> &imgData, BowVocabulary &bowVocab, BowVocParams &params ){
	// This function is used to get vocabulary, either 
	// loading in or caculate trough a new image set
	// Loading the paraments from config file
	// try to loading the vocabulary
	if( !bowVocab.generateVocabulary( params.SavePath ) ){
		cout << "The vocabulary didn't exist, try to get a new one." << endl;
		if( imgData.size()<=2 ){
			cout << "Can't generate vocabulary, pleas offer another set" << endl;
			return -1;
		}
		if( bowVocab.generateVocabulary( imgData, params ) ){
			if( !bowVocab.saveVocabulary( params.SavePath ) ){
				cout << "Saving failed!! Can't save the vocabulary data." << endl;
			}
			else{
				cout << "Successful Saving!" << endl;
			}
		}
		else{
			cout << "Can't generate vocabulary, pleas offer another set" << endl;
			return 0;
		}
	}
	else
		cout << "Loading Vocabulary Successfully!" << endl;
	
	return 1;
}

int read_and_prepare( unsigned int ID,  vector<Mat> &imgData, vector<Point2f> &centralPoint, BowVocParams &params ){
	// read in the data and prepare everything for classify
	vector<string> ImgPath;
	Size2i target_size;
	string temp;
	// Trans everything to string
	stringstream ss;
	ss<<ID; 
    string sID = ss.str();
	// read from cinfig file
	string pathName = "PS"+sID+"_path"; // positive set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "NS"+sID+"_path"; // negative set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "Gt"+sID+"_path"; // ground truth
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	target_size.height = configSettings.Read("sample_sizeM", target_size.height); // target size
	target_size.width = configSettings.Read("sample_sizeN", target_size.width);
	params.targetSize = target_size;
	// read in Bow Params
	params.vocabSize = configSettings.Read("vocabSize", params.vocabSize);
	params.memoryUse = configSettings.Read("memoryUse", params.memoryUse);
	params.descProportion = configSettings.Read("descProportion", params.descProportion);
	string OutPath1, OutPath2;
	OutPath1 = configSettings.Read("OutPath", OutPath1);
	OutPath2 = configSettings.Read("BOWSavePath", OutPath2);
	params.SavePath = OutPath1 + OutPath2;
	params.OutPath = OutPath1;
	string OutPath3;
	OutPath3 = configSettings.Read("SequentialFilePath", OutPath3);
	params.SequentialFilePath = OutPath1 + OutPath3;
	string OutPath4;
	OutPath4 = configSettings.Read("InvertedFilePath", OutPath4);
	params.InvertedFilePath = OutPath1 + OutPath4;

	Imgread imgread( ImgPath, target_size );
	//imgread.BeginRead( imgData, "image_", ".jpg", 1, 435, 4 );
	if(ID==2)
		imgread.BeginRead( imgData, centralPoint );
	else if( ID==3 ){
		vector<FDDBMark> faceMark;
		imgread.BeginRead( imgData, faceMark, "E:/database/originalPics/", ".jpg" );
	}
	return 1;
}

int read_and_prepare( unsigned int ID,  vector<Mat> &imgData, BowVocParams &params ){
	// read in the data and prepare everything for classify
	vector<string> ImgPath;
	Size2i target_size;
	string temp;
	// Trans everything to string
	stringstream ss;
	ss<<ID; 
    string sID = ss.str();
	// read from cinfig file
	string pathName = "PS"+sID+"_path"; // positive set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "NS"+sID+"_path"; // negative set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "Gt"+sID+"_path"; // ground truth
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	target_size.height = configSettings.Read("sample_sizeM", target_size.height); // target size
	target_size.width = configSettings.Read("sample_sizeN", target_size.width);
	params.targetSize = target_size;
	// read in Bow Params
	params.vocabSize = configSettings.Read("vocabSize", params.vocabSize);
	params.memoryUse = configSettings.Read("memoryUse", params.memoryUse);
	params.descProportion = configSettings.Read("descProportion", params.descProportion);
	string OutPath1, OutPath2;
	OutPath1 = configSettings.Read("OutPath", OutPath1);
	OutPath2 = configSettings.Read("BOWSavePath", OutPath2);
	params.SavePath = OutPath1 + OutPath2;
	params.OutPath = OutPath1;
	string OutPath3;
	OutPath3 = configSettings.Read("SequentialFilePath", OutPath3);
	params.SequentialFilePath = OutPath1 + OutPath3;
	string OutPath4;
	OutPath4 = configSettings.Read("InvertedFilePath", OutPath4);
	params.InvertedFilePath = OutPath1 + OutPath4;

	Imgread imgread( ImgPath, target_size );
	imgread.BeginRead( imgData, "image_", ".jpg", 1, 435, 4 );
	return 1;
}

int read_and_prepare( unsigned int ID, BowVocParams &params ){
	// read in the data and prepare everything for classify
	vector<string> ImgPath;
	Size2i target_size;
	string temp;
	// Trans everything to string
	stringstream ss;
	ss<<ID; 
    string sID = ss.str();
	// read from cinfig file
	string pathName = "PS"+sID+"_path"; // positive set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "NS"+sID+"_path"; // negative set
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	pathName = "Gt"+sID+"_path"; // ground truth
	temp = configSettings.Read(pathName, temp);
	ImgPath.push_back( temp );
	target_size.height = configSettings.Read("sample_sizeM", target_size.height); // target size
	target_size.width = configSettings.Read("sample_sizeN", target_size.width);
	params.targetSize = target_size;
	// read in Bow Params
	params.vocabSize = configSettings.Read("vocabSize", params.vocabSize);
	params.memoryUse = configSettings.Read("memoryUse", params.memoryUse);
	params.descProportion = configSettings.Read("descProportion", params.descProportion);
	string OutPath1, OutPath2;
	OutPath1 = configSettings.Read("OutPath", OutPath1);
	OutPath2 = configSettings.Read("BOWSavePath", OutPath2);
	params.SavePath = OutPath1 + OutPath2;
	params.OutPath = OutPath1;
	string OutPath3;
	OutPath3 = configSettings.Read("SequentialFilePath", OutPath3);
	params.SequentialFilePath = OutPath1 + OutPath3;
	string OutPath4;
	OutPath4 = configSettings.Read("InvertedFilePath", OutPath4);
	params.InvertedFilePath = OutPath1 + OutPath4;
	return 1;
}

void RoundShow( vector<Mat> &imgData, int show_begin_num, Size2i show_size  ){
	// This function is used to show some small samples at the same time
	Size target_size = imgData[0].size();
	Mat all(show_size.height*target_size.height,show_size.width*target_size.width, imgData[0].type() );
	//cout << imgData.size() << endl;
	int size_of_imgData = (int)imgData.size();
	int endI = min( size_of_imgData-1, show_begin_num + show_size.height*show_size.width-1 ); 
	for( int i=show_begin_num; i<=endI; i++ ){
		int i_t = i - show_begin_num;
		int x = i_t%show_size.width;
		int y = (int)floor(i_t/show_size.width);
		Range rowrange( y*target_size.height, (y+1)*target_size.height );
		Range colrange( x*target_size.width, (x+1)*target_size.width );
		Mat all_rect = Mat(all, rowrange, colrange);
		imgData[i].copyTo(all_rect);
	}
	imshow( "target", all );
	return;
}

void RoundShow( vector<Mat> &imgData, int show_begin_num, Size2i show_size, BowVocabulary &bowVocab  ){
	// This function is used to show some small samples at the same time
	Size target_size = imgData[0].size();
	Mat all(show_size.height*target_size.height,show_size.width*target_size.width, imgData[0].type() );
	//cout << imgData.size() << endl;
	int size_of_imgData = imgData.size();
	int endI = min( size_of_imgData-1, show_begin_num + show_size.height*show_size.width-1 ); 
	int i_match = 0, seq_i = 0;
	for( int i=show_begin_num; i<=endI; i++ ){
		int i_t = i - show_begin_num;
		int x = i_t%show_size.width;
		int y = floor(i_t/show_size.width);
		Range rowrange( y*target_size.height, (y+1)*target_size.height );
		Range colrange( x*target_size.width, (x+1)*target_size.width );
		Mat all_rect = Mat(all, rowrange, colrange);
		imgData[i].copyTo(all_rect);

		// draw keypoints
		int index = bowVocab.getIndexByImageID( i );
		if( index>=0 ){
			vector<Point2f> centers;
			bowVocab.getExemplarKeyPoints(index, centers);
			for( int k=0; k<centers.size(); k++ ){
				circle(all_rect,centers[k],5,Scalar(255,0,0));
			}
		}
	}
	imshow( "target", all );
	return;
}


int main()  
{   
	bool needAnnotatio = false;
	bool needShowExamplars = false;
	if( needAnnotatio ){
		// Annotation
		int stopPoint = 39; // normally, it equals to " num of image in output path - num of image deleted "
		//ManualAnnotation( stopPoint );
		ManualAnnotation( 3, 1, "E:/database/FDDB/FDDB-folds/FDDB-fold-01.txt", 545, stopPoint );
		while(1);
	}
	/*Rect SelectArea = Rect(0,0,0,0);
	// Loading figure
	Mat imgdata = imread("D:/c_lab/Data_base/MaoShu/1.jpg");
	//cout << imgsize << endl;
	//建立窗口
    namedWindow("Select_figure");//显示视频原图像的窗口
	//捕捉鼠标
	setMouseCallback( "Select_figure",onMouse,0);
	//cout << "step1" << endl;
	while(1){
		if( select_flag ){
			MouseSelect( imgdata, SelectArea ); // This function will not return until select_flag become false
			rectangle(imgdata,SelectArea,Scalar(0,0,255),3,8,0);
		}
		imshow("Select_figure",imgdata);
		waitKey(30);
	}*/
	//DDFileTest( );
	//DDFilePrepare( );
	//while(1);

	// param
	int show_start = 0;int show_start_t = 0;
	Size2i show_size = Size2i(11, 6);
	BowVocParams params;
	// read image
	vector<Mat> imgData;
	vector<Point2f> centralPoint;
	if( needShowExamplars )
		read_and_prepare( 2, imgData, centralPoint, params );
	else
		read_and_prepare( 2, params );
	//read_and_prepare( 1, imgData, params );
	// build or load Vocabulary
	BowVocabulary bowVocab;
	bowVocab.setting( params.targetSize , Point2f( params.targetSize.width/2, params.targetSize.height/2 ) );
	vector<Mat> imgData2 = imgData;
	get_vocabulary( imgData2, bowVocab, params );

	// quantizing
	get_sequentialFile( imgData, bowVocab, params );
	// inverted
	get_invertedFile( imgData, bowVocab, params );
	// Check if load corecctly
	CV_Assert( bowVocab.IsLoadCorecct() );
	// test in target image
	bowVocab.trainFlaan();
	//Mat target_img = imread( "D:/cloud_work/Baidu_cloud/data/Object/101_ObjectCategories/Faces/image_0030.jpg" );
	/*Mat target_img = imread("E:/database/originalPics/2002/08/04/big/img_769.jpg");
	//Mat target_img = imread( "E:/database/originalPics/2003/05/03/big/img_559.jpg" );
	//cout << target_img.size() << endl;
	Mat VotingMap; target_img.copyTo( VotingMap ); 
	//Mat VotingHeatMap = Mat::zeros( target_img.rows, target_img.cols, CV_32FC1);
	//Mat BoxMap; target_img.copyTo( BoxMap );
	VotingMap = 0.3 * VotingMap; // For a clearly shown
	//if( !match_a_new_image( target_img, VotingMap, VotingHeatMap, bowVocab, params ) )
	if( !match_a_new_image( target_img, VotingMap, bowVocab, params ) )
		cout << "The Matcher need be trained first!" << endl;
	imshow( "targetImage", target_img );
	resize( VotingMap, VotingMap, Size( 2*VotingMap.cols, 2*VotingMap.rows ) );
	imshow( "VotingMap", VotingMap );*/
	match_a_set( "E:/database/FDDB/FDDB-folds/FDDB-fold-02.txt", "E:/database/originalPics/", ".jpg", bowVocab, params );
	//match_a_set(  bowVocab, params );
	if( needShowExamplars )
		RoundShow( imgData, show_start, show_size, bowVocab );
	//RoundShow( imgData, show_start, show_size);
	while(1){
		// show
		if( show_start_t!=show_start ){
			if( needShowExamplars )
				RoundShow( imgData, show_start, show_size, bowVocab );
			//RoundShow( imgData, show_start, show_size);
			show_start_t = show_start;
		}
		//USER INPUT:
		int c;
		c = waitKey(10)&0xFF;
		if(c == 27)
			  break;
		switch (c)
		{
		case 'q': show_start = max( 0, show_start-show_size.height*show_size.width );break;
		case 'e': show_start = min( (int)imgData.size(), show_start+show_size.height*show_size.width );break;
		default:
			break;
		}
	}
} 