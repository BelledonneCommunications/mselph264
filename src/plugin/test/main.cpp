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

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <bctoolbox/logging.h>

int main(int argc, char *argv[]) {
  bctbx_set_log_level("mediastreamer", BCTBX_LOG_WARNING);
  bctbx_set_log_level(BCTBX_LOG_DOMAIN, BCTBX_LOG_DEBUG);
  int result = Catch::Session().run(argc, argv);
  return result;
}