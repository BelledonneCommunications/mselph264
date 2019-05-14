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

#include "filter.hpp"

#include <bctoolbox/logging.h>
#include <cassert>
#include <chrono>
#include <cstring>
#include <functional>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msticker.h>
#include <mediastreamer2/msvideo.h>

#include "h264camera/elp_usb100w04h.hpp"
#include "h264helper.hpp"
#include "utils.hpp"


using namespace h264camera;
using namespace std::chrono_literals;

namespace mselph264 {

/// MSFilter is locked during lifetime of FilterLock
struct FilterLock {
  FilterLock(MSFilter *f) : filter(f) { ms_filter_lock(filter); }
  ~FilterLock() { ms_filter_unlock(filter); }
  MSFilter *filter{nullptr};
};

State::State(MSFilter *filter)
    : mFilter(filter), mVideoConf(ms_video_find_best_configuration_for_size(
                           sVideoConfList.data(), MS_VIDEO_SIZE_720P, 1)) {
  bctbx_debug("Start vconf: %dbit/s %dbit/s %dx%d %ffps mincpu: %d extra: %p",
              mVideoConf.required_bitrate, mVideoConf.bitrate_limit,
              mVideoConf.vsize.width, mVideoConf.vsize.height, mVideoConf.fps,
              mVideoConf.mincpu, mVideoConf.extra);
}

State *State::from(MSFilter *filter) {
  assert(filter);
  assert(filter->data);
  return static_cast<State *>(filter->data);
}

void State::setDevice(const std::string &path) {
  bctbx_message("Set camera device %s", path.c_str());
  mDevice = std::make_unique<h264camera::Usb100W04H>(path);
}

void State::captureLoop() {
  assert(mDevice);
  ms_message("Start capture loop");
  try {
    while (mReconfigure) {
      // Configure camera berfore opening the stream
      MSVideoConfiguration vconf;
      {
        FilterLock lock(mFilter);
        vconf = mVideoConf;
        mReconfigure = false;
        mRunning = true;
      }
      bctbx_message(
          "Reconfigure %dbit/s %dbit/s %dx%d %ffps mincpu: %d extra: %p",
          vconf.required_bitrate, vconf.bitrate_limit, vconf.vsize.width,
          vconf.vsize.height, vconf.fps, vconf.mincpu, vconf.extra);
      mDevice->reopen();
      mDevice->xuEnableSeiHeader(true);
      mDevice->setFormat(vconf.vsize.width, vconf.vsize.height,
                         V4L2_PIX_FMT_H264);
      mDevice->setFramerate(vconf.fps);
      mDevice->mmap();

      ms_video_starter_first_frame(&mVideoStarter, mFilter->ticker->time);
      mDevice->streamOn();

      // The camera is mounted upside down in SmartConnect door, so flip it
      // before sending images.
      //
      // Toggle flip option between frames to take effect
      if (auto frame = mDevice->dequeue(500ms)) {
        frame->done();
        mDevice->queue(frame->index);
      }
      mDevice->xuSetFlip(false);
      if (auto frame = mDevice->dequeue(500ms)) {
        frame->done();
        mDevice->queue(frame->index);
      }
      mDevice->xuSetFlip(true);
      if (auto frame = mDevice->dequeue(500ms)) {
        frame->done();
        mDevice->queue(frame->index);
      }
      mDevice->xuResetIFrame();

      while (mRunning) {
        if (ms_video_starter_need_i_frame(&mVideoStarter,
					  mFilter->ticker->time) ||
           ms_iframe_requests_limiter_iframe_requested(
						       &mIFrameLimiter,
						       mFilter->ticker->time)) {
	   mDevice->xuResetIFrame();
	   ms_iframe_requests_limiter_notify_iframe_sent(&mIFrameLimiter,
							mFilter->ticker->time);
        }

        if (auto frame = mDevice->dequeue(200ms)) {
	  MSQueue *nalus = getMSQueue();
	  separate_h264_nalus(frame, nalus);

	  mCaptureMutex.lock();
	  if (!ms_queue_empty(nalus)) {
	    mNalusQueue.push(nalus);
	  } else {
	    mAvailableNalus.push(nalus);
	  }
	  mCaptureMutex.unlock();
	  mDevice->queue(frame->index);
        } else {
          bctbx_warning("Timeout when waiting for a new frame");
        }
      }
      mDevice->streamOff();
    }
    mDevice->close();
  } catch (const std::exception &e) {
    ms_error("Something went wrong in the capture loop %s", e.what());
  }
  ms_message("Stop capture loop");
}

void State::preprocess() {
  assert(mDevice);

  mPacker = rfc3984_new_with_factory(mFilter->factory);
  rfc3984_set_mode(mPacker, 1);
  ms_video_starter_init(&mVideoStarter);
  ms_iframe_requests_limiter_init(&mIFrameLimiter, 1000);

  mCaptureThread = std::thread(&State::captureLoop, this);
}

void State::process() {
  // rtp uses a 90 kHz clockrate for video
  auto timestamp = mFilter->ticker->time * 90;

  if (mCaptureMutex.try_lock()) {
    while (!mNalusQueue.empty()) {
      MSQueue *nalus = mNalusQueue.front();
      if (!ms_queue_empty(nalus)) {
        rfc3984_pack(mPacker, nalus, mFilter->outputs[0], timestamp);
      }
      mNalusQueue.pop();
      ms_queue_flush(nalus);
      mAvailableNalus.push(nalus);
    }
    mCaptureMutex.unlock();
  }
}

void State::postprocess() {
  mReconfigure = false;
  mRunning = false;
  mCaptureThread.join();

  rfc3984_destroy(mPacker);

  while (!mNalusQueue.empty()) {
    MSQueue *nalus = mNalusQueue.front();
    ms_queue_flush(nalus);
    delete nalus;
    mNalusQueue.pop();
  }
  while (!mAvailableNalus.empty()) {
    MSQueue *nalus = mAvailableNalus.front();
    ms_queue_flush(nalus);
    delete nalus;
    mAvailableNalus.pop();
  }
}

MSQueue *State::getMSQueue() {
  MSQueue *queue;

  if (!mAvailableNalus.empty()) {
    queue = mAvailableNalus.front();
    mAvailableNalus.pop();
  } else {
    queue = new MSQueue();
    ms_queue_init(queue);
  }
  return queue;
}

const MSVideoConfiguration *State::videoConfList() {
  return sVideoConfList.data();
}

void State::setVideoConf(MSVideoConfiguration conf) {
  bctbx_set_log_level("mediastreamer", BCTBX_LOG_MESSAGE);
  if ((conf.vsize.width == 1280 && conf.vsize.height == 720) ||
      (conf.vsize.width == 800 && conf.vsize.height == 600) ||
      (conf.vsize.width == 640 && conf.vsize.height == 480)) {
    {
      FilterLock lock(mFilter);
      if (conf.vsize != mVideoConf.vsize || conf.fps != mVideoConf.fps) {
        mReconfigure = true;
        mRunning = false;
      }
      mVideoConf = conf;
    }
  } else {
    bctbx_error("Passing invalid resolution: %dx%d", conf.vsize.width,
                conf.vsize.height);
  }
}

void State::requestVFU() {
  FilterLock lock(mFilter);
  ms_video_starter_deactivate(&mVideoStarter);
  ms_iframe_requests_limiter_request_iframe(&mIFrameLimiter);
}

void State::notifyPLI() { requestVFU(); }
void State::notifyFIR() { requestVFU(); }

// Define callbacks for various filter method calls.
// Implement methods listed in msinterfaces.h listed for
// MSFilterVideoEncoderInterface
static MSFilterMethod filter_method_table[] = {
    {MS_FILTER_GET_PIX_FMT,
     [](MSFilter *, void *arg) -> int {
       bctbx_debug("Filter method: MS_FILTER_GET_PIX_FMT");
       auto pixfmt = static_cast<MSPixFmt *>(arg);
       *pixfmt = MS_H264;
       return 0;
     }},
    {MS_FILTER_SET_PIX_FMT,
     [](MSFilter *, void *arg) -> int {
       auto pixfmt = static_cast<const MSPixFmt *>(arg);
       bctbx_debug("Filter method: MS_FILTER_SET_PIX_FMT %d", *pixfmt);
       if (*pixfmt != MS_H264) {
         ms_error("This filter does only support H264.");
         return -1;
       }
       return 0;
     }},
    {MS_VIDEO_ENCODER_SUPPORTS_PIXFMT,
     [](MSFilter *, void *arg) -> int {
       auto enc_support = static_cast<MSVideoEncoderPixFmt *>(arg);
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_SUPPORTS_PIXFMT arg=%u",
                   enc_support->pixfmt);
       enc_support->supported = (enc_support->pixfmt == MS_H264);
       return enc_support->pixfmt ? 0 : -1;
     }},
    {MS_VIDEO_ENCODER_GET_CONFIGURATION,
     [](MSFilter *f, void *arg) -> int {
       auto state = State::from(f);
       auto vconf = static_cast<MSVideoConfiguration *>(arg);
       *vconf = state->videoConf();
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_GET_CONFIGURATION "
                   "arg=(%dbit/s %dbit/s %dx%d %ffps mincpu: %d extra: %p)",
                   vconf->required_bitrate, vconf->bitrate_limit,
                   vconf->vsize.width, vconf->vsize.height, vconf->fps,
                   vconf->mincpu, vconf->extra);
       return 0;
     }},
    {MS_VIDEO_ENCODER_SET_CONFIGURATION,
     [](MSFilter *f, void *arg) -> int {
       auto state = State::from(f);
       auto vconf = static_cast<const MSVideoConfiguration *>(arg);
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_SET_CONFIGURATION "
                   "arg=(%dbit/s %dbit/s %dx%d %ffps mincpu: %d extra: %p)",
                   vconf->required_bitrate, vconf->bitrate_limit,
                   vconf->vsize.width, vconf->vsize.height, vconf->fps,
                   vconf->mincpu, vconf->extra);
       state->setVideoConf(*vconf);
       return 0;
     }},
    {MS_VIDEO_ENCODER_REQ_VFU,
     [](MSFilter *f, void *) -> int {
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_REQ_VFU");
       auto state = State::from(f);
       state->requestVFU();
       return 0;
     }},
    {MS_VIDEO_ENCODER_NOTIFY_PLI,
     [](MSFilter *f, void *) -> int {
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_NOTIFY_PLI");
       auto state = State::from(f);
       state->notifyPLI();
       return 0;
     }},
    {MS_VIDEO_ENCODER_NOTIFY_FIR,
     [](MSFilter *f, void *) -> int {
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_NOTIFY_FIR");
       auto state = State::from(f);
       state->notifyFIR();
       return 0;
     }},
    {MS_VIDEO_ENCODER_ENABLE_AVPF,
     [](MSFilter *, void *arg) -> int {
       auto enable_avpf = static_cast<bool *>(arg);
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_ENABLE_AVPF %d",
                   *enable_avpf);
       return 0;
     }},
    {MS_VIDEO_ENCODER_IS_HARDWARE_ACCELERATED,
     [](MSFilter *, void *arg) -> int {
       bctbx_debug("Filter method: MS_VIDEO_ENCODER_IS_HARDWARE_ACCELERATED");
       auto accelerated = static_cast<bool *>(arg);
       *accelerated = true;
       return 0;
     }},
    {0, nullptr}};

// Struct with all filter related callbacks and information.  It is
// used by the camera filter initialization.
MSFilterDesc filter_description = {
    MS_FILTER_PLUGIN_ID /*id*/,
    "h264camera" /*name*/,
    "Access ELP USB100W04H with H.264 encoder" /*text*/,
    MS_FILTER_ENCODING_CAPTURER /*category*/,
    "H264" /*enc_fmt*/,
    0 /*ninputs*/,
    1 /*noutputs*/,
    [](MSFilter *f) { f->data = new State(f); } /*init*/,
    [](MSFilter *f) { State::from(f)->preprocess(); } /*preprocess*/,
    [](MSFilter *f) { State::from(f)->process(); } /*process*/,
    [](MSFilter *f) { State::from(f)->postprocess(); } /*postprocess*/,
    [](MSFilter *f) {
      delete State::from(f);
      f->data = nullptr;
    } /*uninit*/,
    filter_method_table,
    0 /*flags*/};

} // namespace mselph264
