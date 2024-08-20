cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(GetGitRevisionDescription)
git_describe(SOURCE_VERSION --always --exclude *)
git_local_changes(GIT_DIRTY)
if("${GIT_DIRTY}" STREQUAL DIRTY)
	string(APPEND SOURCE_VERSION "-dirty")
endif()

message(STATUS "Source version: ${SOURCE_VERSION}")

project(bb_imgacquisition LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)  # added for boost compatibility

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
	src/util/type_traits.hpp
	src/util/format.hpp
	src/util/log.hpp
	src/util/log.cpp
	src/util/ConcurrentQueue.hpp
	src/util/PlatformAdapter.hpp
	src/util/PlatformAdapter.cpp
	src/GrayscaleImage.hpp
	src/GrayscaleImage.cpp
	src/ImageStream.hpp
	src/ImageStream.cpp
	src/ImageStreamsWriter.hpp
	src/ImageStreamsWriter.cpp
	src/VideoFileWriter.hpp
	src/VideoFileWriter.cpp
	src/Watchdog.hpp
	src/Watchdog.cpp
	src/Settings.hpp
	src/Settings.cpp
	src/camera/Camera.hpp
	src/camera/Camera.cpp
	src/ImgAcquisitionApp.hpp
	src/ImgAcquisitionApp.cpp
	src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME} PRIVATE
	${CMAKE_DL_LIBS}
	Threads::Threads
	fmt
	Qt5::Core
	Boost::headers Boost::system Boost::filesystem Boost::program_options Boost::date_time Boost::disable_autolinking
	FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil
	${OpenCV_LIBRARIES}
	dl # Added explicitly to ensure the linker finds libdl
	z  # Added explicit linking
)

# Find pkg-config
find_package(PkgConfig REQUIRED)

# Use pkg-config to get the flags for compiling and linking against FFmpeg libraries
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)

# Include directories
include_directories(${AVCODEC_INCLUDE_DIRS} ${AVFORMAT_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS} ${SWSCALE_INCLUDE_DIRS})

# Link against FFmpeg libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${AVCODEC_LIBRARIES} ${AVFORMAT_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})

# For other libraries like x264, x265, mp3lame, opus, vorbis, explicitly link them as well
target_link_libraries(${PROJECT_NAME} PRIVATE x264 x265 mp3lame opus vorbis)

if(CAMERA_BACKENDS)
	foreach(camera_backend ${CAMERA_BACKENDS})
		if(${camera_backend} STREQUAL Flea3)
			find_package(FlyCapture2 REQUIRED)
		elseif(${camera_backend} STREQUAL XIMEA)
			find_package(XIMEA REQUIRED)
		elseif(${camera_backend} STREQUAL Basler)
			# Attempt to find Pylon.  ideal is 7.4 or higher
			find_package(PYLON QUIET)
			if(NOT PYLON_FOUND)
				# If neither Pylon 7 nor Pylon 6 is found, fallback to Pylon 5
				find_package(Pylon5 QUIET)
				if(Pylon5_FOUND)
					message(STATUS "Using Pylon 5 for Basler cameras.")
					set(PYLON_TARGET Pylon5::Base Pylon5::Utility Pylon5::GenAPI Pylon5::GCBase)
				else()
					message(FATAL_ERROR "Pylon (version 5, 6, or 7.4 and above) not found for Basler cameras.")
				endif()
			else()
				message(STATUS "Using Pylon ${PYLON_VERSION} for Basler cameras.")
				set(PYLON_TARGET pylon::pylon)
			endif()
			if(NOT PYLON_TARGET)
				message(FATAL_ERROR "Pylon target not set for Basler cameras.")
			endif()
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

	# Attempt to find Pylon.  Ideal is 7.4 or higher
	find_package(PYLON QUIET)
	if(NOT PYLON_FOUND)
		# If neither Pylon 7 nor Pylon 6 is found, fallback to Pylon 5
		find_package(Pylon5 QUIET)
	endif()

	# Check if any version of Pylon was found and append Basler to CAMERA_BACKENDS
	if(PYLON_FOUND OR Pylon5_FOUND)
		list(APPEND CAMERA_BACKENDS Basler)
	endif()


endif()

if(Flea3 IN_LIST CAMERA_BACKENDS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FLEA3)
	target_sources(${PROJECT_NAME} PRIVATE src/camera/Flea3Camera.hpp src/camera/Flea3Camera.cpp)

	target_link_libraries(${PROJECT_NAME} PRIVATE FlyCapture2)
endif()

if(XIMEA IN_LIST CAMERA_BACKENDS)
	target_sources(${PROJECT_NAME} PRIVATE src/camera/XimeaCamera.hpp src/camera/XimeaCamera.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_XIMEA)

	target_link_libraries(${PROJECT_NAME} PRIVATE XIMEA)
endif()

if(Basler IN_LIST CAMERA_BACKENDS)
	target_sources(${PROJECT_NAME} PRIVATE src/camera/BaslerCamera.hpp src/camera/BaslerCamera.cpp)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_BASLER)

	# Check which version of Pylon is found and link the appropriate libraries
	if(PYLON_FOUND)
		message(STATUS "Linking against Pylon ${PYLON_VERSION}")
		target_link_libraries(${PROJECT_NAME} PRIVATE pylon::pylon)
	elseif(Pylon5_FOUND)
		# For Pylon 5
		message(STATUS "Linking against Pylon version 5")
		target_link_libraries(${PROJECT_NAME} PRIVATE Pylon5::Base Pylon5::Utility Pylon5::GenAPI Pylon5::GCBase)
	else()
		message(FATAL_ERROR "Pylon SDK required for Basler backend not found.")
	endif()
endif()


message(STATUS "Enabled camera backends: ${CAMERA_BACKENDS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(${PROJECT_NAME} PRIVATE -MMD -fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer)
	target_compile_options(${PROJECT_NAME} PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
endif()

include(AddTargetMetadata)
add_target_metadata(${PROJECT_NAME}
	SOURCE_VERSION ${SOURCE_VERSION}
	BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
