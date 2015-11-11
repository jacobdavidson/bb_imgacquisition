#ifndef FLEA3CAMTHREAD_H
#define FLEA3CAMTHREAD_H

#include <QThread>
#include "FlyCapture2.h"
#include "MutexRingbuffer.h"
using namespace FlyCapture2;

//inherits from Qthread
class Flea3CamThread : public QThread
{
	Q_OBJECT   //generates the MOC

public:
	Flea3CamThread(); //constructor
	~Flea3CamThread(); //destructor
	bool				initialize(unsigned int id, beeCompress::MutexRingbuffer * pBuffer); //here goes the camera ID (from 0 to 3)
	bool				initialized;
	unsigned int		_ID; //private variable

private:
	bool				initCamera();
	bool				startCapture();
	void				PrintCameraInfo			(CameraInfo* pCamInfo); // Just prints the camera's info
	void				PrintFormat7Capabilities(Format7Info fmt7Info); // We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print this info
	bool				checkReturnCode			(Error error);
	void				sendLogMessage			(int logLevel, QString message);

	void				cleanFolder(QString path, QString message); // function to clean the folder and avoid to run out of memory
	void				generateLog(QString path, QString message); // function to generate log file
	void				localCounter(unsigned int oldTime, unsigned int newTime); // 
		
	JPEGOption			_jpegConf; //compression parameters

	Camera				_Camera; //objects that points to the camera
	Image				_Image; //object to catch the image temporally 
	ImageMetadata		_ImInfo; //object needed to read the frame counter	
	unsigned int		_FrameNumber; //the current frame in Image
	TimeStamp			_TimeStamp; //time stamp from the current frame

	unsigned int		_LocalCounter; //to enumerate each image in a second

	beeCompress::MutexRingbuffer *_Buffer;
	
protected:
	void run(); //this is the function that will be iterated indefinitely

signals:
	void				logMessage			(int logLevel, QString message);
};

#endif // FLEA3CAMTHREAD_H
