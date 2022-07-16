#pragma once

#include <string>
#include <vector>
#include <gst/gst.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

class WebRTCDataChannel;

class WebRTCDataChannelListener {
public:
  virtual void onConnected(WebRTCDataChannel *channel) {}
  virtual void onDisconnected(WebRTCDataChannel *channel) {}
  virtual void onMessage(WebRTCDataChannel *channel, std::string& message) {}
};

class WebRTCDataChannel {
private:
  GstElement *mWebRTCBin;
  GstWebRTCDataChannel *mDataChannel;
  WebRTCDataChannelListener *mListener;

  gint mOpenHandleId;
  gint mCloseHandleId;
  gint mMessageHandleId;
  gint mErrorHandleId;

  void setCallback();

  static void onError(GObject *dc, gpointer userData);
  static void onOpen(GObject *dc, gpointer userData);
  static void onClose(GObject *dc, gpointer userData);
  static void onMessageString(GObject *dc, gchar *message, gpointer userData);

public:
  WebRTCDataChannel(GstElement *webrtcbin);
  virtual ~WebRTCDataChannel();

  inline void setListener(WebRTCDataChannelListener *listener) {
    mListener = listener;
  }

  void connect(std::string& name);
  void connect(GstWebRTCDataChannel *dataChannel);
  void disconnect();
  void sendMessage(std::string& message);
};
