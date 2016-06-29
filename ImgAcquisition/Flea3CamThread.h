#ifndef FLEA3CAMTHREAD_H
#define FLEA3CAMTHREAD_H

#ifdef WINDOWS
#include <windows.h>
#endif
#include <QThread>
#include "FlyCapture2.h"
#include "Buffer/MutexBuffer.h"
#include "Watchdog.h"
#include <mutex>
using namespace FlyCapture2;

struct CalibrationInfo{
	bool doCalibration;
	double calibrationData[4][4];
	std::mutex dataAccess;
};

/*!\brief Thread object which acquires images from a camera.
 *
 * Inherits from Qthread for threading.
 * Contains functions to initialize the cameras and run the acquistion.
 */
class Flea3CamThread : public QThread
{
	Q_OBJECT   //generates the MOC

public:
	Flea3CamThread(); //constructor
	~Flea3CamThread(); //destructor

	/*!\brief Initialization of cameras and configuration
	 *
	 * \param Virtual ID of the camera (0 to 3)
	 * \param Buffer shared with the encoder thread
	 * \param Buffer shared with the shared memory thread
	 * \param Pointer to calibration data storeage
	 * \param Watchdog to notifiy each acquisition loop (when running)
	 */
	bool				initialize(unsigned int id, beeCompress::MutexBuffer * pBuffer,
								beeCompress::MutexBuffer * pSharedMemBuffer, CalibrationInfo *calib, Watchdog *dog); //here goes the camera ID (from 0 to 3)

	//! Object has been initialized using "initialize"
	bool				_initialized;

	//! Virtual ID of the camera
	unsigned int		_ID;

	//! Hardware ID (id in bus order) of the camera. (used internally)
	unsigned int		_HWID;

	//! Serial number of the camera
	unsigned int		_Serial;

	//! Pointer to calibration data storeage (set by initialize)
	CalibrationInfo		*_Calibration;

	//! Watchdog to notifiy each acquisition loop (set by initialize)
	Watchdog	 		*_Dog;

private:
	bool				initCamera();
	bool				startCapture();

	//! \brief Just prints the camera's info
	void				PrintCameraInfo			(CameraInfo* pCamInfo);
	void				PrintFormat7Capabilities(Format7Info fmt7Info); // We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print this info
	bool				checkReturnCode			(Error error);
	void				sendLogMessage			(int logLevel, QString message);

	void				cleanFolder				(QString path, QString message); // function to clean the folder and avoid to run out of memory
	void				generateLog				(QString path, QString message); // function to generate log file
	void				localCounter			(unsigned int oldTime, unsigned int newTime); //
	void 				logCriticalError		(Error e);
		
	//JPEGOption			_jpegConf; //compression parameters

	Camera				_Camera; //objects that points to the camera
	Image				_Image; //object to catch the image temporally 
	ImageMetadata		_ImInfo; //object needed to read the frame counter	
	unsigned int		_FrameNumber; //the current frame in Image
	TimeStamp			_TimeStamp; //time stamp from the current frame

	unsigned int		_LocalCounter; //to enumerate each image in a second

	//! Buffer shared with the encoder thread
	beeCompress::MutexBuffer *_Buffer;

	//! Buffer shared with the shared memory thread
	beeCompress::MutexBuffer *_SharedMemBuffer;
	
protected:
	void run(); //this is the function that will be iterated indefinitely

signals:
	void				logMessage			(int logLevel, QString message);
};

#endif // FLEA3CAMTHREAD_H
