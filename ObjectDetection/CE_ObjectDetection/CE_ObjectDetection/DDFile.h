// This file is used to generate and maintain a Database describe file (DDFile), 
// which contains information of each image.
// Each line in this file is organized as follow:
// ClassID ) filepath ) size ) central point ) image ID ;
// Each parts is divised by ")", and notice that the image ID is given by DDFile generating process, by default setting.
#ifndef DDFILE_H
#define DDFILE_H

#include "stdafx.h"
#include "string.h"
#include "iostream"
#include <fstream>
#include <sstream>
#include "vector"
#include <opencv2/core/core.hpp>
#include <highgui.hpp>
#include "imgproc.hpp"

using namespace std;

class DDFile {
public:
	DDFile( string filepath, string indexpath ) : DDFilePath(filepath), IndexPath(indexpath){};
	bool Init();
	bool Save();
	// generate function
	int InsertNewLine( unsigned int ClassID, string filepath, vector<int> &imgsize, vector<float> &centralPoint );
	int InsertNewClass( unsigned int ClassID, string basepath, unsigned int num_of_image );
	int InsertNewClass( unsigned int ClassID, string basepath, unsigned int num_of_image, string s1, unsigned int isFit );
	// Option
		// Search
	unsigned int IndexSearch( unsigned int ID, bool &IsExist );
private:
	string DDFilePath;
	string IndexPath;
	vector<unsigned int> max_ID_of_each_class; 
	vector<unsigned int> ClassIDs;
	unsigned int CacheSize; // The cache technology is used to read in many lines at one time
	vector<string> Cache;
};

#endif