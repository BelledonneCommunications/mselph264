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

#ifndef V4L2_DEVICE_HPP__
#define V4L2_DEVICE_HPP__

#include <chrono>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <string>
#include <vector>

namespace h264camera {

struct VideoSize {
  unsigned int width{0};
  unsigned int height{0};
};

constexpr inline bool operator==(const VideoSize &a, const VideoSize &b) {
  return (a.width == b.width && a.height == b.height);
}

constexpr VideoSize VIDEO_SIZE_720P{1280, 720};
constexpr VideoSize VIDEO_SIZE_SVGA{800, 600};
constexpr VideoSize VIDEO_SIZE_VGA{640, 480};

/// This class will handle the camera device descriptor and wrap v4l2
/// calls.
///
/// https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/user-func.html
class Device {
public:
  /// Constructor opening the device using v4l2_open
  /// @param dev Device path, e.g., /dev/video0
  Device(const std::string &dev_path);
  /// Destructor closing the device using v4l2_close
  ~Device();
  /// Open device
  void open();
  /// Close device
  void close();
  /// Check if device is open
  bool isOpen() const;
  /// Open the device, after closing when necessary
  void reopen();

  /// @see v4l2_ioctl
  int ioctl(unsigned long int request, void *argp) const;

  /// Create memory map
  void mmap();

  /// Wait for new data to be ready
  bool ready(std::chrono::milliseconds timeout) const;

  /// Return the device path
  inline const std::string &path() const { return mPath; }

  // Some convenience functions
  void setFormat(uint32_t width, uint32_t height, uint32_t format);
  void setFramerate(uint32_t framerate);

  VideoSize vsize();

  /// Enable capture stream
  int streamOn();
  /// Disable capture stream
  int streamOff();

  struct Mem {
    Mem(std::uint32_t index, void *ptr, std::size_t len);
    ~Mem();
    Mem(Mem &&);
    /// - UNUSED : Ready to be queued again
    /// - READY : Dequeud and has data to be processed
    /// - QUEUED : Wait for new data
    enum class State { UNUSED, READY, QUEUED } state{State::UNUSED};
    /// The index within the vector
    const std::uint32_t index;
    /// Pointer to the mapped memory
    void *ptr{nullptr};
    /// Length of the buffer
    std::uint32_t len{0};
    /// Count of bytes with data in the buffer
    v4l2_buffer video_buffer;
    std::uint32_t used() const { return video_buffer.bytesused; }
    /// Call this when done with the buffered data so it might be
    /// requeued
    inline void done() { state = State::UNUSED; }
    inline bool isUnused() const { return state == State::UNUSED; }
    inline bool isReady() const { return state == State::READY; }
    inline bool isQueued() const { return state == State::QUEUED; }
  };

  /// Check if there are still queued buffers
  bool queued() const { return mQueuedBuffers > 0; }
  /// Indicate if there a buffers ready for queuing.
  bool freeSlots() const { return mQueuedBuffers < mMap.size(); }

  /// Try and queue all unqueued buffer
  int queue();
  /// Queue a given frame
  int queue(std::size_t index);
  /// Dequeue a buffer a ready buffer
  Mem *dequeue(std::chrono::milliseconds timeout);

private:
  uint32_t requestBuffer(uint32_t count);
  v4l2_buffer bufferRequest(unsigned long int request, uint32_t index);
  int queue(Mem &mem);

  static const uint32_t V4L2_DEFAULT_BUFFER_COUNT{6};
  /// Device path string
  const std::string mPath;
  /// File descriptor
  int fd{-1};
  /// The buffers
  std::vector<Mem> mMap;
  /// Count and track queued buffers
  std::size_t mQueuedBuffers{0};
};

} // namespace h264camera

#endif // V4L2_DEVICE_HPP__
