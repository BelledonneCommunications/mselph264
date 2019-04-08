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

#include "h264camera/v4l2_device.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace h264camera {

Device::Device(const std::string &path) : mPath(path) {
  struct stat st;
  if (-1 == ::stat(path.c_str(), &st)) {
    std::stringstream ss;
    ss << "Cannot identify '" << mPath << "'";
    throw std::system_error(errno, std::system_category(), ss.str());
  }
  if (!S_ISCHR(st.st_mode)) {
    std::stringstream ss;
    ss << "File '" << mPath << "' is not a device";
    throw std::runtime_error(ss.str());
  }
}

Device::~Device() { close(); }

void Device::open() {
  if (fd != -1) {
    return;
  }
  fd = ::open(mPath.c_str(), O_RDWR | O_NONBLOCK);
  if (-1 == fd) {
    throw std::system_error(errno, std::system_category(), "v4l2 open failed");
  }
  struct v4l2_capability cap;
  std::memset(&cap, 0, sizeof cap);
  if (-1 == ioctl(VIDIOC_QUERYCAP, &cap)) {
    throw std::system_error(errno, std::system_category(), "VIDIOC_QUERYCAP");
  }
}

void Device::close() {
  if (isOpen()) {
    streamOff();
    mQueuedBuffers = 0;
    mMap.clear();
    if (-1 == ::close(fd)) {
      throw std::system_error(errno, std::system_category(),
                              "v4l2 close failed");
    }
    fd = -1;
  }
}

bool Device::isOpen() const { return fd != -1; }

void Device::reopen() {
  if (isOpen()) {
    close();
  }
  open();
}

int Device::ioctl(unsigned long int request, void *argp) const {
  return ::ioctl(fd, request, argp);
}

bool Device::ready(std::chrono::milliseconds timeout) const {
  struct pollfd fds;
  std::memset(&fds, 0, sizeof fds);
  fds.events = POLLIN;
  fds.fd = fd;
  int poll_rv = ::poll(&fds, 1, timeout.count());
  bool rv = false;
  if (0 < poll_rv) {
    rv = POLLIN == fds.revents;
    if (fds.revents & POLLERR) {
      std::stringstream ss;
      ss << "v4l2 poll error " << std::hex << fds.revents;
      throw std::runtime_error(ss.str());
    }
  } else if (0 > poll_rv) {
    throw std::system_error(errno, std::system_category(), "v4l2 poll failed");
  }
  return rv;
}

void Device::setFormat(uint32_t width, uint32_t height, uint32_t format) {
  struct v4l2_format fmt;
  std::memset(&fmt, 0, sizeof fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = format;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  if (-1 == ioctl(VIDIOC_S_FMT, &fmt)) {
    throw std::system_error(errno, std::system_category(),
                            "VIDIOC_S_FMT failed");
  }
}

void Device::setFramerate(uint32_t framerate) {
  struct v4l2_streamparm parm;
  std::memset(&parm, 0, sizeof parm);
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == ioctl(VIDIOC_G_PARM, &parm)) {
    throw std::system_error(errno, std::system_category(),
                            "VIDIOC_G_PARM failed");
  }
  parm.parm.capture.timeperframe.numerator = 1;
  parm.parm.capture.timeperframe.denominator = framerate;
  if (-1 == ioctl(VIDIOC_S_PARM, &parm)) {
    throw std::system_error(errno, std::system_category(),
                            "VIDIOC_S_PARM failed");
  }
}

VideoSize Device::vsize() {
  struct v4l2_format fmt;
  std::memset(&fmt, 0, sizeof fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.field = V4L2_FIELD_ANY;
  if (-1 == ioctl(VIDIOC_G_FMT, &fmt)) {
    throw std::system_error(errno, std::system_category(),
                            "VIDIOC_G_FMT failed");
  }
  return {fmt.fmt.pix.width, fmt.fmt.pix.height};
}

int Device::streamOn() {
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  return ioctl(VIDIOC_STREAMON, &type);
}

int Device::streamOff() {
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  return ioctl(VIDIOC_STREAMOFF, &type);
}

uint32_t Device::requestBuffer(uint32_t count) {
  struct v4l2_requestbuffers rb;
  std::memset(&rb, 0, sizeof rb);
  rb.count = count;
  rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rb.memory = V4L2_MEMORY_MMAP;
  if (-1 == ioctl(VIDIOC_REQBUFS, &rb)) {
    throw std::system_error(errno, std::system_category(),
                            "requesting buffers failed");
  }
  return rb.count;
}

v4l2_buffer Device::bufferRequest(unsigned long int request, uint32_t index) {
  struct v4l2_buffer buf;
  std::memset(&buf, 0, sizeof buf);
  buf.index = index;
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  if (-1 == ioctl(request, &buf)) {
    throw std::system_error(errno, std::system_category(),
                            "buffer request failed");
  }
  return buf;
}

void Device::mmap() {
  const auto buffer_count = requestBuffer(V4L2_DEFAULT_BUFFER_COUNT);
  assert(buffer_count == V4L2_DEFAULT_BUFFER_COUNT);
  // Map the buffers
  for (std::uint32_t index = 0; index < buffer_count; ++index) {
    auto buf = bufferRequest(VIDIOC_QUERYBUF, index);
    assert(index == buf.index);
    void *ptr =
        ::mmap(nullptr, buf.length, PROT_READ, MAP_SHARED, fd, buf.m.offset);
    if (ptr == MAP_FAILED) {
      throw std::system_error(errno, std::system_category(),
                              "v4l2 mmap failed");
    }
    auto mem = Mem(index, ptr, buf.length);
    std::memcpy(&mem.video_buffer, &buf, sizeof(v4l2_buffer));
    mMap.emplace_back(std::move(mem));
  }
  mMap.shrink_to_fit();

  // Queue the buffers
  for (auto &mem : mMap) {
    if (queue(mem) != 1) {
      throw std::system_error(errno, std::system_category(),
                              "unable to queue buffer");
    }
  }
}

int Device::queue() {
  int count = 0;
  for (auto &mem : mMap) {
    auto rv = queue(mem);
    if (-1 == rv) {
      return -1;
    }
    count += rv;
  }
  return count;
}

int Device::queue(Mem &mem) {
  if (!mem.isUnused()) {
    return 0;
  }
  bufferRequest(VIDIOC_QBUF, mem.index);
  mem.state = Mem::State::QUEUED;
  ++mQueuedBuffers;
  return 1;
}

int Device::queue(std::size_t idx) { return queue(mMap[idx]); }

Device::Mem *Device::dequeue(std::chrono::milliseconds timeout) {
  if (!ready(timeout)) {
    return nullptr;
  }
  auto buf = bufferRequest(VIDIOC_DQBUF, 0);
  auto &mem = mMap[buf.index];
  std::memcpy(&mem.video_buffer, &buf, sizeof(v4l2_buffer));
  mem.state = Mem::State::READY;
  --mQueuedBuffers;
  return &mem;
}

Device::Mem::Mem(std::uint32_t index, void *ptr, std::size_t len)
    : index(index), ptr(ptr), len(len) {}

Device::Mem::~Mem() { ::munmap(ptr, len); }

Device::Mem::Mem(Mem &&other)
    : index(other.index), ptr(other.ptr), len(other.len) {
  other.ptr = nullptr;
  other.len = 0;
}

} // namespace h264camera
