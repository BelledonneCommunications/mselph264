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

#include "helper.hpp"

#include <bctoolbox/list.h>

MSWebCam *find_camera(MSWebCamManager *manager) {
  auto cam_list = ms_web_cam_manager_get_list(manager);
  MSWebCam *cam{nullptr};
  bctbx_list_for_each2(
      cam_list,
      [](void *ptr, void *data) {
        auto cam = static_cast<MSWebCam *>(ptr);
        auto rv = static_cast<MSWebCam **>(data);

        if (strcmp("ELP-USB100W04H", ms_web_cam_get_driver_type(cam)) == 0) {
          ms_message("Found camera %s", ms_web_cam_get_string_id(cam));
          *rv = cam;
        }
      },
      &cam);
  return cam;
}