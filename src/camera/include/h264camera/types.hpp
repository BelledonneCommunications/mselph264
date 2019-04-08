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

#ifndef H264_CAMERA_HPP__
#define H264_CAMERA_HPP__

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>

namespace h264camera {

// Convert register values into or from structurs for ease of use

enum class Mode : uint8_t { CBR = 1, VBR = 2 };

struct ChipId {
  int id;
};

struct TimeRTC {
  std::chrono::system_clock::time_point time;
};

template <std::size_t SIZE, typename TYPE>
constexpr auto convert_to(const std::array<uint8_t, SIZE> &data) -> TYPE;

template <>
constexpr auto convert_to(const std::array<uint8_t, 11> &data) -> int {
  return (data[0] << 16) | (data[1] << 8) | data[2];
}

template <>
constexpr auto convert_to(const std::array<uint8_t, 11> &data) -> bool {
  return data[0] == 1;
}

template <>
constexpr auto convert_to(const std::array<uint8_t, 4> &data) -> ChipId {
  return {data[2]};
}

template <>
constexpr auto convert_to(const std::array<uint8_t, 11> &data) -> Mode {
  return static_cast<Mode>(data[0]);
}

template <>
inline auto convert_to(const std::array<uint8_t, 11> &data) -> TimeRTC {
  tm cam_rtc;
  cam_rtc.tm_sec = data[0];
  cam_rtc.tm_min = data[1];
  cam_rtc.tm_hour = data[2];
  cam_rtc.tm_mday = data[3];
  cam_rtc.tm_mon = data[4] - 1;
  cam_rtc.tm_year = ((data[5] << 8) | data[6]) - 1900;
  return TimeRTC{std::chrono::system_clock::from_time_t(mktime(&cam_rtc))};
}

template <std::size_t SIZE, typename TYPE>
constexpr void convert_from(std::array<uint8_t, SIZE> &dest, TYPE value);

template <>
constexpr void convert_from(std::array<uint8_t, 11> &dest, int value) {
  dest = {{static_cast<uint8_t>((value & 0x00FF0000) >> 16),
           static_cast<uint8_t>((value & 0x0000FF00) >> 8),
           static_cast<uint8_t>(value & 0x000000FF)}};
}

template <>
constexpr void convert_from(std::array<uint8_t, 11> &dest, bool value) {
  dest = {{(value) ? static_cast<uint8_t>(0x01) : static_cast<uint8_t>(0x00)}};
}

template <>
constexpr void convert_from(std::array<uint8_t, 11> &dest, Mode value) {
  dest[0] = static_cast<uint8_t>(value);
}

template <>
inline void convert_from(std::array<uint8_t, 11> &dest, TimeRTC value) {
  auto tt = std::chrono::system_clock::to_time_t(value.time);
  tm cam_rtc = *std::localtime(&tt);
  dest[0] = cam_rtc.tm_sec;
  dest[1] = cam_rtc.tm_min;
  dest[2] = cam_rtc.tm_hour;
  dest[3] = cam_rtc.tm_mday;
  dest[4] = cam_rtc.tm_mon + 1;
  int year = cam_rtc.tm_year + 1900;
  dest[5] = (year & 0xFF00) >> 8;
  dest[6] = (year & 0x00FF);
}
} // namespace h264camera

#endif