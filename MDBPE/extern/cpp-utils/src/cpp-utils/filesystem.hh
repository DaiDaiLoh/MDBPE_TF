#pragma once

#include <clean-core/string_view.hh>

namespace util
{
void make_directories(cc::string_view path);

bool exists(cc::string_view path);
}
