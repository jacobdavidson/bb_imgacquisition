# SPDX-License-Identifier: GPL-3.0-or-later

find_path(XIMEA_INCLUDE_DIR xiApi.h
          PATHS
              /opt/XIMEA/include/
              "$ENV{XIMEA_DIR}/include")

find_library(XIMEA_LIBRARY NAMES m3api)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(XIMEA DEFAULT_MSG
                                  XIMEA_INCLUDE_DIR XIMEA_LIBRARY)

mark_as_advanced(XIMEA_INCLUDE_DIR XIMEA_LIBRARY)

add_library(XIMEA UNKNOWN IMPORTED)
set_target_properties(XIMEA PROPERTIES IMPORTED_LOCATION ${XIMEA_LIBRARY})
set_target_properties(XIMEA PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${XIMEA_INCLUDE_DIR})
