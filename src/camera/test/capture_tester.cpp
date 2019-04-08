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

#include <cstdio>
#include <cstring>
#include <fmt/format.h>
#include <h264camera/elp_usb100w04h.hpp>
#include <h264camera/v4l2_device.hpp>
#include <iostream>
#include <sys/time.h>

constexpr auto dev_path_h264 = "/dev/elp-h264";

using namespace h264camera;

using namespace std::chrono_literals;

/// Make a video capturing *device* with *frame_count* frames a resolution of
/// *width*x**height* and a framerate *fps*.  The data is written to *fname*.
int capture(Usb100W04H &device, uint32_t width, uint32_t height, uint32_t fps,
            int frame_count, const std::string &fname) {
  fmt::print("Capture {} images with {}x{} and write to {}\n", frame_count,
             width, height, fname.c_str());

  device.reopen();
  device.xuEnableSeiHeader(true);
  device.setFormat(width, height, V4L2_PIX_FMT_H264);
  device.setFramerate(fps);

  auto stream = std::fopen(fname.c_str(), "wb");
  if (!stream) {
    std::perror("fopen failed");
    return -1;
  }
  device.mmap();

  device.streamOn();

  if (auto frame = device.dequeue(500ms)) {
    frame->done();
    device.queue(frame->index);
  }
  device.xuSetFlip(false);
  if (auto frame = device.dequeue(500ms)) {
    frame->done();
    device.queue(frame->index);
  }
  device.xuSetFlip(true);
  if (auto frame = device.dequeue(500ms)) {
    frame->done();
    device.queue(frame->index);
  }

  int capture_counter = 0;
  for (int idx = 0; idx < frame_count; ++idx) {
    if (idx % 10 == 0) {
      device.xuResetIFrame();
    }
    using namespace std::chrono_literals;
    if (auto frame = device.dequeue(500ms)) {
      const auto &buf = frame->video_buffer;
      fmt::print("Frame [{:2d}] buffer: {} used: {:5d} seq: {:2d} timestamp: "
                 "{:d}.{:06d}\n",
                 idx, buf.index, buf.bytesused, buf.sequence,
                 buf.timestamp.tv_sec, buf.timestamp.tv_usec);
      ++capture_counter;
      std::fwrite(frame->ptr, frame->used(), 1, stream);
      frame->done();
      device.queue(frame->index);
    } else {
      fmt::print("Timeout when dequeuing frame\n");
    }
  }
  device.streamOff();

  fmt::print("Captured {} of {} images\n", capture_counter, frame_count);

  std::fclose(stream);

  return 0;
}

int main(int argc, char *argv[]) {
  try {
    Usb100W04H device(dev_path_h264);
    device.open();
    device.addXuCtrl();
    device.xuUpdateOsdRtc();
    fmt::print("Camera device: {}\n", dev_path_h264);
    fmt::print("Chip id: {}\n", device.xuReadChipId());
    fmt::print("Mode: {}\n", static_cast<int>(device.xuMode()));

    const auto duration = 2s;
    // const auto resolutions = {VIDEO_SIZE_720P, VIDEO_SIZE_VGA};
    const auto resolutions = {VIDEO_SIZE_VGA};
    // const auto fpss = {30, 5};
    const auto fpss = {15};

    for (auto vsize : resolutions) {
      for (auto fps : fpss) {
        fmt::memory_buffer buf;
        format_to(buf, "stream_{}x{}_{}.h264", vsize.width, vsize.height, fps);
        int count = duration.count() * fps;
        capture(device, vsize.width, vsize.height, fps, count, to_string(buf));
      }
    }
  } catch (const std::exception &e) {
    fmt::print("Exception: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}