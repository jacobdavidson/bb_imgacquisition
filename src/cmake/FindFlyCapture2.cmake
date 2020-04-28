# SPDX-License-Identifier: GPL-3.0-or-later

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    find_path(FlyCapture2_INCLUDE_DIR FlyCapture2.h
              PATHS
                  /usr/include/flycapture)

    find_library(FlyCapture2_LIBRARY NAMES flycapture)

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

    find_path(FlyCapture2_INCLUDE_DIR FlyCapture2.h
              PATHS
                  "$ENV{FLYCAPTURE2_DIR}/include"
                  "C:/Program Files/Point Grey Research/FlyCapture2/include")

    if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(FlyCapture2_LIBRARY_DIR "lib64")
    else()
        set(FlyCapture2_LIBRARY_DIR "lib")
    endif()

    find_library(FlyCapture2_LIBRARY
                 NAMES
                     FlyCapture2
                 PATHS
                    "$ENV{FLYCAPTURE2_DIR}${FlyCapture2_LIBRARY_DIR}"
                    "C:/Program Files/Point Grey Research/FlyCapture2/${FlyCapture2_LIBRARY_DIR}")

    find_library(FlyCapture2_Toolset_LIBRARY
                 NAMES
                     FlyCapture2_v110
                 PATHS
                     "$ENV{FLYCAPTURE2_DIR}${FlyCapture2_LIBRARY_DIR}"
                     "C:/Program Files/Point Grey Research/FlyCapture2/${FlyCapture2_LIBRARY_DIR}")
endif()

include(FindPackageHandleStandardArgs)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package_handle_standard_args(FlyCapture2 DEFAULT_MSG
                                      FlyCapture2_INCLUDE_DIR FlyCapture2_LIBRARY)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package_handle_standard_args(FlyCapture2 DEFAULT_MSG
                                      FlyCapture2_INCLUDE_DIR FlyCapture2_LIBRARY FlyCapture2_Toolset_LIBRARY)
    mark_as_advanced(FlyCapture2_Toolset_LIBRARY)
endif()

mark_as_advanced(FlyCapture2_INCLUDE_DIR FlyCapture2_LIBRARY)

add_library(FlyCapture2 UNKNOWN IMPORTED)
set_target_properties(FlyCapture2 PROPERTIES IMPORTED_LOCATION ${FlyCapture2_LIBRARY})
set_target_properties(FlyCapture2 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${FlyCapture2_INCLUDE_DIR})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_library(FlyCapture2::Toolset UNKNOWN IMPORTED)
    set_target_properties(FlyCapture2::Toolset PROPERTIES IMPORTED_LOCATION ${FlyCapture2_Toolset_LIBRARY})
    set_target_properties(FlyCapture2::Toolset PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${FlyCapture2_INCLUDE_DIR})

    target_link_libraries(FlyCapture2 INTERFACE FlyCapture2::Toolset)
endif()
