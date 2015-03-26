#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>
#include <stdio.h>
#include <dirent.h>

#define FILENAME_LENGTH 32
#define FILE_HOLENUM_DIGITS 3
using namespace std;
using namespace cv;

#include "RefocusingCoordinates.h"

float zMin, zMax, zStep;
string datasetRoot;

class R_image{
  
  public:
        cv::Mat Image;
        int led_num;
        float tan_x;
        float tan_y;
};

void circularShift(Mat img, Mat result, int x, int y){
    int w = img.cols;
    int h  = img.rows;

    int shiftR = x % w;
    int shiftD = y % h;
    
    if (shiftR < 0)
        shiftR += w;
    
    if (shiftD < 0)
        shiftD += h;

    cv::Rect gate1(0, 0, w-shiftR, h-shiftD);
    cv::Rect out1(shiftR, shiftD, w-shiftR, h-shiftD);
    
	 cv::Rect gate2(w-shiftR, 0, shiftR, h-shiftD); //rect(x, y, width, height)
	 cv::Rect out2(0, shiftD, shiftR, h-shiftD);
    
	 cv::Rect gate3(0, h-shiftD, w-shiftR, shiftD);
	 cv::Rect out3(shiftR, 0, w-shiftR, shiftD);
    
	 cv::Rect gate4(w-shiftR, h-shiftD, shiftR, shiftD);
	 cv::Rect out4(0, 0, shiftR, shiftD);
   
    cv::Mat shift1 = img ( gate1 );
    cv::Mat shift2 = img ( gate2 );
    cv::Mat shift3 = img ( gate3 );
    cv::Mat shift4 = img ( gate4 );
   
//   if(shiftD != 0 && shiftR != 0)

	   shift1.copyTo(cv::Mat(result, out1));
	if(shiftR != 0)
    	shift2.copyTo(cv::Mat(result, out2));
	if(shiftD != 0)
    	shift3.copyTo(cv::Mat(result, out3));
	if(shiftD != 0 && shiftR != 0)
    	shift4.copyTo(cv::Mat(result, out4));

    //result.convertTo(result,img.type());
}

int loadImages(string datasetRoot, vector<R_image> *images) {
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (datasetRoot.c_str())) != NULL) {
	  
      int num_images = 0;
      cout << "Loading Images..." << endl;
	  while ((ent = readdir (dir)) != NULL) {
		//add to list
		string fileName = ent->d_name;
		string filePrefix = "_scanning_";
		if (fileName.compare(".") != 0 && fileName.compare("..") != 0)
		{
		   string holeNum = fileName.substr(fileName.find(filePrefix)+filePrefix.length(),FILE_HOLENUM_DIGITS);
		   //cout << "Filename is: " << fileName << endl;
		 //  cout << "Filename is: " << fileName << ". HoleNumber is: " << holeNum << endl;
		R_image currentImage;
		currentImage.led_num = atoi(holeNum.c_str());
		//currentImage.Image = imread(datasetRoot + "/" + fileName, CV_8UC1);
		currentImage.Image = imread(datasetRoot + "/" + fileName, -1);//apparently - loads with a?

		currentImage.tan_x = -domeCoordinates[currentImage.led_num][0] / domeCoordinates[currentImage.led_num][2];
		currentImage.tan_y = domeCoordinates[currentImage.led_num][1] / domeCoordinates[currentImage.led_num][2];
		(*images).push_back(currentImage);
		num_images ++;
		}
	  }
	  closedir (dir);
	  return num_images;

	} else {
	  /* could not open directory */
	  perror ("");
	  return EXIT_FAILURE;
	}
}

void computeFocusDPC(vector<R_image> iStack, int fileCount, float z, int width, int height, int xcrop, int ycrop, Mat* results) {
    int newWidth = width;// - 2*xcrop;
    int newHeight = height;// - 2*ycrop;

    cv::Mat bf_result = cv::Mat(newHeight, newWidth, CV_16UC3, double(0));
	 cv::Mat dpc_result_tb = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
 	 cv::Mat dpc_result_lr = cv::Mat(newHeight, newWidth, CV_16SC1,double(0));
 	 
    cv::Mat bf_result8 = cv::Mat(newHeight, newWidth, CV_8UC3);
    cv::Mat dpc_result_tb8 = cv::Mat(newHeight, newWidth, CV_8UC1);
    cv::Mat dpc_result_lr8 = cv::Mat(newHeight, newWidth, CV_8UC1);
    
    cv::Mat img;
	 cv::Mat img16;
    cv::Mat shifted = cv::Mat(iStack[0].Image.rows, iStack[0].Image.cols, CV_16UC3,double(0));
    vector<Mat> channels(3);
    for (int idx = 0; idx < fileCount; idx++)
        {
         // Load image, convert to 16 bit grayscale image
         img = iStack[idx].Image;

         // Get home number
         int holeNum = iStack[idx].led_num;

         // Calculate shift based on array coordinates and desired z-distance
         int xShift = (int) round(z*iStack[idx].tan_x);
         int yShift = (int) round(z*iStack[idx].tan_y);

         // Shift the Image in x and y
			circularShift(img, shifted, yShift, xShift);
			
			// Add Brightfield image
			cv::add(bf_result, shifted, bf_result);
			
			// Convert shifted to b/w for DPC
			split(shifted, channels);
			channels[1].convertTo(channels[1],dpc_result_lr.type());
			
			if (find(leftList, leftList + 30, holeNum) != leftList + 30)
             cv::add(dpc_result_lr, channels[1], dpc_result_lr);
         else
             cv::subtract(dpc_result_lr, channels[1], dpc_result_lr);

         if (find(topList, topList + 30, holeNum) != topList + 30)
             cv::add(dpc_result_tb, channels[1], dpc_result_tb);
         else
             cv::subtract(dpc_result_tb, channels[1], dpc_result_tb);

         //float progress = 100*((idx+1) / (float)fileCount);
         //cout << progress << endl;
        }
        
        // Scale the values to 8-bit images
        double min_1, max_1, min_2, max_2, min_3, max_3;
        
        cv::minMaxLoc(bf_result, &min_1, &max_1);
	     bf_result.convertTo(bf_result8, CV_8UC4, 255/(max_1 - min_1), - min_1 * 255.0/(max_1 - min_1));
       
        cv::minMaxLoc(dpc_result_lr.reshape(1), &min_2, &max_2);
        dpc_result_lr.convertTo(dpc_result_lr8, CV_8UC4, 255/(max_2 - min_2), -min_2 * 255.0/(max_2 - min_2));
        
        cv::minMaxLoc(dpc_result_tb.reshape(1), &min_3, &max_3);
        dpc_result_tb.convertTo(dpc_result_tb8, CV_8UC4, 255/(max_3 - min_3), -min_3 * 255.0/(max_3 - min_3));
        
        results[0] = bf_result8;
        results[1] = dpc_result_lr8;
        results[2] = dpc_result_tb8;

}

int main(int argc, char** argv )
{

   /*
   Inputs:
   RefocusMin
   RefocusStep
   RefocusMax
   DatasetRoot
   */
   if (argc < 5)
   {
      cout << "Error: Not enough inputs.\nUSAGE: ./Refocusing zMin zStep zMax DatasetRoot" << endl;
      return 0;
   }else{
      zMin = atof(argv[1]);
      zStep = atof(argv[2]);
      zMax = atof(argv[3]);
      datasetRoot = argv[4];
   }
   
   std::cout << "zMin: " << zMin << std::endl;
   std::cout << "zStep: " << zStep << std::endl;
   std::cout << "zMax: " << zMax << std::endl;
   std::cout << "DatasetRoot: " << datasetRoot << std::endl;
   
   vector<R_image> * imageStack;
   imageStack = new vector<R_image>;
   int16_t imgCount = loadImages(datasetRoot,imageStack);
	cout << "Processing Refocusing..."<<endl;
	Mat results[3];
	
	for (float zDist = zMin; zDist <= zMax; zDist += zStep)
	{
	   cout << "Processing: " << zDist << "um..." <<endl;
	   computeFocusDPC(*imageStack, imgCount, zDist, imageStack->at(0).Image.cols, imageStack->at(0).Image.rows, 0, 0, results);
	   
	   char bfFilename[FILENAME_LENGTH];
	   char dpcLRFilename[FILENAME_LENGTH];
	   char dpcTBFilename[FILENAME_LENGTH];
	   snprintf(bfFilename,sizeof(bfFilename), "./Refocused/BF_%3f.png",zDist);
	   snprintf(dpcLRFilename,sizeof(dpcLRFilename), "./Refocused/DPCLR_%.2f.png",zDist);
	   snprintf(dpcTBFilename,sizeof(dpcTBFilename), "./Refocused/DPCTB_%.2f.png",zDist);
	   
	   imwrite(bfFilename, results[0]);
	   imwrite(dpcLRFilename, results[1]);
	   imwrite(dpcTBFilename, results[2]);
	}
	cout << "Finished!"<<endl;

	namedWindow("Result bf", WINDOW_NORMAL);
	imshow("Result bf", results[0]);
	//imwrite("BF.tiff", results[0]);

	namedWindow("Result lr", WINDOW_NORMAL);
	imshow("Result lr", results[1]);
   //imwrite("DPCLR.tiff", results[1]);

	namedWindow("Result tb", WINDOW_NORMAL);
	imshow("Result tb", results[2]);
	//imwrite("DPCTB.tiff", results[2]);
	waitKey(0);
} 

