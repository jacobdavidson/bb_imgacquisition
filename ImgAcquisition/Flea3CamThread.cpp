#include "Flea3CamThread.h"
#include <cstdlib>
#include <iostream>
#include <time.h>
#include "FlyCapture2.h"

Flea3CamThread::Flea3CamThread()
{
	initialized = false;	
}

Flea3CamThread::~Flea3CamThread()
{

}
//this function reads the data input vector 
bool Flea3CamThread::initialize(unsigned int id)
{
	_ID							= id;	
	_jpegConf.quality			= 90;
	_jpegConf.progressive		= false;

	if ( initCamera() )
		initialized = startCapture();

	return initialized;
}

bool Flea3CamThread::initCamera()
{
	
	Format7Info				fmt7Info;
	
	const Mode				fmt7Mode		= MODE_10;
	const PixelFormat		fmt7PixFmt		= PIXEL_FORMAT_RAW8;		
	
	const float				frameRate		= 4;
	const float				wbalance		= 512;

	Format7ImageSettings	fmt7ImageSettings;

	// SET VIDEO MODE HERE!!!		
	fmt7ImageSettings.mode					= fmt7Mode;
	fmt7ImageSettings.offsetX				= 0;
	fmt7ImageSettings.offsetY				= 0;
	fmt7ImageSettings.width					= 4000;
	fmt7ImageSettings.height				= 3000;
	fmt7ImageSettings.pixelFormat			= fmt7PixFmt;	

	Format7PacketInfo		fmt7PacketInfo;
	
	
	BusManager				busMgr;
	PGRGuid					guid;
	CameraInfo				camInfo;
	unsigned int			snum;
		
	FC2Config				BufferFrame;
	
	EmbeddedImageInfo		EmbeddedInfo;
	
	Property				exposure;
	exposure.type			= AUTO_EXPOSURE;

	Property				shutter;
	shutter.type			= SHUTTER;
	
	Property				frmRate;
	frmRate.type			= FRAME_RATE;

	Property				wBalance;
	wBalance.type			= WHITE_BALANCE;

	
	
	bool					supported;
	
	Error				error;
	

	// Gets the Camera serial number
	if ( !checkReturnCode( busMgr.GetCameraSerialNumberFromIndex( _ID, &snum ) ) )
		return false;

	sendLogMessage (3, "Camera Serial Number " +	QString::number(snum));	// serial number is printed
	
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
	// Modify a couple of parameters and send it back to the camera
	BufferFrame.numBuffers = 200;
	BufferFrame.grabMode = BUFFER_FRAMES;

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

	//-------------------- EXPOSURE STARTS -----------------------------------			
	/*if ( !checkReturnCode( _Camera.GetProperty(&exposure) ) )
		return false;*/

	exposure.onOff				= true;
	exposure.autoManualMode		= true;
		
	if ( !checkReturnCode( _Camera.SetProperty(&exposure) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&exposure) ) )
		return false;

	 sendLogMessage(3, "Exposure Parameter is: " + QString().sprintf("%s and %s", exposure.onOff ? "On":"Off", exposure.autoManualMode ? "Auto":"Manual" ));
	
	//-------------------- EXPOSURE ENDS -----------------------------------
	 
	//-------------------- SHUTTER STARTS -----------------------------------	
	/*if ( !checkReturnCode( _Camera.GetProperty(&shutter) ) )
		return false;*/

	shutter.onOff				= false; 
	shutter.autoManualMode		= true;
	
	if ( !checkReturnCode( _Camera.SetProperty(&shutter) ) )
		return false;

	if ( !checkReturnCode( _Camera.GetProperty(&shutter) ) )
		return false;

	 sendLogMessage(3, "New shutter parameter is: " + QString().sprintf("%s", shutter.autoManualMode ? "Auto":"Manual" ));
	
	//-------------------- SHUTTER ENDS -----------------------------------
	
	//-------------------- FRAME RATE STARTS -----------------------------------
	// Get the info of the image in frmRate
	/*if ( !checkReturnCode( _Camera.GetProperty(&frmRate) ) )
		return false;*/

	//sendLogMessage(3, "Frame rate is" + QString().sprintf("%.2f", frmRate.absValue) + " fps" );
		
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

	//-------------------- WHITE BALANCE STARTS -----------------------------------
	// Get the info of the image in wBalance
	/*if ( !checkReturnCode( _Camera.GetProperty(&wBalance) ) )
		return false;

	sendLogMessage(3, "White Balance parameter is: " + QString().sprintf("%s", wBalance.onOff ? "On":"Off" ) );*/
	//sendLogMessage(3, "White Balance is " + QString().sprintf("%.2f", wBalance.absValue ) );
		
	wBalance.onOff				= false;
	//wBalance.absValue			= wbalance;

	if ( !checkReturnCode( _Camera.SetProperty(&wBalance) ) )
		return false;
	
	if ( !checkReturnCode( _Camera.GetProperty(&wBalance) ) )
		return false;
	
	sendLogMessage(3, "New White Balance parameter is: " + QString().sprintf("%s", wBalance.onOff ? "On":"Off" ) );
	//sendLogMessage(3, "New White Balance is" + QString().sprintf("%.2f", wBalance.absValue ) );

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
	struct	tm * timeinfo;
	char	timeresult[15];
	char	filename[512];

	timeresult[14] = 0;

	int cont = 0;

	//while (1)
	while (cont < 10)
	{
		_Camera.RetrieveBuffer(&_Image);
		_ImInfo = _Image.GetMetadata();		
		_FrameNumber = _ImInfo.embeddedFrameCounter;
		_TimeStamp = _Image.GetTimeStamp();
		timeinfo = localtime(&_TimeStamp.seconds);  //converts the time in seconds to local time
		// string with local time info
		sprintf(timeresult, "%d%.2d%.2d%.2d%.2d%.2d%",
		timeinfo -> tm_year + 1900,
		timeinfo -> tm_mon  + 1,
		timeinfo -> tm_mday,
		timeinfo -> tm_hour,
		timeinfo -> tm_min,
		timeinfo -> tm_sec);


		cont ++;		
		//name of the file
		sprintf (filename, "G:\\images\\Cam_%u\\Cam_%u_%s_%u.jpeg", _ID, _ID, timeresult, _FrameNumber);
		_Image.Save(filename, &_jpegConf);
	}	
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