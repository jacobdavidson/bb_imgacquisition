
set(LIBS flycapture
    ${Boost_LIBRARIES} )

set(CMAKE_INCLUDE_CURRENT_DIR OFF)

# Location of the CUDA Toolkit binaries and libraries
set(CUDA_PATH /usr/local/cuda-7.5)
set(CUDA_INC_PATH   ${CUDA_PATH}/include)
set(CUDA_LIB_PATH   ${CUDA_PATH}/lib)
set(CUDA_BIN_PATH   ${CUDA_PATH}/bin)

# Common binaries
set(NVCC  ${CUDA_BIN_PATH}/nvcc)

set(CUDA_LIBS 	cuda
		cudart
		nvidia-encode
		dl)
set(CUDA_LIBS  ${CUDA_LIBS} -L${CUDA_LIB_PATH}64 )

message("CUDA_LIBS is ${CUDA_LIBS}")


#TODO: configure this properly
set(INCLUDE_DIRS /usr/local/include /usr/include/flycapture/ ${CUDA_INC_PATH})
set(LIBS ${LIBS} -L/usr/local/lib Qt5::Core)
include_directories(${INCLUDE_DIRS})

message("Configuring ImageAcquisition...")
set(EXE_NAME bb_ImageAcquistion)
file(GLOB_RECURSE ImgAcquisitionSrc RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h *.hpp)
add_executable(${EXE_NAME} ${ImgAcquisitionSrc} )
target_link_libraries(${EXE_NAME} ${LIBS} ${CUDA_LIBS})

