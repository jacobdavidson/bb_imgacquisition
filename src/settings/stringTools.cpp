// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * stringTools.cpp
 *
 *  Created on: Dec 18, 2014
 *      Author: tobias
 */

#include <climits>
#include <boost/filesystem/path.hpp>

#include <string>    // std::string
#include <stdexcept> // std::invalid_argument

std::string stem_filename(const std::string& s)
{
    boost::filesystem::path p(s);
    return p.stem().string();
}
