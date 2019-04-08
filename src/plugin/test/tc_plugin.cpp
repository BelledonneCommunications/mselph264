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

#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msfactory.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msvideo.h>
#include <mediastreamer2/mswebcam.h>
#include <ortp/rtpprofile.h>

#include "helper.hpp"
#include "utils.hpp"

#include <catch.hpp>

const int SUCCESS = 0;

#define H264_PAYLOAD_TYPE 102

TEST_CASE("Load plugin", "[plugin][camera]") {
  auto factory = ms_factory_new();
  REQUIRE(factory);
  INFO("Load plugins from MS_PLUGIN_DIRECTORY=" MS_PLUGIN_DIRECTORY);
  ms_factory_set_plugins_dir(factory, MS_PLUGIN_DIRECTORY);

  { // Setup a webcam manager manually
    auto wm = ms_web_cam_manager_new();
    wm->factory = factory;
    factory->wbcmanager = wm;
    REQUIRE(wm);
  }

  ms_factory_init_plugins(factory);

  SECTION("Find plugin") {
    CHECK(ms_factory_lookup_filter_by_name(factory, "h264camera"));
  }

  SECTION("Supported format") {
    CHECK(ms_factory_codec_supported(factory, "H264"));
  }

  SECTION("Check for methods") {
    auto cam_manager = ms_factory_get_web_cam_manager(factory);
    REQUIRE(cam_manager);

    auto cam = find_camera(cam_manager);
    REQUIRE(cam);

    auto capture = ms_web_cam_create_reader(cam);
    REQUIRE(capture);

    CHECK(ms_filter_has_method(capture, MS_FILTER_GET_PIX_FMT));
    CHECK(
        ms_filter_has_method(capture, MS_VIDEO_ENCODER_GET_CONFIGURATION_LIST));
    CHECK(ms_filter_has_method(capture, MS_VIDEO_ENCODER_SET_CONFIGURATION));
    CHECK(ms_filter_has_method(capture, MS_VIDEO_ENCODER_REQ_VFU));
    CHECK(ms_filter_has_method(capture, MS_VIDEO_ENCODER_NOTIFY_PLI));
    CHECK(ms_filter_has_method(capture, MS_VIDEO_ENCODER_NOTIFY_FIR));
  }

  SECTION("Manipulate settings") {
    auto cam_manager = ms_factory_get_web_cam_manager(factory);
    REQUIRE(cam_manager);

    auto cam = find_camera(cam_manager);
    REQUIRE(cam);

    auto capture = ms_web_cam_create_reader(cam);
    REQUIRE(capture);

    MSPixFmt pix_fmt;
    REQUIRE(ms_filter_call_method(capture, MS_FILTER_GET_PIX_FMT, &pix_fmt) ==
            SUCCESS);
    CHECK(pix_fmt == MS_H264);
  }

  SECTION("Test MSVideoConfiguration interface") {
    auto cam_manager = ms_factory_get_web_cam_manager(factory);
    REQUIRE(cam_manager);

    auto cam = find_camera(cam_manager);
    REQUIRE(cam);

    auto capture = ms_web_cam_create_reader(cam);
    REQUIRE(capture);

    const MSVideoConfiguration *vconf_list = nullptr;
    REQUIRE(ms_filter_call_method(capture,
                                  MS_VIDEO_ENCODER_GET_CONFIGURATION_LIST,
                                  &vconf_list) == SUCCESS);
    REQUIRE(vconf_list);
    REQUIRE(vconf_list[0].required_bitrate > 0);

    MSVideoConfiguration vconf;
    REQUIRE(ms_filter_call_method(capture, MS_VIDEO_ENCODER_GET_CONFIGURATION,
                                  &vconf) == SUCCESS);
    CHECK(1280 == vconf.vsize.width);
    CHECK(720 == vconf.vsize.height);

    vconf.vsize.width = 800;
    vconf.vsize.height = 600;
    REQUIRE(ms_filter_call_method(capture, MS_VIDEO_ENCODER_SET_CONFIGURATION,
                                  &vconf) == SUCCESS);
  }

  ms_factory_destroy(factory);
}

TEST_CASE("Start stream", "[plugin][camera]") {
  auto factory = ms_factory_new();
  REQUIRE(factory);
  ms_factory_init_voip(factory);
  INFO("Load plugins from MS_PLUGIN_DIRECTORY=" MS_PLUGIN_DIRECTORY);
  ms_factory_set_plugins_dir(factory, MS_PLUGIN_DIRECTORY);
  ms_factory_init_plugins(factory);

  auto cam_manager = ms_factory_get_web_cam_manager(factory);
  REQUIRE(cam_manager);
  auto cam = find_camera(cam_manager);
  REQUIRE(cam);
  auto capture = ms_web_cam_create_reader(cam);
  REQUIRE(capture);

  rtp_profile_set_payload(&av_profile, 102, &payload_type_h264);

  auto stream = video_stream_new2(factory, "127.0.0.1", 50001, 50002);
  REQUIRE(stream);
  video_stream_set_direction(stream, MediaStreamSendOnly);
  video_stream_set_freeze_on_error(stream, true);
  video_stream_set_sent_video_size(stream, MS_VIDEO_SIZE_720P);
  video_stream_set_fps(stream, 30);

  CHECK(video_stream_start_with_source(stream, &av_profile, "127.0.0.1", 50003,
                                       "127.0.0.1", 50004, 102, 0, cam,
                                       capture) == SUCCESS);
  CHECK(MS_VIDEO_SIZE_720P == video_stream_get_sent_video_size(stream));
  CHECK(30 == video_stream_get_sent_framerate(stream));

  video_stream_iterate(stream);
  video_stream_send_only_stop(stream);
}
