// ImgRead.h
//#pragma once
#ifndef IMGREAD_H
#define IMGREAD_H

#include "stdafx.h"
#include "string.h"
#include "iostream"  
#include <opencv2/core/core.hpp>
#include <highgui.hpp>
#include "imgproc.hpp"
#include <fstream>
#include <sstream>

using namespace cv;
using namespace std;

class Imgread {
public:
	// structure function
	Imgread( vector<string> &ImgPath ) : path(ImgPath), sample_size( Size2i(80, 80) ) { };
	Imgread( vector<string> &ImgPath, Size2i &target_size ) : path(ImgPath), sample_size(target_size) { };
	//void BeginRead( vector<Mat> &imgData );
	int BeginRead( vector<Mat> &imgData, vector<Point2f> &centralPoint ); // directly read in ( all images in ddr file should be load into memory )
	void BeginRead( vector<Mat> &imgData, const string &str, const string &format, unsigned int startNum, unsigned int endNum, unsigned int isFit );
private:
	vector<string> path;
	Size2i sample_size;
};


#endif 