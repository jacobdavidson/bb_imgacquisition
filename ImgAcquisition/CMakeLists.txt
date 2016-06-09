
#########################
#Get all the libraries into a variable
#########################
set(LIBS 
    ${CUDA_LIBRARIES} 
    ${Qt5Core_LIBRARIES} 
    ${Boost_LIBRARIES}
    ${OpenCV_LIBRARIES}
		)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)


message("CUDA_LIBRARIES is ${CUDA_LIBRARIES}")
message("Boost_LIBRARIES is ${Boost_LIBRARIES}")
message("OpenCV_LIBRARIES is ${OpenCV_LIBRARIES}")
message("Qt5Core_LIBRARIES is ${Qt5Core_LIBRARIES}")

#########################
#As the AOT compiling app needs to be run beforehand, add 
#running it als as a dependency (not only compiling it!)
#########################

#Add halide precompiled functions headers and name object files to link to
set(HALIDE_INCLUDE ${PROJECT_BINARY_DIR}/halidePreCompile)
set(HALIDE_OBJECTS ${PROJECT_BINARY_DIR}/halidePreCompile/halideYuv420Conv.o)

add_custom_command(
	#halideYuvConvPrecExecution
	WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/halidePreCompile/"
	COMMAND "${PROJECT_BINARY_DIR}/halidePreCompile/halideYuvConvPrec" 
	OUTPUT "${PROJECT_BINARY_DIR}/halidePreCompile/halideYuv420Conv.h"
	OUTPUT "${PROJECT_BINARY_DIR}/halidePreCompile/halideYuv420Conv.o"
	DEPENDS halideYuvConvPrec
)
add_custom_target(
	halideJustOnceCompilingDummy ALL
	DEPENDS "${PROJECT_BINARY_DIR}/halidePreCompile/halideYuv420Conv.h"
	DEPENDS "${PROJECT_BINARY_DIR}/halidePreCompile/halideYuv420Conv.o"
	DEPENDS halideYuvConvPrec
)

#This will add the halide AOT compiling of code as a dependency
add_library(halideYuv420Conv  STATIC  ${HALIDE_OBJECTS})
SET_SOURCE_FILES_PROPERTIES(${HALIDE_OBJECTS}  PROPERTIES EXTERNAL_OBJECT true  GENERATED true)
SET_TARGET_PROPERTIES(halideYuv420Conv PROPERTIES  LINKER_LANGUAGE C )

#########################
#Do some OS specific configuration
#########################
if("${CMAKE_SYSTEM}" MATCHES "Linux")
	set(INCLUDE_DIRS /usr/local/include /usr/include/flycapture/ /opt/ffmpeg/ffmpeg_build/include/ )
	set(LIBS -L/usr/local/lib ${LIBS} pthread dl flycapture cuda Halide rt )
	
	message("Configuring for linux...")
else()
	set(INCLUDE_DIRS "C:\\Program Files\\Point Grey Research\\FlyCapture2\\include"  ${Boost_INCLUDE_DIRS} )
	set(LIBS ${LIBS}  
	"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2.lib"
	"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2_v110.lib"
	"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cudart.lib"
	"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cuda.lib"
	)
	message("Configuring for windows...")
endif()


message("All libs: ${LIBS}")

#########################
#Put everything together
#########################

include_directories(${INCLUDE_DIRS} ${HALIDE_INCLUDE} ${CUDA_INCLUDE_DIRS} )

message("Configuring ImageAcquisition...")
set(EXE_NAME bb_ImageAcquistion)
file(GLOB_RECURSE ImgAcquisitionSrc RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h *.hpp)

IF (DEFINED NO_ENCODER)
    file(GLOB_RECURSE ImgAcquisitionSrcExcludeCuda RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} nvenc/*)
	LIST(REMOVE_ITEM ImgAcquisitionSrc ${ImgAcquisitionSrcExcludeCuda})
	add_definitions(-DUSE_ENCODER)
ENDIF()


message("Sources are ${ImgAcquisitionSrc}")
add_executable(${EXE_NAME} ${ImgAcquisitionSrc} )
#Add the halide project as a dependency
add_dependencies(${EXE_NAME} halideYuvConvPrec)
target_link_libraries(${EXE_NAME} halideYuv420Conv ${LIBS} )

#Add this dependency, so halide will not always be built anew
add_dependencies(${EXE_NAME} halideJustOnceCompilingDummy)






