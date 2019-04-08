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

#ifndef PLUGIN_FILTER_HPP__
#define PLUGIN_FILTER_HPP__

#include <array>
#include <atomic>
#include <mediastreamer2/mscodecutils.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/rfc3984.h>
#include <memory>
#include <string>
#include <thread>

namespace h264camera {
	class Usb100W04H;
}

namespace mselph264 {

/// Structure holding all relevant data during the lifetime of the
/// filter.  It is to be created in the init method and will be
/// destroyed on uninit.  The reference is stored int filter->data.
class State {
public:
  State(MSFilter *filter);

  /// Extract the State from the userdata in a MSFilter
  static State *from(MSFilter *filter);
  /// Set the video device string. Device will be opened during the
  /// preprocessing.
  void setDevice(const std::string &path);

  void preprocess();
  void process();
  void postprocess();

  const MSVideoConfiguration *videoConfList();
  MSVideoConfiguration videoConf() const { return mVideoConf; }
  void setVideoConf(MSVideoConfiguration conf);

  MSVideoSize vsize() const { return mVideoConf.vsize; }
  float fps() const { return mVideoConf.fps; }

  void requestVFU();
  void notifyPLI();
  void notifyFIR();

  // Just guessuing. CPU count does not matter, since the hardware encoder does
  // all the hard work.
  static constexpr std::array<MSVideoConfiguration, 8> sVideoConfList{
      {MS_VIDEO_CONF(2048000, 10000000, 720P, 30, 0),
       MS_VIDEO_CONF(1024000, 5000000, 720P, 20, 0),
       MS_VIDEO_CONF(750000, 2048000, SVGA, 20, 0),
       MS_VIDEO_CONF(500000, 1024000, SVGA, 15, 0),
       MS_VIDEO_CONF(256000, 800000, VGA, 15, 0),
       MS_VIDEO_CONF(128000, 512000, VGA, 10, 0),
       MS_VIDEO_CONF(100000, 380000, QVGA, 10, 0),
       MS_VIDEO_CONF(0, 0, UNKNOWN, 0, 0)}};

private:
  void captureLoop();

  MSFilter *mFilter{nullptr};
  /// The camera device.  It will be set by the create_reader call in the
  /// MSWebCamDesc.
  std::unique_ptr<h264camera::Usb100W04H> mDevice;
  /// Selected configuration
  MSVideoConfiguration mVideoConf;

  std::thread mCaptureThread;
  // The capture loop will stop, if this is false.
  std::atomic<bool> mRunning{false};
  // The capture loop will restart after exit, if this is true.
  std::atomic<bool> mReconfigure{true};
  MSQueue mNalus;
  Rfc3984Context *mPacker{nullptr};
  MSVideoStarter mVideoStarter;
  MSIFrameRequestsLimiterCtx mIFrameLimiter;
};

} // namespace mselph264

#endif
