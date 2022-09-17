#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "gst-webrtc-pipeline.h"
#include "gst-websocket-client.h"

using json = nlohmann::json;

class WebRTCMain : public WebsocketClientListener, WebRTCPipelineListener {
private:
  WebsocketClient *mClient;
  WebRTCPipeline *mPipeline;
  std::string mPipelineStr;

  void startPipeline();
  void stopPipeline();
  void praseSdpAndIce(std::string& message);
  void parseSdp(json& data_json_object);
  void parseIce(json& data_json_object);

public:
  WebRTCMain(std::string& pipelineStr);
  virtual ~WebRTCMain();

  void connectSignallingServer(std::string& url, std::string& origin);
  void disconnectSignallingServer();

  // WebsocketClientListener implements.
  virtual void onConnected(WebsocketClient *client);
  virtual void onDisconnected(WebsocketClient *client);
  virtual void onMessage(WebsocketClient *client, std::string& message);

  // WebRTCPipelineListener implements.
  virtual void onSendSdp(WebRTCPipeline *pipeline, gint type, gchar *sdp_string);
  virtual void onSendIceCandidate(WebRTCPipeline *pipeline, guint mlineindex, gchar *candidate);
  virtual void onAddStream(WebRTCPipeline *pipeline, GstPad *pad);
  virtual void onDataChannelConnected(WebRTCPipeline *pipeline);
  virtual void onDataChannelDisconnected(WebRTCPipeline *pipeline);
  virtual void onDataChannel(WebRTCPipeline *pipeline, std::string& message);
};
