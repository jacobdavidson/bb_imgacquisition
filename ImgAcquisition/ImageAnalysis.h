/*
 * ImageAnalysis.h
 *
 *  Created on: Feb 2, 2016
 *      Author: hauke
 */

#ifndef ImageAnalysis_H_
#define ImageAnalysis_H_
#include <opencv2/opencv.hpp>
#include <QThread>
#include "Buffer/MutexBuffer.h"
#include "Buffer/MutexLinkedList.h"
#include "Watchdog.h"

namespace beeCompress {


class ImageAnalysis : public QThread
{
	Q_OBJECT   //generates the MOC

	std::string _Logfile;
	Watchdog *_Dog;

public:

	MutexLinkedList *_Buffer;

	ImageAnalysis(std::string logfile, Watchdog *dog);
	virtual ~ImageAnalysis();

	void run();

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

#endif /* ImageAnalysis_H_ */
