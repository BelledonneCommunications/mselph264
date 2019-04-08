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

#include "h264helper.hpp"

#include <algorithm>
#include <iterator>

using namespace std;

namespace mselph264 {

// union nal_unit_t {
// uint8_t raw;
// struct {
// unsigned char Type : 5; // nal unit type
// unsigned char NRI : 2;  // nal ref idc
// unsigned char F : 1;    // forbidden zero
// };
// };
// static_assert(sizeof(nal_unit_t) == 1, "Bit field is not packed");

void push_nalu(const uint8_t *begin, const uint8_t *end, MSQueue *nalus) {
  mblk_t *m = allocb(distance(begin, end), 0);
  while (begin != end) {
    *m->b_wptr = *begin;
    ++begin;
    ++m->b_wptr;
  }
  ms_queue_put(nalus, m);
}

void separate_h264_nalus(Device::Mem *mem, MSQueue *nalus) {
  size_t zero_cnt{0};

  const uint8_t *unit_begin = nullptr;
  auto cur = static_cast<const uint8_t *>(mem->ptr);
  for (size_t idx = 0; idx < mem->used(); ++idx) {
    if (*cur == 0) {
      ++zero_cnt;
    } else if (zero_cnt >= 3 && *cur == 1) {
      if (unit_begin) {
        push_nalu(unit_begin, cur, nalus);
      }
      unit_begin = cur + 1;
    } else {
      zero_cnt = 0;
    }
    ++cur;
  }
  if (unit_begin) {
    push_nalu(unit_begin, cur, nalus);
  }
  mem->done();
}

} // namespace mselph264
