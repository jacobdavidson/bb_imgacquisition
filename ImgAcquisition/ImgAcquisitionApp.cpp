#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include <iostream>
#include <QDebug>
#include <unistd.h> //sleep

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

	//Map the buffers to camera id's
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

	/***** Check memory usage *****/
	//Grab video dimensions from settings
	SettingsIAC *set = SettingsIAC::getInstance();
	long long unsigned int prevDim = set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH)/2
									* set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT)/2;
	long long unsigned int hqDim = set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH)
								   * set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT);

	//Grab current memory usage of each buffer every 10 seconds and calculate total usage
	while(1){
		long long unsigned int usage[8];
		long long unsigned int scale = 1024*1024;
		usage[0] = (glue1._Buffer1->size() * hqDim)  / scale;
		usage[1] = (glue1._Buffer2->size() * hqDim) / scale;
		usage[2] = (glue1._Buffer1_preview->size() * prevDim) / scale;
		usage[3] = (glue1._Buffer2_preview->size() * prevDim) / scale;
		usage[4] = (glue2._Buffer1->size() * hqDim) / scale;
		usage[5] = (glue2._Buffer2->size() * hqDim) / scale;
		usage[6] = (glue2._Buffer1_preview->size() * prevDim) / scale;
		usage[7] = (glue2._Buffer2_preview->size() * prevDim) / scale;
		long long unsigned int total = 0;
		for (int i=0; i<8; i++) total += usage[i];
		std::cout << "Currently using memory: "<< total<< " MB" << std::endl;
		usleep(10000*1000);
	}
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
