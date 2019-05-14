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

#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/mswebcam.h>

#include <cassert>

namespace mselph264 {
extern MSFilterDesc filter_description;
extern MSWebCamDesc camera_description;
} // namespace mselph264

// This function is automatically called by Mediastreamer2.  The
// signature has to be 'void libms<name>_init(MSFactory *)' and the
// file name has to be 'libms<name>.so'.  From here the camera and
// filter is registered.
extern "C" void libmselph264_init(MSFactory *factory) {
  // bctbx_set_log_level(BCTBX_LOG_DOMAIN, BCTBX_LOG_DEBUG);
  assert(factory);
  ms_factory_register_filter(factory, &mselph264::filter_description);
  auto cam_manager = ms_factory_get_web_cam_manager(factory);
  assert(cam_manager);
  ms_web_cam_manager_register_desc(cam_manager, &mselph264::camera_description);
  ms_message("elph264 plugin registered");
}
