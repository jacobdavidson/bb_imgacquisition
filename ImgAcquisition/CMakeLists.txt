
set(LIBS 
    ${CUDA_LIBRARIES} 
    ${Qt5Core_LIBRARIES} 
    ${Boost_LIBRARIES}
		)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)


message("CUDA_LIBRARIES is ${CUDA_LIBRARIES}")
message("Boost_LIBRARIES is ${Boost_LIBRARIES}")
message("Qt5Core_LIBRARIES is ${Qt5Core_LIBRARIES}")


if("${CMAKE_SYSTEM}" MATCHES "Linux")
	set(INCLUDE_DIRS /usr/local/include /usr/include/flycapture/ ${CUDA_INCLUDE_DIRS})
	set(LIBS ${LIBS} dl flycapture cuda -L/usr/local/lib)
	
	message("Configuring for linux...")
else()
	set(INCLUDE_DIRS "C:\\Program Files\\Point Grey Research\\FlyCapture2\\include" ${CUDA_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
	set(LIBS ${LIBS}  
	"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2.lib"
	"C:\\Program Files\\Point Grey Research\\FlyCapture2\\lib64\\FlyCapture2_v110.lib"
	#"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cudart.lib"
	#"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v7.5\\lib\\x64\\cuda.lib"
	)
	message("Configuring for windows...")
endif()


include_directories(${INCLUDE_DIRS})

message("Configuring ImageAcquisition...")
set(EXE_NAME bb_ImageAcquistion)
file(GLOB_RECURSE ImgAcquisitionSrc RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h *.hpp)
add_executable(${EXE_NAME} ${ImgAcquisitionSrc} )
target_link_libraries(${EXE_NAME} ${LIBS})

