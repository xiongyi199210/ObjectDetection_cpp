#include "stdafx.h"
#include "ImgRead.h"

using namespace cv;
using namespace std;

int Imgread::BeginRead( vector<Mat> &imgData, vector<Point2f> &centralPoint ){
	float x_rate = 0;
	// open ddr file
	fstream file;
	file.open( path[0], ios::in );
	if(!file){
		cout<< endl << "Error: Bad path to ddf file." << endl;
		return -1;
	}
	string line, classID, imagepath, size, cenPoint_y, cenPoint_x, imageID;
	while (getline(file, line)) {
        stringstream liness(line);
		getline(liness, classID, ')');
		getline(liness, imagepath, ')');
		getline(liness, size, ')');
		getline(liness, cenPoint_y, ',');
		getline(liness, cenPoint_x, ')');
		getline(liness, imageID, ';');
		// Load in central points
		Point2f cenPoint = Point2f( atof(cenPoint_y.c_str()), atof(cenPoint_x.c_str()) );
		// Load in images
		Mat imgdata = imread( imagepath );
		//cout << imagepath << endl;
		//cout << imgdata.size() << endl;
		if( imgdata.size().height == 0 )
			cout << "Error: Can't read in image at: " << imagepath << endl;
		// fill until imgM == imgN
		Size imgsize = imgdata.size();
		int fillBlank = imgsize.height - imgsize.width;
		Mat imgTemp;
		bool needT = false;
		if( fillBlank>0 ){
			imgdata = imgdata.t();
			imgsize = imgdata.size();
			fillBlank = imgsize.height - imgsize.width;
			needT = true;
		}
		if( fillBlank!=0 ){    // if imgM !=imgN
			fillBlank = -fillBlank;
			imgTemp = Mat::zeros( imgsize.width, imgsize.width, imgdata.type());    //  Make a [ imgN, imgN ] image
			int fillhalf = fillBlank>>1;  // [fillBlank/2]
			for( int i=0; i<3*imgsize.width; i+=3 ){    // part 1
				uchar datavalue1 = imgdata.at<uchar>(0,i);
				uchar datavalue2 = imgdata.at<uchar>(0,i+1);
				uchar datavalue3 = imgdata.at<uchar>(0,i+2);
				//cout << datavalue << endl;
				for( int j=0; j<fillhalf; j++ ){
					imgTemp.at<uchar>(j,i) = datavalue1;
					imgTemp.at<uchar>(j,i+1) = datavalue2;
					imgTemp.at<uchar>(j,i+2) = datavalue3;
				}
			}
			Range colrange( 0, imgsize.width );    // part 2
			Range rowrange( fillhalf, fillhalf+imgsize.height );
			imgdata.copyTo( Mat(imgTemp,rowrange, colrange) );
			for( int i=0; i<3*imgsize.width; i+=3 ){    // part 3
				uchar datavalue1 = imgdata.at<uchar>(imgsize.height-1, i);
				uchar datavalue2 = imgdata.at<uchar>(imgsize.height-1, i+1);
				uchar datavalue3 = imgdata.at<uchar>(imgsize.height-1, i+2);
				for( int j=fillhalf+imgsize.height; j<imgsize.width; j++ ){
					imgTemp.at<uchar>(j,i) = datavalue1;
					imgTemp.at<uchar>(j,i+1) = datavalue2;
					imgTemp.at<uchar>(j,i+2) = datavalue3;
				}
			}
			// central point
			cenPoint.y += fillhalf;
		}
		else
			imgTemp = imgdata;
		if( needT )
			imgTemp = imgTemp.t();

		//imshow("c", imgdata);
		//imshow("b", imgTemp);
		// rerocate central point
		float rate = (float)sample_size.height / (float)imgTemp.cols;
		cenPoint.y *= rate;
		cenPoint.x *= rate;
		centralPoint.push_back( cenPoint );
		// resize to the target size
		resize( imgTemp, imgTemp, sample_size );
		//resize( imgTemp, imgTemp, sample_size );
		imgData.push_back( imgTemp );
		//imshow("a", imgTemp);
		//waitKey();
		x_rate += rate;
    }
	x_rate = imgData.size() / x_rate;
	cout << "Loading finished. Totally " << imgData.size() << " samples included." << endl;
	cout << "scare paramate of this set is:" << x_rate << endl;
	return 1;
}

void Imgread::BeginRead( vector<Mat> &imgData, const string &str, const string &format, unsigned int startNum, unsigned int endNum, unsigned int isFit ){
	// rate
	float x_rate = 0;
	float y_rate = 0;
	for( int i=startNum; i<=endNum; i++ ){
		// structure the name of image
		string str2;
		if( isFit ){ // insert some "0" front of i to fit the same length of name
			unsigned int str2_l = 0;
			while( pow( 10, isFit-str2_l++ ) > i );
			str2_l = min( str2_l-2, isFit-1 );
			str2.insert( str2.size(), str2_l, '0' );
		}
		// Trans everything to string
		stringstream ss;
		ss<<i; 
		string si = ss.str();
		string ImgName = str + str2 + si + format;
		// read in a image
		Mat imgTemp = imread( path[0]+ImgName );
		if( imgTemp.size().height == 0 )
			cout << "Error: Can't read in image at: " <<path[0]+ImgName << endl;
		// resize to the target size
		int pre_size = (int)(min( imgTemp.size().height, imgTemp.size().width ) * 0.7);
		int row_offset = 30;
		Range rowrange( (int)(imgTemp.size().height-pre_size)/2+row_offset, (int)(imgTemp.size().height+pre_size)/2+row_offset );
		Range colrange( (int)(imgTemp.size().width-pre_size)/2, (int)(imgTemp.size().width+pre_size)/2 );
		resize( Mat(imgTemp,rowrange, colrange), imgTemp, sample_size );
		//resize( imgTemp, imgTemp, sample_size );
		imgData.push_back( imgTemp );
		x_rate += ( (float)(imgTemp.size().width+pre_size)/2 - (float)(imgTemp.size().width-pre_size)/2 ) / (float)sample_size.width;
		y_rate += ( (float)(imgTemp.size().height+pre_size)/2 - (float)(imgTemp.size().height-pre_size)/2 ) / (float)sample_size.height;
	}
	//cout << x_rate << ", " << y_rate << endl;
	x_rate /= (float)(endNum-startNum+1);
	y_rate /= (float)(endNum-startNum+1);
	cout << "scare paramate of this set is:" << x_rate << ", " << y_rate << endl;
}