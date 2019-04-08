// Copyright (C) 2019 SmartWireless GmbH & Co. KG
// 
// This file is part of mselph264.
// 
// mselph264 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
// 
// mselph264 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with mselph264.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PLUGIN_UTILS_HPP__
#define PLUGIN_UTILS_HPP__

#include <mediastreamer2/formats.h>

constexpr bool operator==(const MSVideoSize &a, const MSVideoSize &b) {
  return (a.width == b.width && a.height == b.height);
}

constexpr bool operator!=(const MSVideoSize &a, const MSVideoSize &b) {
  return !(a == b);
}

#endif