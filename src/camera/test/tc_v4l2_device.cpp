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

#include <h264camera/v4l2_device.hpp>

#include <catch.hpp>

using namespace h264camera;

constexpr auto dev_path_mjpeg = "/dev/elp-mjpeg";

TEST_CASE("Compare resolutions", "[v4l2]") {
  CHECK(VIDEO_SIZE_720P == VIDEO_SIZE_720P);
  CHECK_FALSE(VIDEO_SIZE_720P == VIDEO_SIZE_SVGA);
}

TEST_CASE("No device", "[v4l2]") { REQUIRE_THROWS(Device("no_device")); }

TEST_CASE("Open v4l2 camera device", "[v4l2][device]") {
  auto dev = Device(dev_path_mjpeg);
  dev.open();
  CHECK(dev.isOpen());
  CHECK(dev.path() == dev_path_mjpeg);
}

TEST_CASE("Map memory", "[v4l2][device]") {
  auto dev = Device(dev_path_mjpeg);
  dev.open();
  dev.setFormat(VIDEO_SIZE_720P.width, VIDEO_SIZE_720P.height,
                V4L2_PIX_FMT_MJPEG);
  dev.mmap();

  // all buffers are queued at this point
  CHECK_FALSE(dev.freeSlots());
  CHECK(dev.queued());
  CHECK(0 == dev.queue());

  REQUIRE(0 == dev.streamOn());
  using namespace std::chrono_literals;
  auto frame = dev.dequeue(250ms);
  CHECK(nullptr != frame);
  CHECK(dev.freeSlots());
  CHECK(dev.queued());
  frame = dev.dequeue(250ms);
  CHECK(nullptr != frame);
  frame = dev.dequeue(250ms);
  CHECK(nullptr != frame);
  frame = dev.dequeue(250ms);
  CHECK(nullptr != frame);
  frame = dev.dequeue(250ms);
  CHECK(nullptr != frame);
  frame = dev.dequeue(250ms);
  CHECK_FALSE(dev.queued());
  REQUIRE(0 == dev.streamOff());
}

TEST_CASE("Take a picture", "[v4l2][device]") {
  auto dev = Device(dev_path_mjpeg);
  dev.open();
  dev.setFormat(VIDEO_SIZE_720P.width, VIDEO_SIZE_720P.height,
                 V4L2_PIX_FMT_MJPEG);
  dev.mmap();

  CHECK(0 == dev.streamOn());
  auto frame = dev.dequeue(std::chrono::milliseconds(500));
  REQUIRE(nullptr != frame);
  CHECK(Device::Mem::State::READY == frame->state);
  // maybe check data
  CHECK(0 == dev.streamOff());
}

TEST_CASE("Reopen with new configuration", "[v4l2][device]") {
  auto dev = Device(dev_path_mjpeg);
  dev.open();

  dev.setFormat(VIDEO_SIZE_VGA.width, VIDEO_SIZE_VGA.height,
                 V4L2_PIX_FMT_MJPEG);
  dev.mmap();
  CHECK(0 == dev.streamOn());
  CHECK(0 == dev.streamOff());

  dev.reopen();

  dev.setFormat(VIDEO_SIZE_SVGA.width, VIDEO_SIZE_SVGA.height,
                 V4L2_PIX_FMT_MJPEG);
  dev.mmap();
  CHECK(0 == dev.streamOn());
  CHECK(0 == dev.streamOff());
}
