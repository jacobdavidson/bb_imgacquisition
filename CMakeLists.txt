cmake_minimum_required(VERSION 3.15)

project(bb_imageacquisition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.65 COMPONENTS system filesystem program_options date_time QUIET)
if(NOT Boost_FOUND)
	set(Boost_USE_STATIC_LIBS OFF)
	find_package(Boost 1.65 COMPONENTS system filesystem program_options date_time REQUIRED QUIET)
endif()

find_package(OpenCV REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/src/cmake)

find_package(FFmpeg COMPONENTS avformat avcodec avutil REQUIRED)

set(EXE_NAME bb_ImageAcquistion)

add_executable(${EXE_NAME}
	src/ConcurrentQueue.h
	src/PlatformAdapter.h
	src/PlatformAdapter.cpp
	src/GrayscaleImage.h
	src/GrayscaleImage.cpp
	src/VideoStream.h
	src/VideoStream.cpp
	src/VideoWriteThread.h
	src/VideoWriteThread.cpp
	src/VideoFileWriter.h
	src/VideoFileWriter.cpp
	src/Watchdog.h
	src/Watchdog.cpp
	src/settings/utility.h
	src/settings/utility.cpp
	src/settings/Settings.h
	src/settings/Settings.cpp
	src/WriteHandler.h
	src/WriteHandler.cpp
	src/CamThread.h
	src/CamThread.cpp
	src/ImgAcquisitionApp.h
	src/ImgAcquisitionApp.cpp
	src/main.cpp
)

target_link_libraries(${EXE_NAME} PRIVATE
	${CMAKE_DL_LIBS}
	Threads::Threads
	fmt
	Qt5::Core
	Boost::headers Boost::system Boost::filesystem Boost::program_options Boost::date_time Boost::disable_autolinking
	FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil
	${OpenCV_LIBRARIES}
)

if(CAMERA_BACKENDS)
	foreach(camera_backend ${CAMERA_BACKENDS})
		if(${camera_backend} STREQUAL Flea3)
			find_package(FlyCapture2 REQUIRED)
		elseif(${camera_backend} STREQUAL XIMEA)
			find_package(XIMEA REQUIRED)
		elseif(${camera_backend} STREQUAL Basler)
			find_package(Pylon5 REQUIRED)
		else()
			message(FATAL_ERROR "No such camera backend: ${camera_backend}")
		endif()
	endforeach()
else()
	find_package(FlyCapture2)
	if(FlyCapture2_FOUND)
		list(APPEND CAMERA_BACKENDS Flea3)
	endif()

	find_package(XIMEA)
	if(XIMEA_FOUND)
		list(APPEND CAMERA_BACKENDS XIMEA)
	endif()

	find_package(Pylon5)
	if(Pylon5_FOUND)
		list(APPEND CAMERA_BACKENDS Basler)
	endif()
endif()

if(Flea3 IN_LIST CAMERA_BACKENDS)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_FLEA3)
	target_sources(${EXE_NAME} PRIVATE src/Flea3CamThread.h src/Flea3CamThread.cpp)

	target_link_libraries(${EXE_NAME} PRIVATE FlyCapture2)
endif()

if(XIMEA IN_LIST CAMERA_BACKENDS)
	target_sources(${EXE_NAME} PRIVATE src/XimeaCamThread.h src/XimeaCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_XIMEA)

	target_link_libraries(${EXE_NAME} PRIVATE XIMEA)
endif()

if(Basler IN_LIST CAMERA_BACKENDS)
	target_sources(${EXE_NAME} PRIVATE src/BaslerCamThread.h src/BaslerCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_BASLER)

	target_link_libraries(${EXE_NAME} PRIVATE Pylon5::Base Pylon5::Utility Pylon5::GenAPI Pylon5::GCBase)
endif()

message(STATUS "Enabled camera backends: ${CAMERA_BACKENDS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(${EXE_NAME} PRIVATE -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer)
	target_compile_options(${EXE_NAME} PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
endif()
