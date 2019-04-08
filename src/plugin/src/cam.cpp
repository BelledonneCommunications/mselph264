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

#include <mediastreamer2/mswebcam.h>

#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <h264camera/elp_usb100w04h.hpp>
#include <libv4l2.h>
#include <linux/videodev2.h>

#include "filter.hpp"

using namespace std::chrono_literals;

namespace mselph264 {

extern MSFilterDesc filter_description;

static void detect_camera(MSWebCamManager *obj);

// Create the capture filter using the camera device as source.
static MSFilter *create_reader(MSWebCam *cam) {
  MSFactory *factory = ms_web_cam_get_factory(cam);
  MSFilter *f =
      ms_factory_create_filter_from_desc(factory, &filter_description);
  auto s = State::from(f);
  s->setDevice(cam->name);
  return f;
}

// For now, we only answer to H264 requests.  The camera is capable of
// more formats.
static bool_t encode_to_mime_type(MSWebCam *, const char *mime_type) {
  return (strcmp(mime_type, "H264") == 0);
}

MSWebCamDesc camera_description = {"ELP-USB100W04H" /*driver_type*/,
                                   detect_camera,
                                   nullptr /* init */,
                                   create_reader,
                                   nullptr /* uninit */,
                                   encode_to_mime_type};

// Detect suitable candidates for this plugin.
static void detect_camera(MSWebCamManager *obj) {
  // See udev rules (elp-camera.rules)
  h264camera::Usb100W04H device("/dev/elp-h264");
  device.open();
  device.addXuCtrl();
  device.setFormat(MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H,
                   V4L2_PIX_FMT_H264);
  device.setFramerate(30);
  device.xuResetIFrame();
  device.mmap();
  device.streamOn();
  if (nullptr == device.dequeue(500ms)) {
    bctbx_warning("Unable to retrieve image in camera detection");
  }
  device.streamOff();
  device.close();
  MSWebCam *cam = ms_web_cam_new(&camera_description);
  cam->name = ms_strdup(device.path().c_str());
  ms_web_cam_manager_add_cam(obj, cam);
}

} // namespace mselph264
