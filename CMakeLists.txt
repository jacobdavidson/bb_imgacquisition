cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/src/cmake)

include(GetGitRevisionDescription)
git_describe(SOURCE_VERSION --always --exclude *)
git_local_changes(GIT_DIRTY)
if("${GIT_DIRTY}" STREQUAL DIRTY)
	string(APPEND SOURCE_VERSION "-dirty")
endif()

message(STATUS "Source version: ${SOURCE_VERSION}")

# string(TIMESTAMP BUILD_TIME "%Y-%m-%dT%H:%M:%SZ" UTC)
# message(STATUS "Source version: ${SOURCE_VERSION}")

project(bb_imgacquisition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(fmt 6 REQUIRED)

find_package(Qt5 COMPONENTS Core REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.65 COMPONENTS system filesystem program_options date_time QUIET)
if(NOT Boost_FOUND)
	set(Boost_USE_STATIC_LIBS OFF)
	find_package(Boost 1.65 COMPONENTS system filesystem program_options date_time REQUIRED QUIET)
endif()

find_package(OpenCV REQUIRED)

find_package(FFmpeg COMPONENTS avformat avcodec avutil REQUIRED)

add_executable(${PROJECT_NAME}
	src/format.h
	src/log.h
	src/log.cpp
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
	src/settings/Settings.h
	src/settings/Settings.cpp
	src/CamThread.h
	src/CamThread.cpp
	src/ImgAcquisitionApp.h
	src/ImgAcquisitionApp.cpp
	src/main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME} PRIVATE
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
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FLEA3)
	target_sources(${PROJECT_NAME} PRIVATE src/Flea3CamThread.h src/Flea3CamThread.cpp)

	target_link_libraries(${PROJECT_NAME} PRIVATE FlyCapture2)
endif()

if(XIMEA IN_LIST CAMERA_BACKENDS)
	target_sources(${PROJECT_NAME} PRIVATE src/XimeaCamThread.h src/XimeaCamThread.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_XIMEA)

	target_link_libraries(${PROJECT_NAME} PRIVATE XIMEA)
endif()

if(Basler IN_LIST CAMERA_BACKENDS)
	target_sources(${PROJECT_NAME} PRIVATE src/BaslerCamThread.h src/BaslerCamThread.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_BASLER)

	target_link_libraries(${PROJECT_NAME} PRIVATE Pylon5::Base Pylon5::Utility Pylon5::GenAPI Pylon5::GCBase)
endif()

message(STATUS "Enabled camera backends: ${CAMERA_BACKENDS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer)
	target_compile_options(${PROJECT_NAME} PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
endif()

configure_file(src/version.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)
target_sources(${PROJECT_NAME} PRIVATE src/version.h "${CMAKE_CURRENT_BINARY_DIR}/version.cpp")

add_library(build_timestamp STATIC EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/build_timestamp.cpp")
add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build_timestamp.cpp"
	COMMAND ${CMAKE_COMMAND}
		-DSRC="${CMAKE_CURRENT_SOURCE_DIR}/src/build_timestamp.cpp.in"
		-DDST="${CMAKE_CURRENT_BINARY_DIR}/build_timestamp.cpp"
		-P ${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/BuildTimestamp.cmake
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/build_timestamp.cpp.in")

target_link_libraries(${PROJECT_NAME} PRIVATE build_timestamp)
add_custom_command(TARGET ${PROJECT_NAME} PRE_LINK
	COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_SOURCE_DIR}/src/build_timestamp.cpp.in"
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target build_timestamp
)
