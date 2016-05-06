#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include "settings/ParamNames.h"
#include "Watchdog.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <QDebug>
#include <QDir>
#ifdef LINUX
#include <unistd.h>
#endif
#ifdef WINDOWS
#include <windows.h>
#include <stdint.h>
#endif

using namespace FlyCapture2;
using namespace std;

std::string ImgAcquisitionApp::figureBasename(std::string infile){
	std::ifstream f(infile.c_str());
	std::string line;
	std::string fst="", last="";
	int prevSize = -1;
	int lineCount = 0;

	//Simple check, file not found?
	if (!f)	return "-1";

	while (std::getline(f, line)){
		if (fst.size()==0) fst = line;
		if (last.size()==prevSize || prevSize==-1)last = line;
		lineCount ++;
	}

	if (lineCount < 3){
		std::cout << "<Restoring partial video> File can not be rename: Textfile has insufficient info: "<< infile << std::endl;
		return "-1";
	}
	return fst+"_TO_"+last;
}


void ImgAcquisitionApp::resolveLockDir(std::string from, std::string to){
	//list for all files found in the directory
	QList<QString> files;

	QString qsFrom(from.c_str());
	QDir directory(qsFrom);

	QFileInfoList infoList = directory.entryInfoList( QDir::NoDotAndDotDot | QDir::Files,QDir::Name );
	for ( int i = 0; i < infoList.size(); ++i )
	{
		QFileInfo curFile = infoList.at(i);

		if ( curFile.isFile() && curFile.fileName().contains(".lck") ){
			std::string basename 		= curFile.fileName().left(curFile.fileName().length()-3).toStdString();
			std::string lockfile	 	= from + basename + "lck";
			std::string srcvideofile 	= from + basename + "avi";
			std::string srcframesfile 	= from + basename + "txt";
			std::string dstBasename 	= figureBasename(srcframesfile);
			std::string dstvideofile 	= to + dstBasename + ".avi";
			std::string dstframesfile 	= to + dstBasename + ".txt";

			//Some error happened figuring the filename
			if (dstBasename == "-1") continue;

			rename(srcvideofile.c_str(),dstvideofile.c_str());
			rename(srcframesfile.c_str(),dstframesfile.c_str());

			//Remove the lockfile, so others will be allowed to grab the video
			remove( lockfile.c_str() );
		}
	}
}

void ImgAcquisitionApp::resolveLocks(){
	SettingsIAC *set = SettingsIAC::getInstance();
	std::string imdirSet 			= set->getValueOfParam<std::string>(IMACQUISITION::IMDIR);
	std::string imdirprevSet		= set->getValueOfParam<std::string>(IMACQUISITION::IMDIRPREVIEW);
	std::string exchangedirSet		= set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIR);
	std::string exchangedirprevSet	= set->getValueOfParam<std::string>(IMACQUISITION::EXCHANGEDIRPREVIEW);

	std::size_t found = imdirSet.find_last_of("/\\");
	std::string imdir = imdirSet.substr(0,found)+"/";
	found = exchangedirSet.find_last_of("/\\");
	std::string exchangedir = exchangedirSet.substr(0,found)+"/";

	found = imdirprevSet.find_last_of("/\\");
	std::string imdirprev = imdirprevSet.substr(0,found)+"/";
	found = exchangedirprevSet.find_last_of("/\\");
	std::string exchangedirprev = exchangedirprevSet.substr(0,found)+"/";

	for (int i=0; i<4; i++){
		char src[512];
		char dst[512];
		sprintf(src, imdir.c_str(),i,0);
		sprintf(dst, exchangedir.c_str(),i,0);
		resolveLockDir(src,dst);
		sprintf(src, imdirprev.c_str(),i,0);
		sprintf(dst, exchangedirprev.c_str(),i,0);
		resolveLockDir(src,dst);
	}
}


//constructor
//definition of ImgAcquisitionApp, it configures the threads, connects them including the signals(input) to be read and the slots (output)
//the whole process is executed when the object is initialized in main.
ImgAcquisitionApp::ImgAcquisitionApp(int & argc, char ** argv)
: QCoreApplication(argc, argv) //
{
	CalibrationInfo calib;
	Watchdog dog;
	int numCameras 			= 0;
	int camsStarted 		= 0;
	SettingsIAC *set 		= SettingsIAC::getInstance();
	calib.doCalibration 	= false;			// When calibrating cameras only
	analysis 				= new beeCompress::ImageAnalysis(
			set->getValueOfParam<std::string>(IMACQUISITION::ANALYSISFILE), &dog);


	std::cout << "Successfully parsed config!" << std::endl;

	if (argc>1 && strncmp(argv[1],"--help", 6) == 0){
		std::cout << "Usage: ./bb_imageacquision <Options>" << std::endl <<
				"Valid options: "<< std::endl <<
				"(none) \t\t start recording as per config." << std::endl <<
				"--calibrate \t Camera calibration mode." << std::endl;
		QCoreApplication::exit(0);
		std::exit(0);
	}
	else if (argc>1 && strncmp(argv[1],"--calibrate", 11) == 0){
		calib.doCalibration = true;
	}

	printBuildInfo();
	resolveLocks();
	numCameras = checkCameras(); 	// when the number of cameras is insufficient it should interrupt the program

	if(numCameras<1){
		std::exit(2);
	}

	//one connect for each combination signal-slot
	for (int i=0; i<4; i++)
		connect(&threads[i],	SIGNAL(logMessage(int, QString)),
				this,			SLOT(logMessage(int, QString)));

	cout << "Connected " << numCameras << " cameras." << endl;

	//the threads are initialized as a private variable of the class ImgAcquisitionApp
	threads[0].initialize( 0, (glue1._Buffer1), analysis->_Buffer,&calib, &dog );
	threads[1].initialize( 1, (glue2._Buffer1), analysis->_Buffer,&calib, &dog );
	threads[2].initialize( 2, (glue1._Buffer2), analysis->_Buffer,&calib, &dog );
	threads[3].initialize( 3, (glue2._Buffer2), analysis->_Buffer,&calib, &dog );

	//Map the buffers to camera id's
	glue1._CamBuffer1=0;
	glue1._CamBuffer2=2;
	glue2._CamBuffer1=1;
	glue2._CamBuffer2=3;

	cout << "Initialized " << numCameras << " cameras." << endl;

	//execute run() function, spawns cam readers
	for (int i=0; i<4; i++){
		if (threads[i]._initialized){
			threads[i].start();
			camsStarted ++;
		}
	}

	cout << "Started " << camsStarted << " camera threads." << endl;

	//Start encoder threads
	//The if(numCameras>=2) is not required here. If only one camera is present,
	//the other glue thread will sleep most of the time.
	glue1.start();
	glue2.start();

	cout << "Started the encoder threads." << endl;

	//While normal recording, start analysis thread to
	//log image statistics
	if (!calib.doCalibration){
		analysis->start();
		std::cout << "Started analysis thread." << endl;
	}

	//Do output for calibration process
	while (calib.doCalibration){
#ifdef __linux__
		system("clear");
#else
		system("cls");
#endif
		cout << "*************************************" << std::endl<<
				"*** Doing camera calibration only ***" << std::endl<<
				"*************************************" << std::endl;
		printf("CamId,\tSMD,\tVar,\tCont,\tNoise\n");
		calib.dataAccess.lock();
		for (int i=0; i<4; i++)
			printf("Cam %d: %f,\t%f,\t%f,\t%f,\t%f\n",i,
					calib.calibrationData[i][0],calib.calibrationData[i][1],
					calib.calibrationData[i][2],calib.calibrationData[i][3]);

		calib.dataAccess.unlock();
#ifdef __linux__
	usleep(500*1000);
#else
    Sleep(500*1000);
#endif
	}
	while (true){
		dog.check();
		#ifdef __linux__
			usleep(500*1000);
		#else
			Sleep(500*1000);
		#endif
		
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

	//Find hardware ID to serial number
	for (int i = 0; i < 4; i++){
		unsigned int serial;
		Error error = cc_busMgr.GetCameraSerialNumberFromIndex( i, &serial );
		if (error == PGRERROR_OK){
			qDebug() << "Detected cam with serial: " << serial;
		}
	}

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
