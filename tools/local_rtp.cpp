#include <cassert>
#include <iostream>

#include <bctoolbox/list.h>
#include <mediastreamer2/mediastream.h>
#include <mediastreamer2/msfactory.h>
#include <mediastreamer2/msrtp.h>

#define H264_PAYLOAD_TYPE 102

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

struct StreamParam {
  VideoStream *vs{nullptr};
  RtpSession *session{nullptr};
  const char *ip{"127.0.0.1"};
  int rtp_port{-1};
  int rtcp_port{-1};
  MSWebCam *cam{nullptr};
};

int main(int argc, const char *argv[]) {
  ortp_init();
  ortp_set_log_level_mask(
      ORTP_LOG_DOMAIN, ORTP_MESSAGE | ORTP_WARNING | ORTP_ERROR | ORTP_FATAL);

  auto factory =
      ms_factory_new_with_voip_and_directories(MS_PLUGIN_DIRECTORY, nullptr);
  assert(factory);

  // NOTE: You have to copy or link the msopenh264 into the MS_PLUGIN_DIRECTORY
  // directory.
  assert(ms_factory_codec_supported(factory, "H264"));

  RtpProfile rtp_profile;
  rtp_profile_clear_all(&rtp_profile);
  rtp_profile_set_payload(&rtp_profile, H264_PAYLOAD_TYPE, &payload_type_h264);
  int payload_type = H264_PAYLOAD_TYPE;

  float fps = 15.f;
  MSVideoSize vsize = MS_VIDEO_SIZE_720P;

  auto make_stream = [&]() {
    StreamParam p;
    p.vs = video_stream_new2(factory, p.ip, p.rtp_port, p.rtcp_port);
    assert(p.vs);
    p.session = video_stream_get_rtp_session(p.vs);
    assert(p.session);
    p.rtp_port = rtp_session_get_local_port(p.session);
    p.rtcp_port = rtp_session_get_local_rtcp_port(p.session);
    video_stream_set_freeze_on_error(p.vs, true);
    video_stream_set_fps(p.vs, fps);
    video_stream_set_sent_video_size(p.vs, vsize);
    return p;
  };

  StreamParam p1 = make_stream();
  auto cam_manager = ms_factory_get_web_cam_manager(factory);
  assert(cam_manager);
  p1.cam = find_camera(cam_manager);
  assert(p1.cam);

  StreamParam p2 = make_stream();

  video_stream_send_only_start(p1.vs, &rtp_profile, p2.ip, p2.rtp_port,
                               p2.rtcp_port, payload_type, 50, p1.cam);
  video_stream_recv_only_start(p2.vs, &rtp_profile, p1.ip, p1.rtp_port,
                               payload_type, 50);

  std::cout << "Press any key to stop\n";
  std::cin.get();

  video_stream_recv_only_stop(p2.vs);
  video_stream_send_only_stop(p1.vs);

  ms_factory_destroy(factory);
  return 0;
}