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

#ifndef ELPH264_DATA_HELPER_HPP__
#define ELPH264_DATA_HELPER_HPP__

#include <array>

namespace h264camera {

// NOTE: The int conversion uses only 24bit

template <std::size_t size = 11>
constexpr double data_to_int(const std::array<uint8_t, size> &data) {
  static_assert(size >= 3, "Array is too small");
  return (data[0] << 16) | (data[1] << 8) | data[2];
}

template <std::size_t size = 11>
constexpr std::array<uint8_t, size> data_from_int(int32_t val) {
  static_assert(size >= 3, "Array is too small");
  return {{static_cast<uint8_t>((val & 0x00FF0000) >> 16),
           static_cast<uint8_t>((val & 0x0000FF00) >> 8),
           static_cast<uint8_t>(val & 0x000000FF)}};
}

template <std::size_t size = 11>
constexpr bool data_to_bool(const std::array<uint8_t, size> &data) {
  static_assert(size >= 1, "Array is too small");
  return data[0] == 1;
}

template <std::size_t size = 11>
constexpr std::array<uint8_t, size> data_from_bool(bool val) {
  static_assert(size >= 1, "Array is too small");
  return {{(val) ? static_cast<uint8_t>(0x01) : static_cast<uint8_t>(0x00)}};
}
}

#endif