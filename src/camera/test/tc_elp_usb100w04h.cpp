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

#include <h264camera/elp_usb100w04h.hpp>

#include <catch.hpp>

using namespace h264camera;

constexpr auto dev_path_mjpeg = "/dev/elp-mjpeg";

TEST_CASE("Access extended camera interface", "[usb100w04h][device]") {
  auto dev = Usb100W04H(dev_path_mjpeg);
  dev.open();

  dev.addXuCtrl();

  SECTION("Set bitrate") {
    dev.xuSetBitrate(4e+6);
    CHECK(4e+6 == dev.xuBitrate());
    dev.xuSetBitrate(2e+6);
    CHECK(2e+6 == dev.xuBitrate());
    dev.xuSetBitrate(0e+0);
    CHECK(0e+0 == dev.xuBitrate());
  }

  SECTION("Set mode (CBR/VBR)") {
    dev.xuSetMode(Mode::CBR);
    CHECK(Mode::CBR == dev.xuMode());
    dev.xuSetMode(Mode::VBR);
    CHECK(Mode::VBR == dev.xuMode());
  }

  SECTION("Call resetIFrame") { dev.xuResetIFrame(); }

  SECTION("Set SEI header") {
    dev.xuEnableSeiHeader(false);
    CHECK_FALSE(dev.xuSeiHeaderEnabled());
    dev.xuEnableSeiHeader(true);
    CHECK(dev.xuSeiHeaderEnabled());
  }

  SECTION("Flip") {
    dev.xuSetFlip(false);
    CHECK_FALSE(dev.xuFlip());
    dev.xuSetFlip(true);
    CHECK(dev.xuFlip());
  }

  SECTION("Set color") {
    dev.xuSetColor(false);
    CHECK_FALSE(dev.xuColor());
    dev.xuSetColor(true);
    CHECK(dev.xuColor());
  }

  SECTION("Set OSD time") { dev.xuUpdateOsdRtc(); }

  dev.close();
}

TEST_CASE("Convert types") {
  std::array<uint8_t, 11> data;
  SECTION("int") {
    convert_from<11, int>(data, 0x123456);
    CHECK(data[0] == 0x12);
    CHECK(data[1] == 0x34);
    CHECK(data[2] == 0x56);
    CHECK(data[4] == 0x0);
    CHECK(0x123456 == convert_to<11, int>(data));
  }
  SECTION("bool") {
    convert_from<11, bool>(data, true);
    CHECK(data[0] == 0x1);
    CHECK(data[1] == 0x0);
    CHECK(true == convert_to<11, bool>(data));
    convert_from<11, bool>(data, false);
    CHECK(data[0] == 0x0);
    CHECK(data[1] == 0x0);
    CHECK(false == convert_to<11, bool>(data));
  }
  SECTION("Mode") {
    convert_from<11, Mode>(data, Mode::CBR);
    CHECK(data[0] == 1);
    CHECK(Mode::CBR == convert_to<11, Mode>(data));
    convert_from<11, Mode>(data, Mode::VBR);
    CHECK(data[0] == 2);
    CHECK(Mode::VBR == convert_to<11, Mode>(data));
  }
  SECTION("Time") {
    tm reference_time;
    reference_time.tm_sec = 1;
    reference_time.tm_min = 2;
    reference_time.tm_hour = 3;
    reference_time.tm_mday = 4;
    reference_time.tm_mon = 5 - 1;
    reference_time.tm_year = 2018 - 1900;
    auto tt = std::mktime(&reference_time);
    TimeRTC time_1{std::chrono::system_clock::from_time_t(tt)};

    convert_from<11, TimeRTC>(data, time_1);
    CHECK(data[0] == 1);
    CHECK(data[1] == 2);
    CHECK(data[2] == 3);
    CHECK(data[3] == 4);
    CHECK(data[4] == 5);
    CHECK(data[5] == 7);
    CHECK(data[6] == 226);

    auto time_2 = convert_to<11, TimeRTC>(data);
    CHECK(time_1.time == time_2.time);
  }
}