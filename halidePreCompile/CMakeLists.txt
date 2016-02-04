
set(LIBS )

set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(HALIDE_LIBS Halide)

#TODO: configure this properly
set(HALIDE_INCLUDE_DIRS /usr/local/include)
set(HALIDE_LIBS ${HALIDE_LIBS} -L/usr/local/lib)
include_directories(${HALIDE_INCLUDE_DIRS})

message("Configuring halideYuvConv...")
set(EXE_NAME halideYuvConvPrec)
add_executable(${EXE_NAME} halideYuvConv.cpp )
target_link_libraries(${EXE_NAME} ${HALIDE_LIBS} ${LIBS}) #/usr/lib64

