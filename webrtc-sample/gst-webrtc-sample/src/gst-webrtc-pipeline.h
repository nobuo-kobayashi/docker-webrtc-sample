#pragma once

#include <string>
#include <vector>
#include <gst/gst.h>

#include "gst-webrtc-data-channel.h"

class WebRTCPipeline;

class WebRTCPipelineListener {
public:
  virtual void onSendSdp(WebRTCPipeline *pipeline, gint type, gchar *sdp_string) {}
  virtual void onSendIceCandidate(WebRTCPipeline *pipeline, guint mlineindex, gchar *candidate) {}
  virtual void onAddStream(WebRTCPipeline *pipeline, GstPad *pad) {}

  virtual void onDataChannelConnected(WebRTCPipeline *pipeline) {}
  virtual void onDataChannelDisconnected(WebRTCPipeline *pipeline) {}
  virtual void onDataChannel(WebRTCPipeline *pipeline, std::string& message) {}
};

class WebRTCPipeline : public WebRTCDataChannelListener {
private:
  GstElement *mPipeline;
  GstElement *mWebRTCBin;
  WebRTCPipelineListener *mListener;
  WebRTCDataChannel *mSendDataChannel;
  std::vector<WebRTCDataChannel*> mReceiveDataChannels;
  
  gint mNegotiationNeededHandleId;
  gint mSendIceCandidateHandleId;
  gint mIceGatheringStateNotifyHandleId;
  gint mIncomingStreamHandleId;
  gint mDataChannelHandleId;

  void createReceiveDataChannel(GstWebRTCDataChannel *dataChannel);
  void sendSdp(GstWebRTCSessionDescription *desc);
  void sendIceCandidate(guint mlineindex, gchar *candidate);
  void addStream(GstPad *pad);
  void onOfferReceived(GstSDPMessage *sdp);
  void onAnswerReceived(GstSDPMessage *sdp);

  static void onNegotiationNeeded(GstElement *webrtcbin, gpointer userData);
  static void onSendIceCandidate(GstElement *webrtcbin, guint mlineindex, gchar *candidate, gpointer userData);
  static void onIceGatheringStateNotify(GstElement *webrtcbin, GParamSpec *pspec, gpointer userData);
  static void onIncomingStream(GstElement *webrtcbin, GstPad *pad, gpointer userData);
  static void onDataChannel(GstElement *webrtcbin, GObject *dataChannel, gpointer userData);
  static void onOfferCreated(GstPromise *promise, gpointer userData);
  static void onAnswerCreated(GstPromise *promise, gpointer userData);

public:
  WebRTCPipeline();
  virtual ~WebRTCPipeline();

  inline void setListener(WebRTCPipelineListener *listener) {
    mListener = listener;
  }

  void startPipeline(std::string& bin);
  void stopPipeline();
  void sendMessage(std::string& message);

  void onOfferReceived(const gchar *sdp);
  void onAnswerReceived(const gchar *sdp);
  void onIceReceived(guint mlineIndex, const gchar *candidateString);

  // WebRTCDataChannelListener implements.
  virtual void onConnected(WebRTCDataChannel *channel);
  virtual void onDisconnected(WebRTCDataChannel *channel);
  virtual void onMessage(WebRTCDataChannel *channel, std::string& message);
};
