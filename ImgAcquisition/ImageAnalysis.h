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

    //! File to write the analysis results to
    std::string _Logfile;

    //! Watchdog object. Notify this while the thread is active
    Watchdog    *_Dog;

public:

    //! Buffer which holds the images to analyse
    MutexLinkedList *_Buffer;

    /**
     * @brief A simple constructor. Sets members according to parameters.
     *
     * @param Sets _Logfile
     * @param Sets _Dog
     */
    ImageAnalysis(std::string logfile, Watchdog *dog);

    //! Stub
    virtual ~ImageAnalysis();

    /**
     * @brief runs analysis thread. (Deprecated)
     *
     * Runs the analysis thread while encoding is running.
     * This is deprecated. Use the new project "bb_statistics"
     * for this task. It is using the shared memory interface.
     */
    void run();

    /**
     * @brief Gets the contrast ratio of a Matrix
     *
     * This might be useless for large real images, as
     * the ratio will always be maximum. Pick image sections.
     *
     * @param The input image
     * @return Contrast ratio
     */
    double getContrastRatio(cv::Mat &image);

    /**
     * @brief Gets the variance of an image.
     *
     * @param The image
     * @return The variance
     */
    double getVariance(cv::Mat &image);

    /**
     * @brief Gets the Sum Modulus Difference of an image (SMD).
     *
     * See the following papers for info:
     * Practical issues in pixel-based autofocusing for machine vision
     * Echtzeitfhige Extraktion scharfer Standbilder in der Video-Koloskopie
     *
     * @param The image
     * @return The SMD
     */
    double sumModulusDifference(cv::Mat *image);

    /**
     * @brief A helper function of S_PSM.
     *
     * See paper for detail:
     * Echtzeitfhige Extraktion scharfer Standbilder in der Video-Koloskopie
     * Parameters are as per paper
     */
    double DCT(double k1, double k2, int m, int n, cv::Mat &image);

    /**
     * @brief A helper function of S_PSM.
     *
     * See paper for detail:
     * Echtzeitfhige Extraktion scharfer Standbilder in der Video-Koloskopie
     * Parameters are as per paper
     */
    double SSF(double d);

    /**
     * @brief A helper function of S_PSM.
     *
     * See paper for detail:
     * Echtzeitfhige Extraktion scharfer Standbilder in der Video-Koloskopie
     * Parameters are as per paper
     */
    double STilde(int m, int n,cv::Mat &image);

    /**
     * @brief Calculates the Perceptual Sharpness Metric
     *
     * See paper for detail:
     * Echtzeitfhige Extraktion scharfer Standbilder in der Video-Koloskopie
     *
     * @param The image
     * @return The PSM
     */
    double S_PSM(cv::Mat *image);

    /**
     * @brief Gets a colour histogram. Helper of avgHistDifference
     *
     * Use grayscale images only.
     *
     * @param The image to get the histogram from
     * @param The histogram
     */
    cv::Mat getHist(cv::Mat *M);

    /**
     * @brief Calculates the average difference of colour histograms.
     *
     * @param First image
     * @param Second image
     * @return Avg difference
     */
    double avgHistDifference(cv::Mat reference, cv::Mat measure);

    /**
     * @brief Estimates the noise in an image
     *
     * Applies a median filter and gets the SSD.
     * See thesis for details:
     * Masters Thesis Hauke Moenck
     *
     * @param The image to analyse
     * @return Noise estimate
     */
    double noiseEstimate(cv::Mat image);
};

} /* namespace beeCompress */

#endif /* ImageAnalysis_H_ */
