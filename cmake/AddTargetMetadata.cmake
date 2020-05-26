# SPDX-License-Identifier: GPL-3.0-or-later

include(GetGitRevisionDescription)

function(add_target_metadata target)
	cmake_parse_arguments(ARG "" "SOURCE_VERSION" "BUILD_TIMESTAMP" ${ARGN})

	set(METADATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/${target}_metadata")

	file(MAKE_DIRECTORY "${METADATA_DIR}")

	target_include_directories(${target} PRIVATE "${METADATA_DIR}")

	if(ARG_SOURCE_VERSION)
		file(WRITE "${METADATA_DIR}/source_version.h"
"// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

extern const char g_SOURCE_VERSION[];")

		file(WRITE "${METADATA_DIR}/source_version.cpp.in"
"// SPDX-License-Identifier: GPL-3.0-or-later

#include \"source_version.h\"

const char g_SOURCE_VERSION[] = \"@SOURCE_VERSION@\";")

		configure_file("${METADATA_DIR}/source_version.cpp.in" "${METADATA_DIR}/source_version.cpp" @ONLY)
		target_sources(${target} PRIVATE "${METADATA_DIR}/source_version.h" "${METADATA_DIR}/source_version.cpp")
	endif()

	if(ARG_BUILD_TIMESTAMP)
		file(WRITE "${METADATA_DIR}/build_timestamp.h"
"// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

extern const char g_BUILD_TIMESTAMP[];")

		file(WRITE "${METADATA_DIR}/build_timestamp.cpp.in"
"// SPDX-License-Identifier: GPL-3.0-or-later

#include \"build_timestamp.h\"

const char g_BUILD_TIMESTAMP[] = \"@BUILD_TIMESTAMP@\";")

		file(WRITE "${METADATA_DIR}/ConfigureBuildTimestamp.cmake"
"# SPDX-License-Identifier: GPL-3.0-or-later

string(TIMESTAMP BUILD_TIMESTAMP ${ARG_BUILD_TIMESTAMP})

configure_file(\"\${CMAKE_CURRENT_LIST_DIR}/build_timestamp.cpp.in\" \"\${CMAKE_CURRENT_LIST_DIR}/build_timestamp.cpp\" @ONLY)
")

		add_custom_command(
			OUTPUT "${METADATA_DIR}/build_timestamp.cpp"
			COMMAND ${CMAKE_COMMAND} -P "${METADATA_DIR}/ConfigureBuildTimestamp.cmake"
			DEPENDS "${METADATA_DIR}/build_timestamp.cpp.in")

		add_library(${target}_build_timestamp STATIC EXCLUDE_FROM_ALL  "${METADATA_DIR}/build_timestamp.cpp")
		set_target_properties(${target}_build_timestamp PROPERTIES PREFIX "")
		set_target_properties(${target}_build_timestamp PROPERTIES ARCHIVE_OUTPUT_NAME "build_timestamp")
		set_target_properties(${target}_build_timestamp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${METADATA_DIR}")
		target_include_directories(${target}_build_timestamp PRIVATE "${METADATA_DIR}")

		target_link_libraries(${target} PRIVATE ${target}_build_timestamp)
		add_custom_command(TARGET ${target} PRE_LINK
			COMMAND ${CMAKE_COMMAND} -E touch "${METADATA_DIR}/build_timestamp.cpp.in"
			COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target ${target}_build_timestamp)
	endif()

endfunction()
