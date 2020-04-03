cmake_minimum_required(VERSION 3.1)

project(bb_imageacquisition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(Boost_USE_STATIC_LIBS ON)
add_definitions( -DBOOST_ALL_NO_LIB )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT")

list(APPEND CMAKE_PREFIX_PATH "/opt/Qt5/5.14.1/gcc_64/")

find_package(Qt5Core REQUIRED) 
find_package(Boost REQUIRED system filesystem program_options date_time)
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED COMPONENTS cuda cudart)

message("CUDA_LIBRARIES is ${CUDA_LIBRARIES}")
message("Boost_LIBRARIES is ${Boost_LIBRARIES}")
message("OpenCV_LIBRARIES is ${OpenCV_LIBRARIES}")
message("Qt5Core_LIBRARIES is ${Qt5Core_LIBRARIES}")

set(CAM_HARDWARE "BASLER")
# set(CAM_HARDWARE "FLEA3")
# set(CAM_HARDWARE "XIMEA")

# This option will use OpenCV to display the captured images live - for debugging purposes.
option(WITH_DEBUG_IMAGE_OUTPUT "Show the captured images live." OFF)

#########################
#Get all the libraries into a variable
#########################
set(LIBS 
    ${CUDA_LIBRARIES} 
    ${Qt5Core_LIBRARIES} 
    ${Boost_LIBRARIES}
    ${OpenCV_LIBRARIES}
		)

#########################
#OS specific configuration
#########################
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	set(INCLUDE_DIRS /usr/local/include /usr/include/flycapture/ /opt/ffmpeg/ffmpeg_build/include/ /opt/XIMEA/include/ /opt/pylon5/include/ )
	set(LIBS -L/usr/local/lib ${LIBS} pthread dl cuda rt )
	message("Configuring for ${CAM_HARDWARE} on Linux...")
else() 				##### this needs to be tested, i will not touch this anytime soon, sorry #####
	if(WITH_FLYCAPTURE)
		set(INCLUDE_DIRS "C:\\Program Files\\Point Grey Research\\FlyCapture2\\include"  )
		set(LIBS ${LIBS}
		"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2.lib"
		"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2_v110.lib"
		)
	endif()
	set(INCLUDE_DIRS ${Boost_INCLUDE_DIRS} )
	set(LIBS ${LIBS}
		"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cudart.lib"
		"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cuda.lib"
		)
	message("Configuring for ${CAM_HARDWARE} on Windows...")
endif()


message("All libs: ${LIBS}")

#########################
#Put everything together
#########################

set(EXE_NAME bb_ImageAcquistion)
#file(GLOB_RECURSE ImgAcquisitionSrc RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h *.hpp)

list(APPEND ImgAcquisitionSrc 
	CamThread.cpp;
	ImageAnalysis.cpp;
	ImgAcquisitionApp.cpp;
	NvEncGlue.cpp;
	SharedMemory.cpp;
	main.cpp;
	writeHandler.cpp;
	Buffer/MutexLinkedList.cpp;
	Buffer/MutexBuffer.h;
	Buffer/MutexLinkedList.h;
	Buffer/Semaphore.h;
	nvenc/NvEncoder.cpp;
	nvenc/NvHWEncoder.cpp;
	nvenc/cudaModuleMgr.cpp;
	nvenc/dynlink_cuda.cpp;
	nvenc/dynlink_nvcuvid.cpp;
	settings/Settings.cpp;
	settings/StringTranslator.cpp;
	settings/stringTools.cpp;
	settings/utility.cpp;
	CamThread.h;
	ImageAnalysis.h;
	ImgAcquisitionApp.h;
	NvEncGlue.h;
	SharedMemory.h;
	Watchdog.h;
	nvenc/NvEncoder.h;
	nvenc/NvHWEncoder.h;
	nvenc/cudaModuleMgr.h;
	nvenc/drvapi_error_string.h;
	nvenc/dynlink_builtin_types.h;
	nvenc/dynlink_cuda.h;
	nvenc/dynlink_cudaGL.h;
	nvenc/dynlink_cuda_cuda.h;
	nvenc/dynlink_cuda_gl.h;
	nvenc/dynlink_cuviddec.h;
	nvenc/dynlink_nvcuvid.h;
	nvenc/exception.h;
	nvenc/helper_cuda.h;
	nvenc/helper_cuda_drvapi.h;
	nvenc/helper_functions.h;
	nvenc/helper_string.h;
	nvenc/helper_timer.h;
	nvenc/nvCPUOPSys.h;
	nvenc/nvEncodeAPI.h;
	nvenc/nvFileIO.h;
	nvenc/nvUtils.h;
	settings/ParamNames.h;
	settings/Settings.h;
	settings/StringTranslator.h;
	settings/stringTools.h;
	settings/utility.h;
	writeHandler.h;
)

# keep this in case we return to GLOB_RECURSE
# first remove all cam specific sources and add the targetted ones again
# LIST(REMOVE_ITEM ImgAcquisitionSrc XimeaCamThread.h XimeaCamThread.cpp Flea3CamThread.h Flea3CamThread.cpp BaslerCamThread.h BaslerCamThread.cpp)

# now add the cam specific sources again and define preprocessor 
# flag used for including respective headers
IF (${CAM_HARDWARE} STREQUAL "BASLER")
	link_directories(/opt/pylon5/lib64)
	set(LIBS ${LIBS} pylonutility pylonbase GCBase_gcc_v3_1_Basler_pylon GenApi_gcc_v3_1_Basler_pylon)
	LIST(APPEND ImgAcquisitionSrc BaslerCamThread.h BaslerCamThread.cpp)
	set(INCLUDE_DIRS ${INCLUDE_DIRS} "/opt/pylon5/include" )
	add_definitions(-DUSE_BASLER)
ENDIF()

IF (${CAM_HARDWARE} STREQUAL "FLEA3")
	set(LIBS ${LIBS} flycapture )
	LIST(APPEND ImgAcquisitionSrc Flea3CamThread.cpp)
	add_definitions(-DUSE_FLEA3)
ENDIF()

IF (${CAM_HARDWARE} STREQUAL "XIMEA")
	set(LIBS ${LIBS} m3api )
	LIST(APPEND ImgAcquisitionSrc XimeaCamThread.cpp)
	set(INCLUDE_DIRS ${INCLUDE_DIRS} "/opt/XIMEA/include" )
	add_definitions(-DUSE_XIMEA)
ENDIF()

include_directories(${INCLUDE_DIRS} ${HALIDE_INCLUDE} ${CUDA_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS} )

message("Sources are ${ImgAcquisitionSrc}")

add_executable(${EXE_NAME} ${ImgAcquisitionSrc} )

target_link_libraries(${EXE_NAME} ${LIBS} )








