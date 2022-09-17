#include <gst/gst.h>
#include "gst-webrtc-main.h"

int main(int argc, char *argv[])
{
  gst_init(&argc, &argv);

  std::string url = "ws://signaling:9449/";
  std::string origin = "localhost";
  std::string pipeline;

  if (argc > 1) {
    pipeline = std::string(argv[1]);
  } else {
    pipeline = "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 \
        videotestsrc is-live=true \
         ! videoconvert \
         ! queue \
         ! vp8enc target-bitrate=10240000 deadline=1 \
         ! rtpvp8pay \
         ! application/x-rtp,media=video,encoding-name=VP8,payload=96 \
         ! webrtcbin. \
        audiotestsrc is-live=true \
         ! audioconvert \
         ! audioresample \
         ! queue \
         ! opusenc \
         ! rtpopuspay \
         ! application/x-rtp,media=audio,encoding-name=OPUS,payload=97 \
         ! webrtcbin. ";
  }

  WebRTCMain main(pipeline);
  main.connectSignallingServer(url, origin);

  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  return 0;
}