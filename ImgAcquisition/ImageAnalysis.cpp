/*
 * ImageAnalysis.cpp
 *
 *  Created on: Feb 2, 2016
 *      Author: hauke
 */

#include "ImageAnalysis.h"
#include <math.h>       /* cos */
#include <vector>
#include <algorithm>
#include <iostream>
#define PI 3.14159265

namespace beeCompress {
using namespace cv;

ImageAnalysis::ImageAnalysis(std::string p_logfile) {
	_Logfile = p_logfile;
	_Buffer = new beeCompress::MutexLinkedList();
}

ImageAnalysis::~ImageAnalysis() {
	// TODO Auto-generated destructor stub
}

void ImageAnalysis::run() {
	cv::Mat ref;
	char 	outstr[512];
	FILE*	outfile = fopen(_Logfile.c_str(),"wb");
	FILE* 	fp 		= fopen("refIm.jpg", "r");
	if (fp) {
		ref = cv::imread( "refIm.jpg", CV_LOAD_IMAGE_GRAYSCALE );
		fclose(fp);
	} else {
		std::cout << "Error: not found reference image refIm.jpg."<<std::endl;
	}

	while(true){
		std::shared_ptr<beeCompress::ImageBuffer> imgptr = _Buffer->pop();
		beeCompress::ImageBuffer *img = imgptr.get();
		cv::Mat mat(img->height,img->width,cv::DataType<uint8_t>::type);
		mat.data = img->data;
		double smd = sumModulusDifference(&mat);
		double variance = getVariance(mat);
		double contrast = avgHistDifference(ref,mat);
		double noise = noiseEstimate(mat);
		sprintf(outstr,"Cam %d: %f,\t%f,\t%f,\t%f,\t%f\n",img->camid,smd,variance,contrast,noise);
		fwrite(outstr,sizeof(char), strlen(outstr),outfile);
		fflush(outfile);
		std::cout << "ASDFASDF"<<std::endl;
	}

	//Well, just in case...
	fclose(outfile);
}

void ImageAnalysis::getContrastRatio(Mat &image){
	uint8_t min = 255;
	uint8_t max = 0;
	for( int y = 0; y < image.rows; y++ ){
		for( int x = 0; x < image.cols; x++ ){
			uint8_t val = image.at<uint8_t>(y, x);
			val < min ? min = val : 0 ;
			val > max ? max = val : 0 ;
		}
	}
	double ratio = ((double)min)/((double)max);
	//printf("%d  %d, ratio: %f\n",max,min,ratio);
}

double ImageAnalysis::getVariance(Mat &image){
	//http://www.lfb.rwth-aachen.de/bibtexupload/pdf/GRO10a.pdf
	Mat out(image.size(),cv::DataType<double>::type);
	Mat squared(image.size(),cv::DataType<double>::type);
	Mat in(image.size(),cv::DataType<double>::type);
	image.assignTo(in,CV_64F);

	double s = cv::sum( in )[0];
	double frac = 1.0 / ((double)(in.rows * in.cols));
	double sfrac = -1.0 * s*frac;
	add(in,sfrac,out);
	//square: http://docs.opencv.org/2.4/modules/core/doc/operations_on_arrays.html
	//multiply - Calculates the per-element scaled product of two arrays.
	multiply(out,out,squared);
	double var = frac * cv::sum( squared )[0];

	//printf("Variance: %f\n",var);
	return var;
}

double ImageAnalysis::sumModulusDifference(Mat *image){
	//http://www.lfb.rwth-aachen.de/bibtexupload/pdf/GRO10a.pdf
	double smd = 0.0;
	Mat in(image->size(),cv::DataType<double>::type);
	Mat out1(image->size(),cv::DataType<double>::type);
	Mat out2(image->size(),cv::DataType<double>::type);
	Mat res(image->size(),cv::DataType<double>::type);
	image->assignTo(in,CV_64F);
	Mat vkernel = Mat::ones( 3, 1, CV_32F )/ (double)3.0;
	Mat hkernel = Mat::ones( 1, 3, CV_32F )/ (double)3.0;

	vkernel.at<double>(0, 0) = -1;
	vkernel.at<double>(1, 0) = 1;
	vkernel.at<double>(2, 0) = 0;
	hkernel.at<double>(0, 0) = -1;
	hkernel.at<double>(0, 1) = 1;
	hkernel.at<double>(0, 2) = 0;
	Point anchor = Point( -1, -1 ); //center
	double delta = 0;
	int ddepth = -1;
	filter2D(in, out1, ddepth , hkernel, anchor, delta, BORDER_DEFAULT );
	filter2D(in, out2, ddepth , vkernel, anchor, delta, BORDER_DEFAULT );
	out1 = abs(out1);
	out2 = abs(out2);
	add(out1,out2,res);
	smd = (cv::sum( res )[0])/(double)(image->rows * image->cols);

	//printf("Sum modulus difference: %f\n",smd);
	return smd;
}

double ImageAnalysis::DCT(double k1, double k2, int m, int n, Mat &image){
	double sum = 0.0;
	for (double i=0 ; i < 8; i++){
		for (double j=0 ; j < 8; j++){
			sum += image.at<double>(i+m, j+n)*cos(PI/8.0 * (i+0.5)*k1) * cos(PI/8.0 * (j+0.5)*k2);
		}
	}
	return sum;
}

double ImageAnalysis::SSF(double d){
	return (pow(d, 0.269) * (-3.533+3.533*d) * exp(-0.548*d));
}

double ImageAnalysis::STilde(int m, int n,Mat &image){
	double sum = 0.0;
	double Bn_ijOneOne = image.at<double>(m+1, n+1);
	for (int d=0 ; d < 8; d++){
		double ssf = SSF(d);// / Bn_ijOneOne;
		sum += ssf*(DCT(0,d-1,m,n,image)+ DCT(d-1,0,m,n,image));
	}
	return sum;
}

double ImageAnalysis::S_PSM(Mat *image){
	Mat in(image->size(),cv::DataType<double>::type);
	image->assignTo(in,CV_64F);

	double sum = 0.0;
	for (int i=0 ; i < image->rows-8; i+=8){
		for (int j=0 ; j < image->cols-8; j+=8){
			double st = STilde(i,j,in);
			sum += st;
			//printf("st: %lf\n",st);
		}
	}
	double frac = 1.0 / ((double)(in.rows * in.cols));
	return frac * sum;
}

cv::Mat ImageAnalysis::getHist(cv::Mat *M){

	  /// Establish the number of bins
	  int histSize = 256;

	  /// Set the ranges ( for B,G,R) )
	  float range[] = { 0, 256 } ;
	  const float* histRange = { range };

	  bool uniform = true;
	  bool accumulate = false;

	  Mat grey_hist;

	  /// Compute the histogram:
	  calcHist( M, 1, 0, Mat(), grey_hist, 1, &histSize, &histRange, uniform, accumulate );

	  // Draw the histograms for B, G and R
	  int hist_w = 512;
	  int hist_h = 400;
	  int bin_w = cvRound( (double) hist_w/histSize );

	  Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );

	  /// Normalize the result to [ 0, histImage.rows ]
	  //normalize(grey_hist, grey_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	  return grey_hist;
}

double ImageAnalysis::avgHistDifference(Mat reference, Mat measure){
	Mat ideal = getHist(&reference);
	Mat noisy = getHist(&measure);
	Mat dif(noisy.size(),cv::DataType<double>::type);
	subtract(ideal,noisy,dif);
	double total = cv::sum(cv::abs(dif))[0] / 256.0; //TODO SSD!?
	//std::cout << total << std::endl;
	return total;
}

double ImageAnalysis::noiseEstimate(Mat image){
	Mat blur(image.size(),cv::DataType<double>::type);
	Mat dif(image.size(),cv::DataType<double>::type);
	cv::medianBlur(image,blur,5);
	subtract(image,blur,dif);
	double total = cv::sum(cv::abs(dif))[0] / (image.rows * image.cols); //TODO SSD!?
	//std::cout << total << std::endl;
	return total;
}

} /* namespace beeCompress */
