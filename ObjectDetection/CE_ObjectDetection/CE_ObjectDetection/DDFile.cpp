#include "stdafx.h"
#include "DDFile.h"

using namespace cv;
using namespace std;

// Searching with Class ID
unsigned int DDFile::IndexSearch( unsigned int ID, bool &IsExist ){
	// using Bisection Algorithm 
	if( ClassIDs.size() == 0 ){
		IsExist = false;
		return 0;
	}
	if( ID==1 ){ // Since ID is an unsigned int, it can't be negative value
		if( ClassIDs[0]==ID )
			IsExist = true;
		else
			IsExist = false;
		return 0;
	}
	unsigned int front=0, end = (unsigned int)ClassIDs.size()-1;
	unsigned int mid= (front+end)/2;
	while( front<=end && ClassIDs[mid]!=ID ){
		if(ClassIDs[mid]<ID)	front=mid+1;
		if(ClassIDs[mid]>ID)	end=mid-1;
		mid=(front+end)/2;
	}
	if(ClassIDs[mid]!=ID){
		IsExist = false;
		//return mid;
	}
	else{
		IsExist = true;
		//return mid;
	}
	return mid;
}

// Init
bool DDFile::Init(  ){
	cout << "Loading Database Describe File..." << endl;
	/*FileStorage fs( IndexPath, FileStorage::READ );
	Mat ClassIDs_t, max_ID_of_each_class_t;
    if( fs.isOpened() ){
		fs["ClassIDs"] >> ClassIDs_t;
		//cout << ClassIDs_t.cols << endl;
		if( ClassIDs_t.cols==0 )
			return true;
		fs["max_ID_of_each_class"] >> max_ID_of_each_class_t;
		for( int i=0; i<ClassIDs_t.cols; i++ ){
			ClassIDs.push_back( ClassIDs_t.at<unsigned int>(0,i) );
			max_ID_of_each_class.push_back( max_ID_of_each_class_t.at<unsigned int>(0,i) );
		}
        return true;
    }
	else
		cout << "Error: No Database Describe File!" << endl;
    return false;*/
	// Open DDFile
	fstream file;
	file.open( IndexPath, ios::in );
	if(!file){
		cout << "Error: Bad path to Database Describe file." << endl;
		return false;
	}
	string mark; file >> mark; cout << mark << endl;
    unsigned int num_of_ClassID; file >> num_of_ClassID; cout << num_of_ClassID <<endl;
	//mark.clear();
	file >> mark; cout << mark << endl;
	unsigned int classID_t;
	for( unsigned int i=0; i<num_of_ClassID; i++ ){
		file >> classID_t; ClassIDs.push_back( classID_t ); cout << classID_t << '\t' ;
	}
	cout << endl;
	file >> mark; cout << mark << endl;
	unsigned int max_ID_of_each_class_t;
	for( unsigned int i=0; i<num_of_ClassID; i++ ){
		file >> max_ID_of_each_class_t; max_ID_of_each_class.push_back( max_ID_of_each_class_t ); cout << max_ID_of_each_class_t << '\t' ;
	}
	cout << endl;

	file.close();
	return true;
}

// Save index file
bool DDFile::Save(  ){
	//cout << "Saving Database Describe File..." << endl;
	/*FileStorage fs( IndexPath, FileStorage::WRITE );
	Mat ClassIDs_t = Mat::zeros(1, ClassIDs.size(), CV_16U);
	Mat max_ID_of_each_class_t = Mat::zeros(1, ClassIDs.size(), CV_16U);
    if( fs.isOpened() ){
		cout << ClassIDs_t.cols << endl;
		for( int i=0; i<ClassIDs_t.cols; i++ ){
			ClassIDs_t.at<unsigned int>(0,i) = ClassIDs[i];
			max_ID_of_each_class_t.at<unsigned int>(0,i) = max_ID_of_each_class[i];
		}
		cout << "step2" << endl;
		fs << "ClassIDs" << ClassIDs_t;
		fs << "max_ID_of_each_class" << max_ID_of_each_class_t;
        return true;
    }
    return false;*/
	// Open DDFile
	fstream file;
	file.open( IndexPath, ios::out );
	if(!file){
		cout << "Error: Bad path to Database Describe file." << endl;
		return false;
	}
	file << "num_of_calss:" << endl;
	file << ClassIDs.size() << endl;
	file << "ClassID:" << endl;
	for( int i=0; i<ClassIDs.size(); i++ ){
		file << ClassIDs[i] << '\t';
	}
	file << endl << "max_ID_of_each_class:" << endl;
	for( int i=0; i<ClassIDs.size(); i++ ){
		file << max_ID_of_each_class[i] << '\t';
	}
	file << endl;
	file.close();
	return true;
}

// Insert a new Class with some images
int DDFile::InsertNewClass( unsigned int ClassID, string basepath, unsigned int num_of_image, string s1, unsigned int isFit ){
	// Check if ClassID exist
	bool IsIDExist = false;
	unsigned int ID_index = IndexSearch( ClassID, IsIDExist );
	unsigned int ImageID = 0;
	if( IsIDExist ){ // If ID exist
		ImageID = max_ID_of_each_class[ID_index];
		max_ID_of_each_class[ID_index] += num_of_image;
	}
	else{	// If ID doesn't exist
		unsigned int i;
		if( ClassIDs.size() == 0 || ClassIDs[ID_index] < ClassID ){
			ClassIDs.push_back( ClassID );
			max_ID_of_each_class.push_back( num_of_image );
		}
		else{
			if( ID_index==0 ){ // Bug: vector can't insert menmber at head
				ClassIDs.insert( ClassIDs.begin()+1, ClassIDs[0] );
				ClassIDs[0] = ClassID;
				max_ID_of_each_class.insert( max_ID_of_each_class.begin()+1, max_ID_of_each_class[0] );
				max_ID_of_each_class[0] = num_of_image; 
			}
			else{
				ClassIDs.insert( ClassIDs.begin()+ID_index, ClassID );
				max_ID_of_each_class.insert( max_ID_of_each_class.begin()+ID_index, num_of_image );
			}
		}
	}
	// Open DDFile
	fstream file;
	file.open( DDFilePath, ios::app );
	if(!file){
		cout << "Error: Bad path to Database Describe file." << endl;
		return -1;
	}
	// Offer new imageID
	unsigned long Image_H_16_bit = (unsigned long)ClassID << 16;
	for( unsigned int i=0; i<num_of_image; i++ ){
		unsigned long LongImageID = Image_H_16_bit + (unsigned long)ImageID;
		// structure the name of image
		string str2;
		if( isFit ){ // insert some "0" front of i to fit the same length of name
			unsigned int str2_l = 0;
			while( pow( 10, isFit-str2_l++ ) > i+1 );
				str2_l = min( str2_l-2, isFit-1 );
			str2.insert( str2.size(), str2_l, '0' );
		}
		// Trans everything to string
		stringstream ss1;
		ss1<<i+1; 
		string si = ss1.str();
		string ImgName = basepath + s1 + str2 + si + ".jpg";
		//cout<< ImgName << endl;
		Mat imgdata = imread( ImgName );
		if( imgdata.size().height == 0 )
			cout << "Error: Can't read in image at: " <<ImgName << endl;
		//cout << "ImageSize:" << imgdata.size() << ", " << imgdata.channels() << endl;
		// string in
		stringstream ss;// Trans int to string
		ss<< ClassID << ')' << ImgName << ')'; 
		ss<< imgdata.rows << ',' << imgdata.cols << ',' << imgdata.channels() << ')' ;
		ss<< imgdata.rows/2 << ',' << imgdata.cols/2 << ')' << LongImageID << ';' << endl ;
		string info = ss.str();
		file << info;

		ImageID++;
	}
	file.close();

	return 1;
}

int DDFile::InsertNewClass( unsigned int ClassID, string basepath, unsigned int num_of_image ){
	// Check if ClassID exist
	bool IsIDExist = false;
	unsigned int ID_index = IndexSearch( ClassID, IsIDExist );
	unsigned int ImageID = 0;
	if( IsIDExist ){ // If ID exist
		ImageID = max_ID_of_each_class[ID_index];
		max_ID_of_each_class[ID_index] += num_of_image;
	}
	else{	// If ID doesn't exist
		unsigned int i;
		//for( i=0; i<ClassIDs.size(); i++ ){
		//	if( ClassIDs[i] > ClassID )
		//		break;
		//}
		//if( i==ClassIDs.size() ){
		if( ClassIDs.size() == 0 || ClassIDs[ID_index] < ClassID ){
			ClassIDs.push_back( ClassID );
			max_ID_of_each_class.push_back( num_of_image );
		}
		else{
			if( ID_index==0 ){ // Bug: vector can't insert menmber at head
				ClassIDs.insert( ClassIDs.begin()+1, ClassIDs[0] );
				ClassIDs[0] = ClassID;
				max_ID_of_each_class.insert( max_ID_of_each_class.begin()+1, max_ID_of_each_class[0] );
				max_ID_of_each_class[0] = num_of_image; 
			}
			else{
				ClassIDs.insert( ClassIDs.begin()+ID_index, ClassID );
				max_ID_of_each_class.insert( max_ID_of_each_class.begin()+ID_index, num_of_image );
			}
		}
	}
	// Open DDFile
	fstream file;
	file.open( DDFilePath, ios::app );
	if(!file){
		cout << "Error: Bad path to Database Describe file." << endl;
		return -1;
	}
	// Offer new imageID
	unsigned long Image_H_16_bit = (unsigned long)ClassID << 16;
	for( unsigned int i=0; i<num_of_image; i++ ){
		unsigned long LongImageID = Image_H_16_bit + (unsigned long)ImageID;
		// Trans everything to string
		stringstream ss1;
		ss1<<i+1; 
		string si = ss1.str();
		string ImgName = basepath + si + ".jpg";
		Mat imgdata = imread( ImgName );
		//cout << "ImageSize:" << imgdata.size() << ", " << imgdata.channels() << endl;
		// string in
		stringstream ss;// Trans int to string
		ss<< ClassID << ')' << ImgName << ')'; 
		ss<< imgdata.rows << ',' << imgdata.cols << ',' << imgdata.channels() << ')' ;
		ss<< imgdata.rows/2 << ',' << imgdata.cols/2 << ')' << LongImageID << ';' << endl ;
		string info = ss.str();
		file << info;

		ImageID++;
	}
	file.close();

	return 1;
}

// Insert a new line into DDFile based on ClassID
int DDFile::InsertNewLine( unsigned int ClassID, string filepath, vector<int> &imgsize, vector<float> &centralPoint ){
	// Check if ClassID exist
	bool IsIDExist = false;
	unsigned int ID_index = IndexSearch( ClassID, IsIDExist );
	unsigned int ImageID = 0;
	if( IsIDExist ){ // If ID exist
		ImageID = ++max_ID_of_each_class[ID_index];
	}
	else{	// If ID doesn't exist
		ImageID = 1;
		unsigned int i;
		//for( i=0; i<ClassIDs.size(); i++ ){
		//	if( ClassIDs[i] > ClassID )
		//		break;
		//}
		//if( i==ClassIDs.size() ){
		if( ClassIDs.size() == 0 || ClassIDs[ID_index] < ClassID ){
			ClassIDs.push_back( ClassID );
			max_ID_of_each_class.push_back( 1 );
		}
		else{
			ClassIDs.insert( ClassIDs.begin()+ID_index, ClassID );
			max_ID_of_each_class.insert( max_ID_of_each_class.begin()+ID_index, 1 );
		}
	}
	// Offer Image ID
	unsigned long LongImageID = ((unsigned long)ClassID << 16) + (unsigned long)ImageID;
	// Open DDFile
	fstream file;
	file.open( DDFilePath, ios::app );
	if(!file){
		cout << "Error: Bad path to Database Describe file." << endl;
		return -1;
	}
	// string in
	stringstream ss;// Trans int to string
	ss<< ClassID << ')' << filepath << ')'; 
	ss<< imgsize[0] << ',' << imgsize[1] << ',' << imgsize[2] << ')' ;
	ss<< centralPoint[0] << ',' << centralPoint[1] << ')' << LongImageID << ';' << endl ;
	string info = ss.str();
	file << info;
	file.close();
}