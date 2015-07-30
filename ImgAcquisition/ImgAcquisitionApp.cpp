#include "ImgAcquisitionApp.h"
#include <iostream>
#include <QtGui/QKeyEvent>
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

	checkCameras(); // when the number of cameras is insufficient it should interrupt the program

		
	//one connect for each combination signal-slot
	connect(&threads[0],	SIGNAL(logMessage(int, QString)),
			this,			SLOT(logMessage(int, QString)));	
	
	connect(&threads[1],	SIGNAL(logMessage(int, QString)),
			this,			SLOT(logMessage(int, QString)));
	
	connect(&threads[2],	SIGNAL(logMessage(int, QString)),
			this,			SLOT(logMessage(int, QString)));
	
	connect(&threads[3],	SIGNAL(logMessage(int, QString)),
			this,			SLOT(logMessage(int, QString)));

	//the threads are initialized as a private variable of the class ImgAcquisitionApp
	threads[0].initialize( 0 );
	threads[1].initialize( 1 );	
	threads[2].initialize( 2 );
	threads[3].initialize( 3 );
	


	//execute run() function
	threads[0].start();
	threads[1].start();	
	threads[2].start();
	threads[3].start();
	

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
	return 0;
}

// The slot for signals generated from the threads
void ImgAcquisitionApp::logMessage(int prio, QString message)
{
	qDebug() << message ;
}