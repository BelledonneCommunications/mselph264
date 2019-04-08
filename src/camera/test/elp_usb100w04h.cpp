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

using h264camera::Usb100W04H;

TEST_CASE("Access standard camera interface", "[usb100w04h, camera]") {
  auto device = std::make_unique<Usb100W04H>("/dev/video1");
  REQUIRE(device);
  device->open();
  REQUIRE(device->is_open());

  SECTION("Set various resolutions") {
    const auto vsizes = {h264camera::v4l2::VIDEO_SIZE_720P,
                         h264camera::v4l2::VIDEO_SIZE_SVGA,
                         {640, 480},
                         {640, 360}};
    for (const auto vsize : vsizes) {
      device->setVSize(vsize);
      CHECK(device->vsize() == vsize);
    }
  }

  SECTION("Set various frame rates") {
    const auto fpss = {30.0, 20.0, 15.0, 10.0, 5.0};
    for (const auto fps : fpss) {
      device->setFps(fps);
      CHECK(device->fps() == fps);
    }
  }

  device->close();
}

TEST_CASE("Access extended camera interface", "[usb100w04h, camera]") {
  auto device = std::make_unique<Usb100W04H>("/dev/video1");
  REQUIRE(device);
  device->open();
  device->add_xu_ctrl();

  SECTION("Set various bitrates") {
    const auto bitrates = {4e+6, 2e+6, 0e+0};
    for (const auto bitrate : bitrates) {
      device->setBitrate(bitrate);
      CHECK(device->bitrate() == bitrate);
    }
  }

  SECTION("Set mode (CBR/VBR)") {
    device->setMode(Usb100W04H::Mode::CBR);
    CHECK(device->mode() == Usb100W04H::Mode::CBR);
    device->setMode(Usb100W04H::Mode::VBR);
    CHECK(device->mode() == Usb100W04H::Mode::VBR);
  }

  SECTION("Call resetIFrame") { device->resetIFrame(); }

  device->close();
}
