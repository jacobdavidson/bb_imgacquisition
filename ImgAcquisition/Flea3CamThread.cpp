#include "Flea3CamThread.h"
#include <cstdlib>
#include <iostream>
#include <time.h>

#include <qdir.h>
#include <qtextstream.h>

#include "FlyCapture2.h"
#if HAVE_UNISTD_H
#include <unistd.h> //sleep
#else
#include <stdint.h>
#endif
#include "settings/Settings.h"

//Flea3CamThread constructor
Flea3CamThread::Flea3CamThread()
{
	initialized = false;	
}

//Flea3CamThread constructor
Flea3CamThread::~Flea3CamThread()
{

}

//this function reads the data input vector 
bool Flea3CamThread::initialize(unsigned int id, beeCompress::MutexRingbuffer * pBuffer)
{
	_Buffer 					= pBuffer;
	_ID							= id;	
	_jpegConf.quality			= 90;
	_jpegConf.progressive		= false;

	if ( initCamera() ){
		std::cout << "Starting capture on camera "<< id << std::endl;
		initialized = startCapture();
		std::cout << "Done starting capture." << std::endl;
	}
	else{
		std::cerr << "Error on camera init "<< id << std::endl;
	}

	return initialized;
}

bool Flea3CamThread::initCamera()
{

	SettingsIAC 	*set = SettingsIAC::getInstance();

	// SET VIDEO MODE HERE!!!
	Format7Info				fmt7Info;

	const Mode				fmt7Mode		= MODE_10;
	const PixelFormat		fmt7PixFmt		= PIXEL_FORMAT_RAW8;		

	const float				frameRate		= set->getValueOfParam<int>(IMACQUISITION::FPS);

	Format7ImageSettings	fmt7ImageSettings;

	fmt7ImageSettings.mode					= fmt7Mode;
	fmt7ImageSettings.offsetX				= 0;
	fmt7ImageSettings.offsetY				= 0;
	fmt7ImageSettings.width					= set->getValueOfParam<int>(IMACQUISITION::VIDEO_WIDTH);
	fmt7ImageSettings.height				= set->getValueOfParam<int>(IMACQUISITION::VIDEO_HEIGHT);
	fmt7ImageSettings.pixelFormat			= fmt7PixFmt;	

	Format7PacketInfo		fmt7PacketInfo;
	//fmt7PacketInfo.recommendedBytesPerPacket = 5040;

	BusManager				busMgr;
	PGRGuid					guid;
	CameraInfo				camInfo;
	unsigned int			snum;

	FC2Config				BufferFrame;

	EmbeddedImageInfo		EmbeddedInfo;

	// Properties to be modified

	Property				brightness;
	brightness.type			= BRIGHTNESS;

	Property				exposure;
	exposure.type			= AUTO_EXPOSURE;

	Property				shutter;
	shutter.type			= SHUTTER;

	Property				frmRate;
	frmRate.type			= FRAME_RATE;	

	Property				gain;
	gain.type				= GAIN;

	Property				wBalance;
	wBalance.type			= WHITE_BALANCE;

	bool					supported;

	Error					error;


	// Gets the Camera serial number
	if ( !checkReturnCode( busMgr.GetCameraSerialNumberFromIndex( _ID, &snum ) ) )
		return false;

	sendLogMessage (3, "Camera " + QString::number(_ID) + " Serial Number: " + QString::number(snum));	// serial number is printed

	// Gets the PGRGuid from the camera		
	if ( !checkReturnCode( busMgr.GetCameraFromIndex( _ID, &guid ) ) )
		return false;	

	// Connect to camera
	if ( !checkReturnCode( _Camera.Connect(&guid) ) )
		return false;

	// Get the camera information		
	if ( !checkReturnCode( _Camera.GetCameraInfo(&camInfo) ) )
		return false;

	PrintCameraInfo(&camInfo);  // camera information is printed

	/////////////////// ALL THE PROCESS WITH FORMAT 7 ////////////////////////////////

	// Query for available Format 7 modes						
	if ( !checkReturnCode( _Camera.GetFormat7Info(&fmt7Info, &supported) ) )
		return false;

	// Print the camera capabilities for fmt7
	PrintFormat7Capabilities(fmt7Info);

	// Validate the settings to make sure that they are valid
	if ( !checkReturnCode( _Camera.ValidateFormat7Settings( &fmt7ImageSettings, &supported, &fmt7PacketInfo ) ) )
		return false;

	if (!supported)
	{
		// Settings are not valid
		sendLogMessage(1, "Format7 settings are not valid");
		return false;
	}

	// Set the settings to the camera
	if ( !checkReturnCode( _Camera.SetFormat7Configuration(&fmt7ImageSettings, fmt7PacketInfo.recommendedBytesPerPacket) ) )
		return false;

	/////////////////////////////// ENDS PROCESS WITH FORMAT 7 //////////////////////////////////////////

	/////////////////// ALL THE PROCESS WITH FC2Config  ////////////////////////////////

	// Grab the current configuration from the camera in ss_BufferFrame
	if ( !checkReturnCode( _Camera.GetConfiguration(&BufferFrame) ) )
		return false;
	// Modify the maximum number of frames to be buffered and send it back to the camera
	BufferFrame.numBuffers = 20;
	BufferFrame.grabMode = BUFFER_FRAMES;

	//TODO: Re-enable this, if it happens to work some day
	// Exits if the configuration is not supported
	if ( !checkReturnCode( _Camera.SetConfiguration(&BufferFrame) ) )
		return false;

	/////////////////// ENDS PROCESS WITH FC2Config  ////////////////////////////////

	/////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

	// Get the info of the image in ss_EmbeddedInfo
	if ( !checkReturnCode( _Camera.GetEmbeddedImageInfo(&EmbeddedInfo) ) )
		return false;

	// Again modify a couple of parameters and send them back to the camera
	if (EmbeddedInfo.timestamp.available == true)
	{
		// if possible activates timestamp
		EmbeddedInfo.timestamp.onOff = true;				
	}
	else
	{
		sendLogMessage(3, "Timestamp is not available!" ); 
	}

	if (EmbeddedInfo.frameCounter.available == true)
	{
		// if possible activates frame counter
		EmbeddedInfo.frameCounter.onOff = true;
	}
	else
	{
		sendLogMessage(3, "Framecounter is not avalable!" );	
	}

	if ( !checkReturnCode( _Camera.SetEmbeddedImageInfo(&EmbeddedInfo) ) )
		return false;

	/////////////////// ENDS PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////

	/////////////////// ALL THE PROCESS WITH PROPERTIES  ////////////////////////////////

	//-------------------- BRIGHTNESS STARTS -----------------------------------			
	if ( !checkReturnCode( _Camera.GetProperty(&brightness) ) )
		return false;

	brightness.onOff				= true;
	brightness.autoManualMode		= false;
	brightness.absValue				= 0;

	if ( !checkReturnCode( _Camera.SetProperty(&brightness) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&brightness) ) )
		return false;

	sendLogMessage(3, "Brightness Parameter is: " + QString().sprintf("%s and %.2f", brightness.onOff ? "On":"Off", brightness.absValue ));

	//-------------------- BROGHTNESS ENDS -----------------------------------

	//-------------------- EXPOSURE STARTS -----------------------------------			
	if ( !checkReturnCode( _Camera.GetProperty(&exposure) ) )
		return false;

	exposure.onOff				= false;
	exposure.autoManualMode		= true;

	if ( !checkReturnCode( _Camera.SetProperty(&exposure) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&exposure) ) )
		return false;

	sendLogMessage(3, "Exposure Parameter is: " + QString().sprintf("%s and %s", exposure.onOff ? "On":"Off", exposure.autoManualMode ? "Auto":"Manual" ));

	//-------------------- EXPOSURE ENDS -----------------------------------

	//-------------------- SHUTTER STARTS -----------------------------------	
	if ( !checkReturnCode( _Camera.GetProperty(&shutter) ) )
		return false;

	shutter.onOff				= true; 
	shutter.autoManualMode		= false;
	shutter.absValue			= 120; //40 for original

	if ( !checkReturnCode( _Camera.SetProperty(&shutter) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&shutter) ) )
		return false;

	sendLogMessage(3, "New shutter parameter is: " + QString().sprintf("%s and %.2f ms", shutter.autoManualMode ? "Auto":"Manual", shutter.absValue ));	
	//-------------------- SHUTTER ENDS -----------------------------------

	//-------------------- FRAME RATE STARTS -----------------------------------		
	if ( !checkReturnCode( _Camera.GetProperty(&frmRate) ) )
		return false;

	frmRate.absControl			= true;
	frmRate.onOff				= true;
	frmRate.autoManualMode		= false;
	frmRate.absValue			= frameRate;

	if ( !checkReturnCode( _Camera.SetProperty(&frmRate) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&frmRate) ) )
		return false;

	sendLogMessage(3, "New frame rate is " + QString().sprintf("%.2f", frmRate.absValue ) + " fps" );

	//-------------------- FRAME RATE ENDS -----------------------------------

	//-------------------- GAIN STARTS -----------------------------------
	if ( !checkReturnCode( _Camera.GetProperty(&gain) ) )
		return false;

	gain.onOff				= true;
	gain.autoManualMode		= false;
	gain.absValue			= 20; //Original 0

	if ( !checkReturnCode( _Camera.SetProperty(&gain) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&gain) ) )
		return false;

	sendLogMessage(3, "New gain parameter is: " + QString().sprintf("%s", gain.autoManualMode ? "Auto":"Manual" ) );	
	//-------------------- GAIN ENDS -----------------------------------

	//-------------------- WHITE BALANCE STARTS -----------------------------------		
	if ( !checkReturnCode( _Camera.GetProperty(&wBalance) ) )
		return false;

	wBalance.onOff				= false;

	if ( !checkReturnCode( _Camera.SetProperty(&wBalance) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&wBalance) ) )
		return false;

	sendLogMessage(3, "New White Balance parameter is: " + QString().sprintf("%s", wBalance.onOff ? "On":"Off" ) );	
	//-------------------- WHITE BALANCE ENDS -----------------------------------

	/////////////////// ALL THE PROCESS WITH EMBEDDEDIMAGEINFO  ////////////////////////////////
	//Saves the configuration to memory channel 2
	if ( !checkReturnCode( _Camera.SaveToMemoryChannel(2) ) )
		return false;

	return true;
}

// This function starts the streaming from the camera
bool Flea3CamThread::startCapture()
{
	// Start isochronous image capture	
	return checkReturnCode( _Camera.StartCapture() );
}

//this is what the function does with the information set in configure
void Flea3CamThread::run()
{
	struct			tm * timeinfo;
	time_t rawtime;
	char			timeresult[15];
	char			logfilepathFull[256];
	SettingsIAC 	*set = SettingsIAC::getInstance();
	std::string 	logdir = set->getValueOfParam<std::string>(IMACQUISITION::LOGDIR);

	timeresult[14] = 0;
	/////////////////////////////////////////////

	BusManager				busMgr;
	PGRGuid					guid;
	////////////////////////////////////////////
	int				cont = 0;


	while (1)
	{
		FlyCapture2::Image* cimg = _Buffer->front();

		_Camera.RetrieveBuffer(cimg);
		//_ImInfo = cimg->GetMetadata();
		//_FrameNumber = _ImInfo.embeddedFrameCounter;
		//_TimeStamp = cimg->GetTimeStamp();

		_Buffer->push();

		//TODO Fix, use camera time, not own
		time (&rawtime);
		timeinfo = localtime (&rawtime);
		//timeinfo = localtime(&secs);  //converts the time in seconds to local time
		// string with local time info
		sprintf(timeresult, "%d%.2d%.2d%.2d%.2d%.2d%",
				timeinfo -> tm_year + 1900,
				timeinfo -> tm_mon  + 1,
				timeinfo -> tm_mday,
				timeinfo -> tm_hour,
				timeinfo -> tm_min,
				timeinfo -> tm_sec);

		//localCounter(oldTime, timeinfo -> tm_sec);

		sprintf(logfilepathFull, logdir.c_str(),_ID);
		generateLog(logfilepathFull,timeresult);

		//Do this every second
			_Camera.StopCapture();
			_Camera.Disconnect();
			// Gets the PGRGuid from the camera
			busMgr.GetCameraFromIndex( _ID, &guid );
			// Connect to camera
			_Camera.Connect(&guid);
			_Camera.StartCapture();


	}	

	_Camera.StopCapture();
	_Camera.Disconnect();

	std::cout << "done" << std::endl;
	return;
}

// We will use Format7 to set the video parameters instead of DCAM, so it becomes handy to print this info
void Flea3CamThread::PrintFormat7Capabilities(Format7Info fmt7Info)
{    
	sendLogMessage(3,  "Max image pixels: " + QString::number(fmt7Info.maxWidth) + " x " + QString::number(fmt7Info.maxHeight) + "\n" 
			+	"Image Unit size: " + QString::number(fmt7Info.imageHStepSize) + " x " + QString::number(fmt7Info.imageVStepSize) + "\n"
			+	"Offset Unit size: " + QString::number(fmt7Info.offsetHStepSize) + " x " + QString::number(fmt7Info.offsetVStepSize) + "\n"
			+	"Pixel format bitfield: " + QString::number(fmt7Info.pixelFormatBitField));
}

// Just prints the camera's info
void Flea3CamThread::PrintCameraInfo(CameraInfo* pCamInfo)
{
	sendLogMessage(3,  QString() + "\n*** CAMERA INFORMATION ***\n" + "Serial number - " + QString::number( pCamInfo->serialNumber ) + "\n" 
			+ "Camera model - " +	QString( pCamInfo->modelName ) + "\n"
			+ "Camera vendor - " +	QString( pCamInfo->vendorName ) + "\n"
			+ "Sensor - " +			QString( pCamInfo->sensorInfo ) + "\n"
			+ "Resolution - " +		QString( pCamInfo->sensorResolution ) + "\n"
			+ "Firmware version - " + QString( pCamInfo->firmwareVersion ) + "\n"
			+ "Firmware build time - " + QString( pCamInfo->firmwareBuildTime ) + "\n"+ "\n");
}

bool Flea3CamThread::checkReturnCode(Error error)
{
	if (error != PGRERROR_OK)
	{
		sendLogMessage(1, "Cam " + QString::number(_ID) + " : " + error.GetDescription());	
		return false;
	}
	return true;
}

void Flea3CamThread::sendLogMessage(int logLevel, QString message)
{
	emit logMessage(logLevel, "Cam " + QString::number(_ID) + " : " + message);
}

void Flea3CamThread::cleanFolder(QString path, QString message)
{
	QDir dir(path);
	dir.setNameFilters(QStringList() << "*.jpeg");
	dir.setFilter(QDir::Files);
	foreach(QString dirFile, dir.entryList())
	{
		dir.remove(dirFile);
	}
	QString filename = (path + "log.txt");
	QFile file(filename);
	file.open(QIODevice::Append);
	QTextStream stream(&file);
	stream << message <<"\r\n";
	file.close();	
}

void Flea3CamThread::generateLog(QString path, QString message)
{	
	QString filename = (path + "log" + message.left(8) + ".txt");
	QFile file(filename);
	file.open(QIODevice::Append);
	QTextStream stream(&file);
	stream << message <<"\r\n";
	file.close();	
}

void Flea3CamThread::localCounter(unsigned int oldTime, unsigned int newTime)
{
	if (oldTime != newTime)
		_LocalCounter=0;
	_LocalCounter++;	
}
