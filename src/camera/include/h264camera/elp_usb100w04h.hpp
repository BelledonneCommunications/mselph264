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

#ifndef USB100W04H_HPP__
#define USB100W04H_HPP__

#include <array>

#include "v4l2_device.hpp"
#include "types.hpp"

namespace h264camera {

/// UVC interface extensions for a ELP USB100W04H
class Usb100W04H : public Device {
public:
  using Device::Device;

  /// Read vendor chip id
  int xuReadChipId();
  /// Connecting the extended controls, has to be called at least once
  /// while the camera is plugged.
  void addXuCtrl();
  /// Get bitrate
  double xuBitrate() const;
  /// Set bitrate
  void xuSetBitrate(double br);
  /// Get mode (CBR, VBR)
  Mode xuMode() const;
  /// Set mode
  void xuSetMode(Mode mode);
  /// Check H.264 SEI header settings
  bool xuSeiHeaderEnabled() const;
  /// Set H.264 SEI header
  void xuEnableSeiHeader(bool enable);
  /// This method returns the register value for the color mode, but this does
  /// not alwyas represent the applied color.  To ensure color settings, apply
  /// while the stream is running.
  bool xuColor() const;
  /// Set the color mode.  If set to false, the output is in gray scale.
  void xuSetColor(bool enable);
  /// Request I-frame
  void xuResetIFrame();
  /// Set camera RTC to local time
  void xuUpdateOsdRtc();
  /// Set the flip state
  ///
  /// Setting false is normal orientation and setting true will flip the image
  /// on the H.264 stream.
  void xuSetFlip(bool flip);
  /// Get the flip state
  bool xuFlip();

private:
  template <typename CTRL> auto uvcGetCur() const;
  template <typename CTRL>
  void uvcSetCur(typename CTRL::value_type value) const;
};

} // namespace h264camera

#endif // USB100W04H_HPP__
