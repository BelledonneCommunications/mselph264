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

#ifndef PLUGIN_H264_HELPER_HPP__
#define PLUGIN_H264_HELPER_HPP__

#include <mediastreamer2/msqueue.h>
#include <h264camera/v4l2_device.hpp>

namespace mselph264 {

using h264camera::Device;

/// Split raw h264 data into nalus and store into a queue of mblk_t.
void separate_h264_nalus(Device::Mem *mem, MSQueue *nalus);

} // namespace mselph264

#endif
