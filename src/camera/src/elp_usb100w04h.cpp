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

#include <h264camera/elp_usb100w04h.hpp>

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>

#include <linux/usb/video.h>
#include <linux/uvcvideo.h>

#include "data_helper.hpp"

namespace h264camera {

// All sorts of constant values as defined in the ELP USB100W04H Linux
// SDK.

#define UVC_GUID_RERVISION_SYS_HW_CTRL                                         \
  {                                                                            \
    0x70, 0x33, 0xf0, 0x28, 0x11, 0x63, 0x2e, 0x4a, 0xba, 0x2c, 0x68, 0x90,    \
        0xeb, 0x33, 0x40, 0x16                                                 \
  }
#define UVC_GUID_RERVISION_USR_HW_CTRL                                         \
  {                                                                            \
    0x94, 0x73, 0xDF, 0xDD, 0x3E, 0x97, 0x27, 0x47, 0xBE, 0xD9, 0x04, 0xED,    \
        0x64, 0x26, 0xDC, 0x67                                                 \
  }

constexpr uint32_t V4L2_CID_BASE_EXTCTR_RERVISION = 0x0A0c4501;
constexpr uint32_t V4L2_CID_BASE_RERVISION = V4L2_CID_BASE_EXTCTR_RERVISION;
constexpr uint32_t V4L2_CID_ASIC_RW_RERVISION = V4L2_CID_BASE_RERVISION + 1;
constexpr uint32_t V4L2_CID_H264_CTRL_RERVISION = V4L2_CID_BASE_RERVISION + 4;

enum class XUUnit : uint8_t {
  RERVISION_SYS_ID = 0x03,
  RERVISION_USR_ID = 0x04
};

// ----- XU Control Selector -----

enum class XUSelector : uint8_t {
  SYS_ASIC_RW = 0x01,
  SYS_H264_CTRL = 0x07,
  USR_H264_CTRL = 0x02,
  USR_OSD_CTRL = 0x04,
  USR_IMG_SETTING = 0x06
};

template <XUUnit XUUNIT, XUSelector XUSELECTOR, std::uint8_t TAG,
          std::uint8_t REG, std::size_t XU_SIZE, typename TYPE>
struct Ctrl {
  static constexpr XUUnit unit{XUUNIT};
  static constexpr XUSelector selector{XUSELECTOR};
  static constexpr uint8_t tag{TAG};
  static constexpr uint8_t reg{REG};
  static constexpr std::size_t xu_size{XU_SIZE};
  typedef TYPE value_type;
};

using SysChipId = Ctrl<XUUnit::RERVISION_SYS_ID, XUSelector::SYS_ASIC_RW, 0x1F,
                       0x10, 4, ChipId>;
using CtrlBitRate = Ctrl<XUUnit::RERVISION_USR_ID, XUSelector::USR_H264_CTRL,
                         0x9A, 0x02, 11, int>;
using CtrlIFrame = Ctrl<XUUnit::RERVISION_USR_ID, XUSelector::USR_H264_CTRL,
                        0x9A, 0x04, 11, bool>;
using CtrlSEIHeader = Ctrl<XUUnit::RERVISION_USR_ID, XUSelector::USR_H264_CTRL,
                           0x9A, 0x05, 11, bool>;
using CtrlMode = Ctrl<XUUnit::RERVISION_USR_ID, XUSelector::USR_H264_CTRL, 0x9A,
                      0x06, 11, Mode>;
using SettingImgFlip = Ctrl<XUUnit::RERVISION_USR_ID,
                            XUSelector::USR_IMG_SETTING, 0x9A, 0x02, 11, bool>;
using SettingImgColor = Ctrl<XUUnit::RERVISION_USR_ID,
                             XUSelector::USR_IMG_SETTING, 0x9A, 0x03, 11, bool>;
using OsdCtrlRTC = Ctrl<XUUnit::RERVISION_USR_ID, XUSelector::USR_OSD_CTRL,
                        0x9A, 0x01, 11, TimeRTC>;

/// XU Controller access
template <typename CTRL> uvc_xu_control_query create_query(uint8_t *data) {
  struct uvc_xu_control_query xctrl;
  xctrl.unit = static_cast<uint8_t>(CTRL::unit);
  xctrl.selector = static_cast<uint8_t>(CTRL::selector);
  data[0] = CTRL::tag;
  data[1] = CTRL::reg;
  xctrl.size = CTRL::xu_size;
  xctrl.data = data;
  return xctrl;
}

template <typename CTRL> auto Usb100W04H::uvcGetCur() const {
  std::array<uint8_t, CTRL::xu_size> data{{0}};
  auto xctrl = create_query<CTRL>(data.data());

  xctrl.query = UVC_SET_CUR;
  if (-1 == ioctl(UVCIOC_CTRL_QUERY, &xctrl)) {
    throw std::system_error(errno, std::system_category(),
                            "UVCIOC_CTRL_QUERY failed");
  }

  data.fill(0);
  xctrl.query = UVC_GET_CUR;
  if (-1 == ioctl(UVCIOC_CTRL_QUERY, &xctrl)) {
    throw std::system_error(errno, std::system_category(),
                            "UVCIOC_CTRL_QUERY failed");
  }

  return convert_to<CTRL::xu_size, typename CTRL::value_type>(data);
}

template <typename CTRL>
void Usb100W04H::uvcSetCur(typename CTRL::value_type value) const {
  std::array<uint8_t, CTRL::xu_size> data{{0}};
  auto xctrl = create_query<CTRL>(data.data());

  xctrl.query = UVC_SET_CUR;
  if (-1 == ioctl(UVCIOC_CTRL_QUERY, &xctrl)) {
    throw std::system_error(errno, std::system_category(),
                            "UVCIOC_CTRL_QUERY failed");
  }

  convert_from<CTRL::xu_size, typename CTRL::value_type>(data, value);
  xctrl.query = UVC_SET_CUR;
  if (-1 == ioctl(UVCIOC_CTRL_QUERY, &xctrl)) {
    throw std::system_error(errno, std::system_category(),
                            "UVCIOC_CTRL_QUERY failed");
  }
}

// We do not have the C way of initializing structs :(
// NOTE: This mapping is incomplete, but suffices for now.
constexpr std::array<uvc_xu_control_mapping, 2> control_mapping = {
    {uvc_xu_control_mapping{V4L2_CID_ASIC_RW_RERVISION,
                            "RERVISION: Asic Read",
                            UVC_GUID_RERVISION_SYS_HW_CTRL,
                            static_cast<uint8_t>(XUSelector::SYS_ASIC_RW),
                            4,
                            0,
                            V4L2_CTRL_TYPE_INTEGER,
                            UVC_CTRL_DATA_TYPE_SIGNED,
                            nullptr,
                            0,
                            {0, 0, 0, 0}},
     uvc_xu_control_mapping{V4L2_CID_H264_CTRL_RERVISION,
                            "RERVISION: H264 Control",
                            UVC_GUID_RERVISION_USR_HW_CTRL,
                            static_cast<uint8_t>(XUSelector::USR_H264_CTRL),
                            11,
                            0,
                            V4L2_CTRL_TYPE_INTEGER,
                            UVC_CTRL_DATA_TYPE_UNSIGNED,
                            nullptr,
                            0,
                            {0, 0, 0, 0}}}};

void Usb100W04H::addXuCtrl() {
  for (auto mapping : control_mapping) {
    if (-1 == ioctl(UVCIOC_CTRL_MAP, &mapping)) {
      // The control need only to be set once, the if they are already
      // set for the device the ioctl call will fail with EEXIST and
      // everything is fine.
      if (EEXIST != errno) {
        throw std::system_error(errno, std::system_category(),
                                "setting xu controlls failed");
      }
    }
  }
}

double Usb100W04H::xuBitrate() const { return uvcGetCur<CtrlBitRate>(); }

void Usb100W04H::xuSetBitrate(double br) {
  int32_t rounded_bitrate = static_cast<int32_t>(br);
  uvcSetCur<CtrlBitRate>(rounded_bitrate);
}

Mode Usb100W04H::xuMode() const {
  auto mode = uvcGetCur<CtrlMode>();
  return static_cast<Mode>(mode);
}

void Usb100W04H::xuSetMode(Mode mode) { uvcSetCur<CtrlMode>(mode); }

bool Usb100W04H::xuSeiHeaderEnabled() const {
  return uvcGetCur<CtrlSEIHeader>();
}

void Usb100W04H::xuEnableSeiHeader(bool enable) {
  uvcSetCur<CtrlSEIHeader>(enable);
}

bool Usb100W04H::xuColor() const { return !uvcGetCur<SettingImgColor>(); }

void Usb100W04H::xuSetColor(bool enable) {
  uvcSetCur<SettingImgColor>(!enable);
}

void Usb100W04H::xuResetIFrame() { uvcSetCur<CtrlIFrame>(true); }

int Usb100W04H::xuReadChipId() { return uvcGetCur<SysChipId>().id; }

void Usb100W04H::xuUpdateOsdRtc() {
  uvcSetCur<OsdCtrlRTC>(TimeRTC{std::chrono::system_clock::now()});
}

void Usb100W04H::xuSetFlip(bool flip) { uvcSetCur<SettingImgFlip>(flip); }

bool Usb100W04H::xuFlip() { return uvcGetCur<SettingImgFlip>(); }

} // namespace h264camera
