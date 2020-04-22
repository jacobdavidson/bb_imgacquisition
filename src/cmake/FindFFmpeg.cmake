# SPDX-License-Identifier: GPL-3.0-or-later

find_package(PkgConfig)

if(FFmpeg_FIND_COMPONENTS)
   set(FFmpeg_FOUND PENDING)

   foreach(component ${FFmpeg_FIND_COMPONENTS})

      pkg_check_modules(PC_lib${component} QUIET lib${component})

      find_path(FFmpeg_${component}_INCLUDE_DIR lib${component}/${component}.h
                HINTS ${PC_lib${component}_INCLUDEDIR}
                      ${PC_lib${component}_INCLUDE_DIRS}
                      $ENV{FFMPEG_DIR}/include)

      find_library(FFmpeg_${component}_LIBRARY NAMES ${component} lib${component}
                   HINTS ${PC_lib${component}_LIBDIR}
                         ${PC_lib${component}_LIBRARY_DIRS}
                         $ENV{FFMPEG_DIR}/lib)

      mark_as_advanced(lib${component}_INCLUDE_DIR lib${component}_LIBRARY)

      if(FFmpeg_${component}_INCLUDE_DIR AND FFmpeg_${component}_LIBRARY)
         set(FFmpeg_${component}_FOUND TRUE)

         add_library(FFmpeg::${component} UNKNOWN IMPORTED)
         set_target_properties(FFmpeg::${component}
            PROPERTIES
               IMPORTED_LOCATION "${FFmpeg_${component}_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_${component}_INCLUDE_DIR}")
      else()
         if(FFmpeg_FIND_REQUIRED_${component})
            set(FFmpeg_FOUND FALSE)
         endif()
      endif()
   endforeach()

   if(FFmpeg_FOUND STREQUAL PENDING)
      set(FFmpeg_FOUND TRUE)
   endif()
endif()

if(NOT FFmpeg_FOUND)
   if(FFmpeg_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find FFmpeg")
   endif()
endif()
