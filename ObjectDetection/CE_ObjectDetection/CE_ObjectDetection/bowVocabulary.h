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
#include <fstream>
#include <sstream>

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
	vector<float> k; // k for each line, (xj-xi) is set to 1 if it is zero.
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
	// Setting
	void setting( Size2f Size, Point2f center ){ targetSize = Size; center_position = center; return; }
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
	int getNumOfImages( ){ return (int)sequentialFile.imageID.size(); } 
		// In Inverted File
	int getTFByIndex( size_t index, TeamFrequency &TF );
	int getPositionByIndex( size_t index, keyPositions &TF );
	int getPositionByIndex( int index, vector<Point2f> &Pos, int &n );
	int getLineByIndex( int index, vector<float> &a, vector<float> &b, vector<float> &c, vector<float> &_a, vector<float> &_b, vector<float> &_c, Point2f keyPos );
	int initInvertedFile(  ); // get k or something else dosen't saved in disk
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
	// targetSize
	Size2f targetSize;
	Point2f center_position;
};        

// Score, including three parts
class ClustingCenter{
public:
	ClustingCenter( ) : T_distance( 800.0 ){};
	int check_and_add( Point2f point, int n );
	void get_center( vector<Point2f> &c ){ c = center; };
	void get_max_center( float scale, vector<int> &index );
private:
	vector<Point2f> center;
	vector<int> num; // num of points belong to this center
	float T_distance;
	int max_num;
};

class VotingScore2{
public:
	int getVotingRect( BowMatchResult result, int range );
	int drawVotingRect( BowMatchResult result, Mat &VotingMap );
	int getProbCenter( BowVocabulary vocabulary, BowMatchResult result, vector<Point2f> &ProbCenter, float sample_rate );
private:
	vector<Rect> VotingRect; // The possible rect of face center
	vector<int> labels; // Which rect does each point belong to 
	vector<int> num_of_point; // Num of point belongs to one rect
};

// this struct just for partition function in votingScore2
struct PointLike{
        PointLike(int thresh){
                this->thresh = thresh;
        }
        bool operator()(cv::Point p1,cv::Point p2){
                int x = int(p1.x - p2.x);
                int y = int(p1.y - p2.y);
                //return x*x+y*y <=thresh*thresh;
				return x*x+y*y <=thresh;
        }
        int thresh;
};

class VotingScore{
public:
	int getScore( BowMatchResult result, BowVocabulary vocabulary );
	int saveScore( BowVocParams parms, BowMatchResult result );
	void drawVoting( Mat &VotingMap, BowMatchResult result );
private:
	vector<int> n;    // num of voting points come from each keypoints
	vector<float>O;    // Orignal score of each keypoints
	vector<float>S;    // divergency score, higher if transform vectors appear closly
	vector<Point2f>VotingPoints;    // The position of each voting points
	vector<float>FinalScore;    // Final score of each voting points
	vector<Point2f>CentrePoints;
	ClustingCenter Centre;
	float maxS;
	float scare;    // The scope of voting points, each point in this area will be added together to 
	                // find the central point of target object
	float T_rate;   // The T of max divergency score, only the one larger than that
	                //  should be consider as central point
	Size query_size;    // The size of query image

	vector<Point2f> main_VotingPoints;

	friend class ClustingCenter;
};

#endif