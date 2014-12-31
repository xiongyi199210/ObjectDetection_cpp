// bowVocabulary.cpp
#include "stdafx.h"
#include "bowVocabulary.h"
#

#define DEBUG_DESC_PROGRESS

#define CE_VOTING_ORG_SCORE 4
#define CE_CENTERAL_POS_x 60
#define CE_CENTERAL_POS_y 60

using namespace cv;
using namespace std;


// Face Center
void ClustingCenter::get_max_center( float scale, vector<int> &index ){
	max_num=0;
	for( int i=0; i<num.size(); i++ ){
		if( num[i]>max_num )
			max_num = num[i];
	}
	int T_num = int( scale * float(max_num) );
	for( int i=0; i<num.size(); i++ ){
		if( num[i]>T_num )
			index.push_back(i);
	}
	return;
}

int ClustingCenter::check_and_add( Point2f point, int n ){
	if( center.size()==0 ){
		center.push_back( point );
		num.push_back(n);
	}
	else{
		int i;
		for( i=0; i<center.size(); i++ ){
			Point2f distance = point - center[i];
			if( distance.x*distance.x + distance.y*distance.y <= T_distance ){
				int totalnum = num[i] + n;
				float scale = float(num[i])/float(totalnum); 
				//cout << scale << endl;
				//cout << center[i] << ", " << point << endl;
				center[i] = center[i] + distance * (1-scale); // shift center point
				//cout << center[i] << ", " << point << endl;
				num[i] = totalnum;
				break;
			}
		}
		if( i==center.size() ){ // if no one matched
			center.push_back( point );
			num.push_back(n);
		}
	}
	
	return 1;
}

// Score
void VotingScore::drawVoting( Mat &VotingMap, BowMatchResult result ){
	Rect imgBox( 0, 0, VotingMap.cols, VotingMap.rows );
	int vot_i=0;
	float T = -10;//0.5*maxS;
	for( int keyI=0; keyI<result.keyID.size(); keyI++ ){
		if( S[keyI] > T ){
			Point2f key_point = result.queryMatched[keyI];
			stringstream ss;
			ss<<keyI; 
			string si = ss.str();
			putText( VotingMap, si, key_point, FONT_HERSHEY_COMPLEX, 0.5, Scalar(0, 255, 255) );
			circle(VotingMap,key_point,5,Scalar(255,0,0));
			for( int i=0; i<n[keyI]; i++ ){
				Point2f v_p = VotingPoints[vot_i] + key_point;
				if( v_p.inside( imgBox ) ){
					line( VotingMap, key_point, v_p, Scalar(255,150,255) );
					circle(VotingMap,v_p,1,Scalar(0,0,255));
				}
				vot_i++;
			}
		}
		else{
			vot_i += n[keyI];
		}
	}
	// centre points
	for( int i=0; i<CentrePoints.size(); i++ ){
		circle(VotingMap,CentrePoints[i],7,Scalar(0,255,0));
	}
	// face points
	vector<int> index;
	Centre.get_max_center( 0.8, index );
	for( int i=0; i<index.size(); i++ ){
		circle(VotingMap,CentrePoints[index[i]],40,Scalar(255,0,139), 5);
	}
	return;
}

int VotingScore::saveScore( BowVocParams parms, BowMatchResult result ){
	cout << "Saving Score" << endl;
	// Open log file
	ofstream f_log(  parms.OutPath+"Matching.log" );
	if( !f_log.is_open() ){
		cout << "Warning: Can't open log file" << endl;
		return 0;
	}
	int vot_i=0;
	for( int keyI=0; keyI<n.size(); keyI++ ){
		f_log << "Matched ID: " << keyI << "-------------" << endl;
		f_log << "n: " << n[keyI]<< endl;
		f_log << "O: " << O[keyI]<< endl;
		f_log << "S: " << S[keyI]<< endl;
		Point2f key_point = result.queryMatched[keyI];
		for( int i=0; i<n[keyI]; i++ ){
				Point2f v_p = VotingPoints[vot_i] + key_point;
				f_log << "Voting Point["<< i << "]: ( " << v_p.x << ", " << v_p.y << " )" << endl;
				vot_i++;
		}
		f_log << "Main Voting Point: ( " << main_VotingPoints[keyI].x << ", " << main_VotingPoints[keyI].y << " )" << endl;
	}
	f_log << "Centre Points: " << "Total " << CentrePoints.size() << " Points." << endl;
	for( int i=0; i<CentrePoints.size(); i++ ){
		f_log << "Centre["<< i << "]: ( " << CentrePoints[i].x << ", " << CentrePoints[i].y << " )" << endl;
	}
	f_log.close();
	return 1;
}

int VotingScore::getScore( BowMatchResult result, BowVocabulary vocabulary ){
	// get Score of each points in result, for perfomance, this function
	// dosen't check anything
	T_rate = 0.3;
	maxS = 0;
	vector<int> S_fix;
	//vector<Point2f> main_VotingPoints; // where to store the main voting position of each keypoint
	main_VotingPoints.resize(result.keyID.size());  // some maybe useless
	for( int keyI=0; keyI<result.keyID.size(); keyI++ ){ // For each keypoints
		main_VotingPoints[ keyI ] = Point2f(0,0); // init
		int keyID = result.keyID[keyI];
		// get the voting point relate to the keypoint
		int voting_n, S_fix_n;
		if( vocabulary.getPositionByIndex( keyID, VotingPoints, voting_n ) ){
			n.push_back( voting_n );
			O.push_back( CE_VOTING_ORG_SCORE );
			// caculate transform vector
			Point2f key_point = result.queryMatched[keyI];
			Point2f V_t = Point2f(0.0, 0.0);
			int end = VotingPoints.size()-1;
			S_fix_n=0;
			for( int i=0; i<voting_n; i++ ){  // from the back
				Point2f V_i = VotingPoints[end--];
				main_VotingPoints[keyI] += V_i;
				float length = sqrtf( V_i.x * V_i.x + V_i.y * V_i.y );
				if( length<=3 ){
					 S_fix_n++;
				}
				V_i = V_i / length;
				V_t += V_i;
			}
			main_VotingPoints[keyI] = main_VotingPoints[keyI] / voting_n;
			main_VotingPoints[keyI] += key_point;
			if( S_fix_n>( voting_n>>2 ) && voting_n>=4 ) // fix those points whose transform is too short
				S_fix.push_back( keyI );
			float _V_t = V_t.x * V_t.x + V_t.y * V_t.y;
			_V_t = _V_t/voting_n - 1;
			maxS = ( _V_t>maxS ) ? _V_t : maxS;
			S.push_back( _V_t );
		}
		else
			return 0;
	}
	// fix
	for( int i=0; i<S_fix.size(); i++ )
		S[S_fix[i]] = maxS;
	// The third part, W
	    //step 1: finding clusting centre
	int vot_i=0;
	float T = T_rate*maxS;
	for( int keyI=0; keyI<result.keyID.size(); keyI++ ){
		if( S[keyI] > T ){
			Centre.check_and_add( main_VotingPoints[keyI], n[keyI] );
		}
		else{
			vot_i += n[keyI];
		}
	}
	Centre.get_center( CentrePoints );

	return 1;
}

// Search
int BowVocabulary::getIndexByImageID( int imageID ){
	// using Bisection Algorithm 
	int front=0, end=sequentialFile.imageID.size()-1;
	int mid= (front+end)/2;
	while( front<end && sequentialFile.imageID[mid]!=imageID ){
		if(sequentialFile.imageID[mid]<imageID)	front=mid+1;
		if(sequentialFile.imageID[mid]>imageID)	end=mid-1;
		mid=(front+end)/2;
	}
	if(sequentialFile.imageID[mid]!=imageID)
		return -1;
	else
		return mid;
}
// Init
int BowVocabulary::trainFlaan( ){
	CV_Assert( Vocabulary.cols !=0 );
	if( !isMatcherTrained ){
		cout << "Training the FLANN method" << endl;
		Vocabulary.convertTo( Vocabulary, CV_32F );
		//CV_Assert( Vocabulary.type() == CV_32S );
		flannMatch.add( Vocabulary );
		flannMatch.train();
		isMatcherTrained = true;
		cout << "Training End!" << endl;
	}
	return 1;
}
// Invented File
int BowVocabulary::getTFByIndex( size_t index, TeamFrequency &TF ){
	if( index<Vocabulary.rows ){
		TF = invertedFile.teamFrequency[index];
	}
	else
		return 0;
	return 1;
}
int BowVocabulary::getPositionByIndex( size_t index, keyPositions &Pos ){
	if( index<Vocabulary.rows ){
		Pos = invertedFile.positions[index];
	}
	else
		return 0;
	return 1;
}

int BowVocabulary::getPositionByIndex( int index, vector<Point2f> &Pos, int &n ){
	if( index<Vocabulary.rows ){
		//Pos = invertedFile.positions[index];
		auto point = invertedFile.positions[index].x_y;
		Pos.insert( Pos.end(), point.begin(), point.end() );
		n = point.size();
	}
	else
		return 0;
	return 1;
}

// Check if load corecctly
bool BowVocabulary::IsLoadCorecct( ){
	// Vocabulary
	int num_of_keywords = Vocabulary.rows;
	int feature_length = Vocabulary.cols;
	if( num_of_keywords ==0 ){
		cout << "Error: No Vocabulary load in!" << endl;
		return false;
	}

	// Sequential File
	int num_of_images = sequentialFile.imageID.size();
	int num_of_keypoints = 0;
	for( int imgI=0; imgI<num_of_images; imgI++ ){
		num_of_keypoints += sequentialFile.keywords[imgI].keyID.size();
	}
	if( num_of_images == 0 ){
		cout << "Error: No Sequential File load in!" << endl;
		return false;
	}

	// Inverted File
	int num_of_keywords2 = invertedFile.teamFrequency.size();
	int num_of_keypoints2 = 0;
	for( int keyI=0; keyI<num_of_keywords2; keyI++ ){
		num_of_keypoints2 += invertedFile.positions[keyI].x_y.size();
	}
	if( num_of_keywords2 == 0 ){
		cout << "Error: No Inverted File load in!" << endl;
		return false;
	}

	// all
	if( num_of_keywords != num_of_keywords2 ){
		cout << "Error: [Vocabulary] and [Inverted File] miss matched!" << endl;
		return false;
	}
	if( num_of_keywords != num_of_keywords2 ){
		cout << "Error: [Sequential File] and [Inverted File] miss matched!" << endl;
		return false;
	}

	cout << "Checking finished. Everything loaded corecctly." << endl;
	return true;
}

// Load Inverted File
bool BowVocabulary::loadInvFile( string &path ){
	cout << "Loading Inverted File..." << endl;
	FileStorage fs( path, FileStorage::READ );
    if( fs.isOpened() ){
		//int keywords_number;
		//fs["keywords_number"] >> keywords_number; // Just a simple check
		//CV_Assert( keywords_number==Vocabulary.rows );
		// call for space
		CV_Assert( Vocabulary.rows!=0 );
		invertedFile.teamFrequency.resize(Vocabulary.rows); invertedFile.teamFrequency.shrink_to_fit(); // Fit the memory size
		invertedFile.positions.resize(Vocabulary.rows); invertedFile.positions.shrink_to_fit();
		for( int keyID=0; keyID<Vocabulary.rows; keyID++ ){
			auto keyKey = &invertedFile.teamFrequency[keyID];
			auto posKey = &invertedFile.positions[keyID];
			Mat TF, pos;
			// write in
			stringstream ss;// Trans int to string
			ss<<keyID; 
			string name = ss.str();
			//cout << "step1" << endl;
			//cout << "TF"+name << endl;
			//cout << TF.size() << endl;
			fs["TF"+name] >> TF;
			//cout << "end1" << endl;
			fs["position"+name] >> pos;
			//cout << "end1" << endl;
			//cout << TF.size() << endl;
			if( TF.cols!=0 ){
				// load in
				for( int i=0; i<TF.cols; i++ ){
					keyKey->imageID.push_back( TF.at<int>(0,i) );
					keyKey->TF.push_back( TF.at<int>(1,i) );
				}
				for( int i=0; i<pos.cols; i++ ){
					posKey->x_y.push_back( Point2f( pos.at<float>(0,i), pos.at<float>(1,i) ) );
				}
			}
			//cout << "step2" << endl;
		}
        return true;
    }
    return false;
}

// Save Inverted File
bool BowVocabulary::saveInvertedFile( string &path ){
	cout << "Saving Inverted File..." << endl;
    FileStorage fs( path, FileStorage::WRITE );
    if( fs.isOpened() ){
		fs << "keywords_number" << Vocabulary.rows;
		for( int keyID=0; keyID<Vocabulary.rows; keyID++ ){
			auto keyKey = &invertedFile.teamFrequency[keyID];
			auto posKey = &invertedFile.positions[keyID];
			Mat TF( 2, keyKey->imageID.size(), CV_32S );
			Mat pos( 2, posKey->x_y.size(), CV_32F );
			for( int i=0; i<keyKey->imageID.size(); i++ ){
				TF.at<int>(0,i) = keyKey->imageID[i];
				TF.at<int>(1,i) = keyKey->TF[i];
			}
			for( int i=0; i<posKey->x_y.size(); i++ ){
				pos.at<float>(0,i) = posKey->x_y[i].x;
				pos.at<float>(1,i) = posKey->x_y[i].y;
			}
			// write in
			stringstream ss;// Trans int to string
			ss<<keyID; 
			string name = ss.str();
			fs << "TF"+name << TF;
			fs << "position"+name << pos;
		}
        return true;
    }
    return false;
}

// Trans to Inverted File
int BowVocabulary::seqFile2invFile( ){
	if( sequentialFile.imageID.size()==0 ){
		cout << "Error: No SequentialFile available! Loading or generating one first." << endl;
		return 0;
	}
	if( Vocabulary.rows==0 ){
		cout << "Error: No Vocabulary available! Loading or generating one first." << endl;
		return 0;
	}
	cout << "Trans from [Sequential File] to [Inverted File] ..." << endl;
	// call for space
	invertedFile.teamFrequency.resize(Vocabulary.rows); invertedFile.teamFrequency.shrink_to_fit(); // Fit the memory size
	invertedFile.positions.resize(Vocabulary.rows); invertedFile.positions.shrink_to_fit();
	// Loop
	/*for( int imageI = 0; imageI<sequentialFile.imageID.size(); imageI++ ){  // For each image
		auto imageKey = &sequentialFile.keywords[imageI];
		vector<int> keyID_temp;
		for( int i=0; i<imageKey->keyID.size(); i++ ){ // For each keywords in this exemplar
			int keyI = imageKey->keyID[i];
			keyID_temp.push_back(keyI); // need to sort this
			invertedFile.positions[keyI].x_y.push_back( imageKey->queryMatched[i] ); // Save position of keypoints
			//invertedFile.teamFrequency[keyI].imageID.push_back( sequentialFile.imageID[imageI] ); // Save image ID
		}
		for( int i=0; i<keyID_temp.size(); i++ ){
			
		}
	}*/

	for( int keyI = 0; keyI<Vocabulary.rows; keyI++ ){ // For each keyword
		auto keyKey = &invertedFile.teamFrequency[keyI];
		auto posKey = &invertedFile.positions[keyI];
		for( int imageI = 0; imageI<sequentialFile.imageID.size(); imageI++ ){  // For each image
			auto imageKey = &sequentialFile.keywords[imageI];
			int num_of_matched = 0;
			for( int i=0; i<imageKey->keyID.size(); i++ ){
				if( keyI == imageKey->keyID[i] ){ // If matched!!
					posKey->x_y.push_back( center_position - imageKey->queryMatched[i] ); // save the position
					num_of_matched++;
				}
			}
			if( num_of_matched!=0 ){
				keyKey->imageID.push_back(sequentialFile.imageID[imageI]); // save the image name
				keyKey->TF.push_back( num_of_matched ); // save the Team frequency
			}
		}
	}
	// Check unused keyword
	int unused_key = 0;
	vector<int> deleteIndex; // The index of keyword need deleted
	for( int keyI = 0; keyI<Vocabulary.rows; keyI++ ){
		if( invertedFile.positions[keyI].x_y.size()==0 ){
			unused_key++;
			deleteIndex.push_back( keyI );
			//cout << keyI;
		}
	}
	//cout << endl;
	//unused_key = 0;
	if( unused_key ){
		cout << "Warning: " << unused_key << " Unused keyword existed, loading may cash. Delete the Sequential File and try again." << endl;
		cout << "Try to fix it ...";
		if( deleteKeywords( deleteIndex ) ){
			cout << "finished." << endl;
		}
	}
	return 1;
}

// save sequential file
bool BowVocabulary::saveSequentialFile( string &path ){
	cout << "Saving sequential file..." << endl;
    FileStorage fs( path, FileStorage::WRITE );
    if( fs.isOpened() )
    {
		Mat imageID( 1, sequentialFile.imageID.size(), CV_32S );
		//cout << imageID.cols << endl;
		for(int j=0;j<imageID.cols;j++)
			imageID.at<int>(0,j) = sequentialFile.imageID[j];
		fs << "imageID" << imageID;
		for( int i=0; i<sequentialFile.keywords.size(); i++ ){
			Mat position( 2, sequentialFile.keywords[i].keyID.size(), CV_32F );
			Mat keyID( 1, sequentialFile.keywords[i].keyID.size(), CV_32S );
			for(int j=0;j<sequentialFile.keywords[i].keyID.size();j++){
				position.at<float>(0,j) = sequentialFile.keywords[i].queryMatched[j].x;
				position.at<float>(1,j) = sequentialFile.keywords[i].queryMatched[j].y;
				keyID.at<int>(0,j) = sequentialFile.keywords[i].keyID[j];
			}
			stringstream ss;// Trans int to string
			ss<<sequentialFile.imageID[i]; 
			string name = ss.str();
			fs << "position"+name << position;
			fs << "keyID"+name << keyID;
		}
        return true;
    }
    return false;
}

// loading sequential file
bool BowVocabulary::quantizingExemplars( string &path ){
	cout << "Loading sequential file..." << endl;
	FileStorage fs( path, FileStorage::READ );
    if( fs.isOpened() )
    {
		Mat imageID, position, keyID;
		fs["imageID"] >> imageID;
		num_of_examplers = imageID.cols;
		for(int j=0;j<imageID.cols;j++)
			sequentialFile.imageID.push_back(imageID.at<int>(0,j));
		for( int i=0; i<imageID.cols; i++ ){
			stringstream ss;// Trans int to string
			ss<<sequentialFile.imageID[i]; 
			string name = ss.str();
			fs["position"+name] >> position;
			fs["keyID"+name] >> keyID;
			BowMatchResult result;
			for(int j=0;j<keyID.cols;j++){
				result.queryMatched.push_back( Point2f( position.at<float>(0,j), position.at<float>(1,j)) );
				result.keyID.push_back( keyID.at<int>(0,j) );
				sequentialFile.keywords.push_back(result);
			}
			sequentialFile.keywords.push_back(result);
		}
        return true;
    }
    return false;
}

// Quantizing Exemplars
int BowVocabulary::quantizingExemplars( vector<Mat> &imgdata ){
	cout << "Training the FLANN method" << endl;
	Vocabulary.convertTo( Vocabulary, CV_32F );
	//CV_Assert( Vocabulary.type() == CV_32S );
	flannMatch.add( Vocabulary );
	flannMatch.train();
	cout << "Training End!" << endl;
	isMatcherTrained = true;
	cout << "Quantizing Exemplars ..." << endl;
	cout << "Totally " << imgdata.size() << "exemplars." << endl;
	// Training the FLANN method
	// processing every image
	int processImg = 0;
	for( size_t i = 0; i<imgdata.size(); i++ ){
		BowMatchResult result;
		if( quantizing( imgdata[i], result ) ){
			sequentialFile.keywords.push_back(result);
			sequentialFile.imageID.push_back( i );
			processImg++;
		}
	}
	cout << "Quantizing End! " << processImg << "exemplars included. "<< endl;
	return 1;
}

// This function is used to quantizing the input query set;
int BowVocabulary::quantizing( Mat &imgdata, BowMatchResult &result ){
	Mat descriptors;
	vector<KeyPoint> imageKeypoints;
	//cout << "Begin Detecting" << endl;
	//cout << imgdata.size() << endl;
	akazeFeature->detectAndCompute ( imgdata, noArray(), imageKeypoints, descriptors );
	//cout << "Detecting End!" << endl;
	if( descriptors.size().height<=1 ){
		cout << "Warning: No descriptors found!" << endl;
		return 0;
	}
    //CV_Assert( descriptors.size().height!=0 );
	vector<vector<DMatch>> dmatch;
	//FlannBasedMatcher flannMatch( makePtr<flann::LshIndexParams>(flann::LshIndexParams(20,8,2)), makePtr<flann::SearchParams>() );
	//FlannBasedMatcher flannMatch;
	// save a descriptors
	/*FileStorage fs( "D:/c_lab/ObjectDetection/output/cah/descript.xml", FileStorage::WRITE );
	if( fs.isOpened() ){
		fs << "descripts" << descriptors;
		Mat KeyPointsDeg;
		for( int i=0; i<imageKeypoints.size(); i++ ){
			KeyPointsDeg.push_back( imageKeypoints[i].angle );
		}
		fs << "keyPoints" << KeyPointsDeg;
    }*/
	//waitKey();
	descriptors.convertTo(descriptors,CV_32F );
	//int k = min( max( cvRound(descriptors.size().height*0.5), 20 ), descriptors.size().height); // n/2 <= k <= n, while k at least 20
	//cout << k << endl;
	//cout << "Begin KNN Matching" << endl;
	flannMatch.knnMatch( descriptors, dmatch, 2 );
	//cout << "Begin KNN Matching End." << endl;
	//cout << "descriptors Size :" << descriptors.size() << endl;
	//cout << "k :" << k << endl;
	//cout << "Dmatch Size 1:" << dmatch.size() << endl;
	//cout << "Dmatch Size 2:" << dmatch[0].size() << endl;
	vector<Point2f> keypoint;
	KeyPoint::convert( imageKeypoints, keypoint );
	for(size_t i = 0; i < dmatch.size(); i++) {
		DMatch first = dmatch[i][0];
		float dist1 = dmatch[i][0].distance;
		float dist2 = dmatch[i][1].distance;
		if(dist1 < 0.8 * dist2) {
			result.queryMatched.push_back( keypoint[first.queryIdx] );
			result.keyID.push_back( first.trainIdx );
		}
	}
	if( result.keyID.size()==0 ){
		cout << "Warning: No good descriptors found!" << endl;
		return 0;
	}
	return 1;
}

// save the vocabulary
bool BowVocabulary::saveVocabulary(string &path){
	cout << "Saving vocabulary..." << endl;
    FileStorage fs( path, FileStorage::WRITE );
    if( fs.isOpened() )
    {
        fs << "vocabulary" << Vocabulary;
        return true;
    }
    return false;
}

// read in a pre-trained vocabulary
bool BowVocabulary::generateVocabulary( string &path ){
	cout << "Loading vocabulary..." << endl;
	FileStorage fs( path, FileStorage::READ );
    if( fs.isOpened() )
    {
		fs["vocabulary"] >> Vocabulary;
        return true;
    }
    return false;
}

// This is used to delete some unimportant keywords in vocabulary. All options are done in memory, so you should save the result to disk 
// either by using save function or other self define functions
int BowVocabulary::deleteKeywords( vector<int> index ){
	// caculating step, how far next element need delete is.
	vector<int> step;
	step.push_back( index[0] );
	int i=1;
	while( i<index.size() ){
		step.push_back( index[i]-index[i-1]-1 ); // - 1 beacuse we point the pointer toward next element behind the deleted one, so it automaticly add 1
		i++;
	}
	// delete vector
	auto Iter = invertedFile.positions.begin();
	auto Iter2 = invertedFile.teamFrequency.begin();
	int step_i = 0; // as index of step
	while( Iter != invertedFile.positions.end() ){ // not out of range
		//cout << step[step_i] << endl;
		Iter += step[step_i]; // point to the element need delete
		Iter = invertedFile.positions.erase( Iter ); // delet it, and point to the element behind

		Iter2 += step[step_i]; // point to the element need delete
		Iter2 = invertedFile.teamFrequency.erase( Iter2 ); // delet it, and point to the element behind
		step_i++;
		if( step_i >= step.size() )
			break;
	}
	// delete vocabulary
	Mat VocTemp;
    int index_i = 0;
	for( int i = 0; i<Vocabulary.rows; i++ ){
		if( i == index[index_i] ){
			index_i = min( index_i+1, (int)index.size()-1 );
		}
		else{
			VocTemp.push_back( Vocabulary.row(i) );
		}
	}
	cout << Vocabulary.size() << endl;
	Vocabulary = VocTemp;
	cout << Vocabulary.size() << endl;
	return 1;
}

// generate BOW vocabulary from a image set
int BowVocabulary::generateVocabulary( vector<Mat> &imgData, BowVocParams params ){
	// Step1: Create detector, descriptor. In OpenCV 3.0, both of them are inherited from a new class: Feature2D
	// Using ORB feature
	//Ptr<ORB> akazeFeature = ORB::create( ); // No param by now
	//cout << akazeFeature->descriptorType() << endl;
	//CV_Assert( akazeFeature->descriptorType() == CV_32FC1 );
	//CV_Assert( params.targetSize.width==CE_CENTERAL_POS_x && params.targetSize.height==CE_CENTERAL_POS_y  );
	const int elemSize = CV_ELEM_SIZE( akazeFeature->descriptorType() );
	const int descByteSize = akazeFeature->descriptorSize() * elemSize;
    const int bytesInMB = 1048576; // 1MB
	const int maxDescCount = ( params.memoryUse * bytesInMB) / descByteSize; // Total number of descs to use for training.
	// Step2: caculate the vocabulary.
	TermCriteria terminate_criterion; // k-means end condition
    terminate_criterion.epsilon = FLT_EPSILON;
	BOWKMeansTrainer kmTrainer( params.vocabSize, terminate_criterion, 3, KMEANS_PP_CENTERS );  // I have no idea witch distance k-means using here 
	RNG& rng = theRNG();
	cout << "Learning begin!" << endl;
	int processCounter = 0;
	//vector<KeyPoint> imageKeypoints;
	while( imgData.size() > 0 ) // if no imgdata left, break the loop
        {
			if( kmTrainer.descriptorsCount() > maxDescCount )
            {
#ifdef DEBUG_DESC_PROGRESS
                cout << "Breaking due to full memory ( descriptors count = " << kmTrainer.descriptorsCount()
                        << "; descriptor size in bytes = " << descByteSize << "; all used memory = "
                        << kmTrainer.descriptorsCount()*descByteSize << endl;
#endif
                break;
            }
            // Randomly pick an image from the dataset which hasn't yet been seen
            // and compute the descriptors from that image.
			int randImgIdx = rng( (unsigned)imgData.size() );
            Mat imageDescriptors;
			//imageKeypoints.resize(0);
			vector<KeyPoint> imageKeypoints;
			akazeFeature->detectAndCompute ( imgData[randImgIdx], noArray(), imageKeypoints, imageDescriptors ); // Here maybe we can use BING to acculate
			//cout << imageDescriptors.size().width << endl;
			imageDescriptors.convertTo( imageDescriptors, CV_32FC1);

			//check that there were descriptors calculated for the current image
            if( !imageDescriptors.empty() )
            {
                int descCount = imageDescriptors.rows;
                // Extract trainParams.descProportion descriptors from the image, breaking if the 'allDescriptors' matrix becomes full
				int descsToExtract = static_cast<int>(params.descProportion * static_cast<float>(descCount));
                // Fill mask of used descriptors
				vector<char> usedMask( descCount, false );
                fill( usedMask.begin(), usedMask.begin() + descsToExtract, true );
                for( int i = 0; i < descCount; i++ ) // random select
                {
                    int i1 = rng(descCount), i2 = rng(descCount);
                    char tmp = usedMask[i1]; usedMask[i1] = usedMask[i2]; usedMask[i2] = tmp;
                }
                for( int i = 0; i < descCount; i++ )
                {
                    if( usedMask[i] && kmTrainer.descriptorsCount() < maxDescCount  )
                        kmTrainer.add( imageDescriptors.row(i) );
                }
            }
#ifdef DEBUG_DESC_PROGRESS
			cout << imgData.size() << " images left, " << processCounter+1 << " processed - "
                    <<
					cvRound((static_cast<double>(kmTrainer.descriptorsCount())/static_cast<double>(maxDescCount))*100.0)
                    << " % memory used" << ( imageDescriptors.empty() ? " -> no descriptors extracted, skipping" : "") << endl;
#endif
            // Delete the current element from images so it is not added again
			imgData.erase( imgData.begin() + randImgIdx );
			processCounter++;
        }
        cout << "Maximum allowed descriptor count: " << maxDescCount << ", Actual descriptor count: " << kmTrainer.descriptorsCount() << endl;

        cout << "Training vocabulary..." << endl;
		//cout << kmTrainer.descriptors[0].type() << endl;
		Vocabulary = kmTrainer.cluster();

	return 1;
}