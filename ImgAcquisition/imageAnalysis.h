/*
 * imageAnalysis.h
 *
 *  Created on: Feb 2, 2016
 *      Author: hauke
 */

#ifndef IMAGEANALYSIS_H_
#define IMAGEANALYSIS_H_
#include <opencv2/opencv.hpp>

namespace beeCompress {


class imageAnalysis {
public:
	imageAnalysis();
	virtual ~imageAnalysis();

	void getContrastRatio(cv::Mat &image);

	double getVariance(cv::Mat &image);

	double sumModulusDifference(cv::Mat *image);

	double DCT(double k1, double k2, int m, int n, cv::Mat &image);

	double SSF(double d);

	double STilde(int m, int n,cv::Mat &image);
	double S_PSM(cv::Mat *image);

	cv::Mat getHist(cv::Mat *M);
	double avgHistDifference(cv::Mat reference, cv::Mat measure);

	double noiseEstimate(cv::Mat image);
};

} /* namespace beeCompress */

#endif /* IMAGEANALYSIS_H_ */
