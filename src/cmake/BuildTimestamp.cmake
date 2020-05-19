# SPDX-License-Identifier: GPL-3.0-or-later

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%MZ" UTC)

configure_file("${SRC}" "${DST}" @ONLY)
