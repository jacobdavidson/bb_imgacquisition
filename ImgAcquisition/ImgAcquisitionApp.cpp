#include "ImgAcquisitionApp.h"
#include <iostream>
#include <QDebug>

using namespace FlyCapture2;
using namespace std;


//constructor
//definition of ImgAcquisitionApp, it configures the threads, connects them including the signals(input) to be read and the slots (output)
//the whole process is executed when the object is initialized in main.
ImgAcquisitionApp::ImgAcquisitionApp(int & argc, char ** argv)
: QCoreApplication(argc, argv) //
{

	printBuildInfo();
	int numCameras = 0;
	numCameras = checkCameras(); // when the number of cameras is insufficient it should interrupt the program

	if(numCameras<0){
		exit(2);
	}

	//one connect for each combination signal-slot
	for (int i=0; i<numCameras; i++)
		connect(&threads[i],	SIGNAL(logMessage(int, QString)),
				this,			SLOT(logMessage(int, QString)));

	cout << "Connected " << numCameras << " cameras." << endl;

	//the threads are initialized as a private variable of the class ImgAcquisitionApp
	if(numCameras>=1) threads[0].initialize( 0, (glue1._Buffer1) );
	if(numCameras>=2) threads[1].initialize( 1, (glue2._Buffer1) );
	if(numCameras>=3) threads[2].initialize( 2, (glue1._Buffer2) );
	if(numCameras>=4) threads[3].initialize( 3, (glue2._Buffer2) );

	//Map the ringbuffers to camera id's
	glue1._CamBuffer1=0;
	glue1._CamBuffer2=1;
	glue2._CamBuffer1=2;
	glue2._CamBuffer2=3;

	cout << "Initialized " << numCameras << " cameras." << endl;

	//execute run() function, spawns cam readers
	for (int i=0; i<numCameras; i++)
		threads[i].start();

	cout << "Started " << numCameras << " camera threads." << endl;

	//Start encoder threads
	glue1.start();
	if(numCameras>=2) glue2.start();

	cout << "Started the encoder threads." << endl;
}
//destructor
ImgAcquisitionApp::~ImgAcquisitionApp()
{

}

// Just prints the library's info
void ImgAcquisitionApp::printBuildInfo()
{
	FC2Version fc2Version;
	Utilities::GetLibraryVersion( &fc2Version );

	cout << "FlyCapture2 library version: "
			<< fc2Version.major << "." << fc2Version.minor << "."
			<< fc2Version.type << "." << fc2Version.build << endl << endl;

	cout << "Application build date: " << __DATE__ << ", " << __TIME__ << endl << endl; 
}

// This function checks that at least one camera is connected
int ImgAcquisitionApp::checkCameras()
{	
	BusManager			cc_busMgr;
	Error				error;

	error = cc_busMgr.GetNumOfCameras(&numCameras);
	if (error != PGRERROR_OK)
		return -1;

	qDebug() << "Number of cameras detected: " << numCameras << endl << endl;

	if ( numCameras < 1 )
	{
		cout << "Insufficient number of cameras... press Enter to exit." << endl;
		logMessage(1, "Insufficient number of cameras ...");
		getchar();
		return -1;
	}	
	return numCameras;
}

// The slot for signals generated from the threads
void ImgAcquisitionApp::logMessage(int prio, QString message)
{
	qDebug() << message ;
}
