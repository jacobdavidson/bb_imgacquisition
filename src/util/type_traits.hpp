// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <type_traits>

template<typename T>
struct false_type : std::false_type
{
};
