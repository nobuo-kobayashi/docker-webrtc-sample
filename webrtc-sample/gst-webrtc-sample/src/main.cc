#include <gst/gst.h>
#include "gst-webrtc-main.h"

int main(int argc, char *argv[])
{
  gst_init(&argc, &argv);

  std::string url = "ws://127.0.0.1:9449/";
  std::string origin = "localhost";

  WebRTCMain main;
  main.connectSignallingServer(url, origin);

  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  return 0;
}