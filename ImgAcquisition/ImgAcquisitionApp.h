#ifndef IMGACQUISITIONAPP_H
#define IMGACQUISITIONAPP_H

#include <QCoreApplication>
#include "Flea3CamThread.h"
#include "NvEncGlue.h"
using namespace FlyCapture2;

//inherits from QCoreApplication
class ImgAcquisitionApp : public QCoreApplication
{
	Q_OBJECT  //generates the MOC

public:
	ImgAcquisitionApp(int & argc, char ** argv); //constructor
	~ImgAcquisitionApp();						 //destructor

	void				printBuildInfo();			 // Just prints the library's info	
	int					checkCameras();				 // This function checks that at least one camera is connected

private:
	Flea3CamThread		threads[4];					// A vector of the class Flea3CamThread, they are accessed from the constructor
	unsigned int		numCameras;					// Number of detected cameras
	beeCompress::NvEncGlue glue1,glue2;
	
//Slots for the signals sent by Flea3CamThread Class
public slots:
	void			logMessage(int logLevel, QString message);
};

#endif // IMGACQUISITIONAPP_H
