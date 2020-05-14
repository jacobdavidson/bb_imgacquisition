// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <ctime>
#include <tuple>
#include <chrono>

std::string get_utc_time();
std::string get_utc_offset_string();
std::string getTimestamp();

// Return current calendar date and time in UTC according to system clock
std::tuple<std::tm, std::chrono::microseconds::rep> getUTCDateTime();
