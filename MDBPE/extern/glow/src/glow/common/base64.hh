#pragma once

/*
   base64.cc and base64.hh

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

   with modifications to fit better with glow
*/

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

#include <glow/common/array_view.hh>

namespace glow
{
std::string base64_encode(array_view<std::byte const> data);
std::vector<std::byte> base64_decode(std::string_view encoded_string);
}
