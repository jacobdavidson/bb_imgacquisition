cmake_minimum_required(VERSION 3.15)

project(bb_imageacquisition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

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

set(CAMERA_BACKEND "Basler" CACHE STRING "Select a camera backend")
set_property(CACHE CAMERA_BACKEND PROPERTY STRINGS "Basler" "Flea3" "XIMEA")

message("Configuring for ${CAMERA_BACKEND} on ${CMAKE_SYSTEM_NAME}")

set(EXE_NAME bb_ImageAcquistion)

add_executable(${EXE_NAME}
	src/ConcurrentQueue.h
	src/CamThread.cpp
	src/ImgAcquisitionApp.cpp
	src/VideoWriteThread.cpp
	src/main.cpp
	src/writeHandler.cpp
	src/VideoWriter.cpp
	src/VideoWriter.h
	src/GrayscaleImage.h
	src/GrayscaleImage.cpp
	src/settings/Settings.cpp
	src/settings/utility.cpp
	src/CamThread.h
	src/ImgAcquisitionApp.h
	src/VideoWriteThread.h
	src/Watchdog.h
	src/settings/ParamNames.h
	src/settings/Settings.h
	src/settings/utility.h
	src/writeHandler.h
)

target_link_libraries(${EXE_NAME} PRIVATE
	${CMAKE_DL_LIBS}
	Threads::Threads
	Qt5::Core
	Boost::headers Boost::system Boost::filesystem Boost::program_options Boost::date_time Boost::disable_autolinking
	FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil
	${OpenCV_LIBRARIES}
)

if(${CAMERA_BACKEND} STREQUAL "Basler")
	target_sources(${EXE_NAME} PRIVATE src/BaslerCamThread.h src/BaslerCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_BASLER)

	find_package(Pylon5 REQUIRED)
	target_link_libraries(${EXE_NAME} PRIVATE Pylon5::Base Pylon5::Utility Pylon5::GenAPI Pylon5::GCBase)
endif()

if(${CAMERA_BACKEND} STREQUAL "Flea3")
	target_compile_definitions(${EXE_NAME} PRIVATE USE_FLEA3)
	target_sources(${EXE_NAME} PRIVATE src/Flea3CamThread.h src/Flea3CamThread.cpp)

	find_package(FlyCapture2 REQUIRED)
	target_link_libraries(${EXE_NAME} PRIVATE FlyCapture2)
endif()

if(${CAMERA_BACKEND} STREQUAL "XIMEA")
	target_sources(${EXE_NAME} PRIVATE src/XimeaCamThread.h src/XimeaCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_XIMEA)

	find_package(XIMEA REQUIRED)
	target_link_libraries(${EXE_NAME} PRIVATE XIMEA)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(${EXE_NAME} PRIVATE -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer)
	target_compile_options(${EXE_NAME} PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
endif()
