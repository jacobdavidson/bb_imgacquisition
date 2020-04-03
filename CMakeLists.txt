cmake_minimum_required(VERSION 3.1)

project(bb_imageacquisition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS system filesystem program_options date_time QUIET)
if(NOT Boost_FOUND)
	set(Boost_USE_STATIC_LIBS OFF)
	find_package(Boost COMPONENTS system filesystem program_options date_time REQUIRED QUIET)
endif()

find_package(OpenCV REQUIRED)

set(CAMERA_BACKEND "Basler" CACHE STRING "Select a camera backend")
set_property(CACHE CAMERA_BACKEND PROPERTY STRINGS "Basler" "Flea3" "XIMEA")

# This option will use OpenCV to display the captured images live - for debugging purposes.
option(WITH_DEBUG_IMAGE_OUTPUT "Show the captured images live." OFF)

message("Configuring for ${CAMERA_BACKEND} on ${CMAKE_SYSTEM_NAME}")

set(EXE_NAME bb_ImageAcquistion)

add_executable(${EXE_NAME}
	CamThread.cpp
	ImageAnalysis.cpp
	ImgAcquisitionApp.cpp
	NvEncGlue.cpp
	SharedMemory.cpp
	main.cpp
	writeHandler.cpp
	Buffer/MutexLinkedList.cpp
	Buffer/MutexBuffer.h
	Buffer/MutexLinkedList.h
	Buffer/Semaphore.h
	nvenc/NvEncoder.cpp
	nvenc/NvHWEncoder.cpp
	nvenc/cudaModuleMgr.cpp
	nvenc/dynlink_cuda.cpp
	nvenc/dynlink_nvcuvid.cpp
	settings/Settings.cpp
	settings/StringTranslator.cpp
	settings/stringTools.cpp
	settings/utility.cpp
	CamThread.h
	ImageAnalysis.h
	ImgAcquisitionApp.h
	NvEncGlue.h
	SharedMemory.h
	Watchdog.h
	nvenc/NvEncoder.h
	nvenc/NvHWEncoder.h
	nvenc/cudaModuleMgr.h
	nvenc/drvapi_error_string.h
	nvenc/dynlink_builtin_types.h
	nvenc/dynlink_cuda.h
	nvenc/dynlink_cudaGL.h
	nvenc/dynlink_cuda_cuda.h
	nvenc/dynlink_cuda_gl.h
	nvenc/dynlink_cuviddec.h
	nvenc/dynlink_nvcuvid.h
	nvenc/exception.h
	nvenc/helper_cuda.h
	nvenc/helper_cuda_drvapi.h
	nvenc/helper_functions.h
	nvenc/helper_string.h
	nvenc/helper_timer.h
	nvenc/nvCPUOPSys.h
	nvenc/nvEncodeAPI.h
	nvenc/nvFileIO.h
	nvenc/nvUtils.h
	settings/ParamNames.h
	settings/Settings.h
	settings/StringTranslator.h
	settings/stringTools.h
	settings/utility.h
	writeHandler.h
)

target_link_libraries(${EXE_NAME} PRIVATE
	${CMAKE_DL_LIBS}
	Threads::Threads
	Qt5::Core
	Boost::headers Boost::system Boost::filesystem Boost::program_options Boost::date_time Boost::disable_autolinking
	${OpenCV_LIBRARIES}
)

if(${CAMERA_BACKEND} STREQUAL "Basler")
	target_sources(${EXE_NAME} PRIVATE BaslerCamThread.h BaslerCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_BASLER)
endif()

if(${CAMERA_BACKEND} STREQUAL "Flea3")
	target_compile_definitions(${EXE_NAME} PRIVATE USE_FLEA3)
	target_sources(${EXE_NAME} PRIVATE Flea3CamThread.h Flea3CamThread.cpp)
endif()

if(${CAMERA_BACKEND} STREQUAL "XIMEA")
	target_sources(${EXE_NAME} PRIVATE XimeaCamThread.h XimeaCamThread.cpp)
	target_compile_definitions(${EXE_NAME} PRIVATE USE_XIMEA)
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	if(${CAMERA_BACKEND} STREQUAL "Basler")
		target_include_directories(${EXE_NAME} PRIVATE "/opt/pylon5/include")
		if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
			target_link_directories(${EXE_NAME} PRIVATE /opt/pylon5/lib64)
		else()
			target_link_directories(${EXE_NAME} PRIVATE /opt/pylon5/lib32)
		endif()
		target_link_libraries(${EXE_NAME} PRIVATE pylonutility pylonbase GCBase_gcc_v3_1_Basler_pylon GenApi_gcc_v3_1_Basler_pylon)
	endif()

	if(${CAMERA_BACKEND} STREQUAL "Flea3")
		target_include_directories(${EXE_NAME} PRIVATE "/usr/include/flycapture")
		target_link_libraries(${EXE_NAME} PRIVATE flycapture)
	endif()

	if(${CAMERA_BACKEND} STREQUAL "XIMEA")
		target_include_directories(${EXE_NAME} PRIVATE "/opt/XIMEA/include")
		target_link_libraries(${EXE_NAME} PRIVATE m3api)
	endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
	if(${CAMERA_BACKEND} STREQUAL "Flea3")
	   target_include_directories(${EXE_NAME} PRIVATE "C:\\Program Files\\Point Grey Research\\FlyCapture2\\include")
		target_link_libraries(${EXE_NAME} PRIVATE
			"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2.lib"
			"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2_v110.lib")
	endif()
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(${EXE_NAME} PRIVATE -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer)
	target_compile_options(${EXE_NAME} PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
endif()
