// bowVocabulary.h
//#pragma once
#ifndef BOWVOCABULARY_H
#define BOWVOCABULARY_H

#include "stdafx.h"
#include "string.h"
#include "iostream"  
#include <opencv2/core/core.hpp>
#include <highgui.hpp>
#include <features2d.hpp>
#include "imgproc.hpp"

using namespace cv;
using namespace std;

// Frequencies Dictionary, including: image name, team frequency
class TeamFrequency{
public:
	vector<int> imageID;
	vector<int> TF;
};

// Position Dictionary, including: position of keypoints
class keyPositions{
public:
	vector<Point2f> x_y;
};

// Matching result, including: keypoints position; keyID
class BowMatchResult{
public:
	vector<Point2f> queryMatched;
	vector<int> keyID;
};

// Sequential File: image name -> keywords
class SequentialFile{
public:
protected:
	vector<int> imageID;
	vector<BowMatchResult> keywords;

	friend class BowVocabulary;
};

// Inverted File: keywords -> image name
class InvertedFile{
public:
protected:
	//vector<int> keyID; // Term Dictionary
	vector<TeamFrequency> teamFrequency; //Frequencies Dictionary;
	vector<keyPositions> positions;  //Position Dictionary

	friend class BowVocabulary;
};

class BowVocParams{
public:
	// function
	BowVocParams() : vocabSize(1000), memoryUse(200), descProportion(0.3f) {}
	BowVocParams( int _vocabSize, int _memoryUse, float _descProportion ) : 
		vocabSize((int)_vocabSize), memoryUse((int)_memoryUse), descProportion(_descProportion) {}
	// value
	int vocabSize; //number of visual words in vocabulary to train
    int memoryUse; // Memory to preallocate (in MB) when training vocab.
                   // Change this depending on the size of the dataset/available memory.
    float descProportion; // Specifies the number of descriptors to use from each image as a proportion of the total num descs.
	string OutPath;
	string SavePath;
	string SequentialFilePath;
	string InvertedFilePath;
	Size2i targetSize;
};                

class BowVocabulary{
public:
	BowVocabulary() : Vocabulary(Mat::zeros(0,0,CV_32FC1)), akazeFeature(AKAZE::create(5,0,3,0.0003f,4,4,1) ), isMatcherTrained(false) {};

	// Vocabulary
	int generateVocabulary( vector<Mat> &imgData, BowVocParams params );
	bool generateVocabulary( string &path );
	int deleteKeywords( vector<int> index );  // This is used to delete some unimportant keywords in vocabulary. Saving the result by yourself.
	// Save
	bool saveVocabulary(string &path);
	bool saveSequentialFile( string &path );
	bool saveInvertedFile( string &path );
	// Exemplars
	int quantizing( Mat &imgdata, BowMatchResult &result ); // This function is used to quantizing the input query set;
	int quantizingExemplars( vector<Mat> &imgData );
	bool quantizingExemplars( string &path );
	int seqFile2invFile( );  // This function is used to transfrom sequentialFile to invertedFile;
	bool loadInvFile( string &path );
	bool IsLoadCorecct( );  // Check if all three files are loaded rightly
	// Search
		// Init step
	int trainFlaan( );
	bool IsMatcherTrained() { return isMatcherTrained; }
		// In sequentialFile
	int getImageIDByIndex( int index ){ return sequentialFile.imageID[index]; }  // Get target imageID by index like: 0,1,2,3 ...
	int getIndexByImageID( int imageID ); // Use this to get a index
	void getExemplarKeyPoints( int index, vector<Point2f> &keypoints ){ keypoints = sequentialFile.keywords[index].queryMatched; }
	int getNumOfImages( ){ return sequentialFile.imageID.size(); } 
		// In Inverted File
	int getTFByIndex( size_t index, TeamFrequency &TF );
	int getPositionByIndex( size_t index, keyPositions &TF );
protected:                  
	Mat Vocabulary;   
	// Using AKAZE feature
	Ptr<AKAZE> akazeFeature;
	// using FLANN method to get martch
	FlannBasedMatcher flannMatch;
	bool isMatcherTrained; 
	// file for search
	SequentialFile sequentialFile;
	InvertedFile invertedFile;
	// num
	int num_of_examplers;
};                                                                               

#endif