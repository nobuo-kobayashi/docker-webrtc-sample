#include "gst-webrtc-main.h"

WebRTCMain::WebRTCMain()
{
  mClient = nullptr;
  mPipeline = nullptr;
}

WebRTCMain::~WebRTCMain()
{
  stopPipeline();
  disconnectSignallingServer();
}

void WebRTCMain::connectSignallingServer(std::string& url, std::string& origin)
{
  disconnectSignallingServer();

  mClient = new WebsocketClient();
  mClient->setListener(this);
  mClient->connectAsync(url, origin);
}

void WebRTCMain::disconnectSignallingServer()
{
  if (mClient) {
    delete mClient;
    mClient = nullptr;
  }
}

// private functions.

void WebRTCMain::startPipeline()
{
  stopPipeline();

  // webrtcbin エレメント名前は固定にしておく必要があります
  // webrtcbin name=webrtcbin を変更する場合には、呼び出している箇所も全て変更する必要があります。
  // std::string bin = "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 
  std::string bin = "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 turn-server=turn://username:password@192.168.2.99:3479 \
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

  mPipeline = new WebRTCPipeline();
  mPipeline->setListener(this);
  mPipeline->startPipeline(bin);
}

void WebRTCMain::stopPipeline()
{
  if (mPipeline) {
    delete mPipeline;
    mPipeline = nullptr;
  }
}

/**
 * Websocket で送られてきた ICE or SDP の情報を webrtcbin に渡します。
 * 
 * ICE 情報が格納された JSON オブジェクトのフォーマットは下記の通りです。
 * <pre>
 * {
 *   "type": "ice",
 *   "data": {
 *     "candidate": ...,
 *     "sdpMLineIndex": ...,
 *      ...
 *   }
 * }
 * </pre>
 * 
 * SDP 情報が格納された JSON オブジェクトのフォーマットは下記の通りです。
 * <pre>
 * {
 *   "type": "sdp",  
 *   "data": {
 *     "type": "answer",
 *     "sdp": "o=- [....]"
 *   }
 * }
 * </pre>

 * @param data_json_object ICE or SDP 情報が格納された JSON オブジェクト
 */
void WebRTCMain::praseSdpAndIce(std::string& message)
{
  json j = json::parse(message);
  if (j.find("type") != j.end()) {
    if (j["type"].is_string()) {
      std::string type = j["type"].get<std::string>();
      if (type.compare("sdp") == 0) {
        if (j.find("data") != j.end()) {
          auto data = j["data"];
          parseSdp(data);
        }
      } else if (type.compare("ice") == 0) {
        if (j.find("data") != j.end()) {
          auto data = j["data"];
          parseIce(data);
        }
      } else {
        g_error("Unknown type. %s\n", type.c_str());
      }
    }
  }
}

void WebRTCMain::parseSdp(json& data_json_object)
{
  if (!mPipeline) {
    g_error("mPipeline is not initialized.\n");
    return;
  }

  if (data_json_object.find("type") == data_json_object.end() 
      && !data_json_object["type"].is_string()) {
    g_error("Received SDP message without type field\n");
    return;
  }

  if (data_json_object.find("sdp") == data_json_object.end() 
      && !data_json_object["sdp"].is_string()) {
    g_error("Received SDP message without SDP string\n");
    return;
  }

  std::string type_string = data_json_object["type"].get<std::string>();
  std::string sdp_string = data_json_object["sdp"].get<std::string>();

  if (type_string.compare("answer") == 0) {
    mPipeline->onAnswerReceived(sdp_string.c_str());
  } else if (type_string.compare("offer") == 0) {
    mPipeline->onOfferReceived(sdp_string.c_str());
  } else {
    g_error("Unknown type. %s\n", type_string.c_str());
  }
}

void WebRTCMain::parseIce(json& data_json_object)
{
  if (!mPipeline) {
    g_error("mPipeline is not initialized.\n");
    return;
  }

  if (data_json_object.find("sdpMLineIndex") == data_json_object.end()) {
    g_error("Received ICE message without mline index\n\n");
    return;
  }
  
  if (data_json_object.find("candidate") == data_json_object.end() 
      && !data_json_object["candidate"].is_string()) {
    g_error("Received ICE message without ICE candidate string\n\n");
    return;
  }

  int mline_index = data_json_object["sdpMLineIndex"].get<int>();
  std::string candidate_string = data_json_object["candidate"].get<std::string>();

  mPipeline->onIceReceived(mline_index, candidate_string.c_str());
}

// WebsocketClientListener implements.

void WebRTCMain::onConnected(WebsocketClient *client)
{

}

void WebRTCMain::onDisconnected(WebsocketClient *client)
{

}

void WebRTCMain::onMessage(WebsocketClient *client, std::string& message)
{
  const char *text = message.c_str();
  if (g_strcmp0(text, "playerConnected") == 0) {
    startPipeline();
  } else if (g_strcmp0(text, "playerDisconnected") == 0) {
    stopPipeline();
  } else {
    praseSdpAndIce(message);
  }
}

// WebRTCPipelineListener implements.

void WebRTCMain::onSendSdp(WebRTCPipeline *pipeline, gint type, gchar *sdp_string)
{
  if (!sdp_string) {
    g_assert_not_reached();
  }

  json j;
  j["type"] = "sdp";

  if (type == GST_WEBRTC_SDP_TYPE_OFFER) {
    j["data"] = json{
      {"type", "offer"}, 
      {"sdp", sdp_string}
    };
  } else if (type == GST_WEBRTC_SDP_TYPE_ANSWER) {
    j["data"] = json{
      {"type", "answer"}, 
      {"sdp", sdp_string}
    };
  } else {
    g_assert_not_reached();
  }

  if (mClient) {
    std::string msg = j.dump();
    mClient->sendMessage(msg);
  }
}

void WebRTCMain::onSendIceCandidate(WebRTCPipeline *pipeline, guint mlineindex, gchar *candidate)
{
  if (!candidate) {
    g_assert_not_reached();
  }

  json j;
  j["type"] = "ice";
  j["data"] = json{
    {"sdpMLineIndex", mlineindex}, 
    {"candidate", candidate}
  };

  if (mClient) {
    std::string msg = j.dump();
    mClient->sendMessage(msg);
  }
}

void WebRTCMain::onAddStream(WebRTCPipeline *pipeline, GstPad *pad)
{
  // TODO 相手からの映像・音声のストリームが送られてきた時の処理を行う
}

void WebRTCMain::onDataChannelConnected(WebRTCPipeline *pipeline)
{

}

void WebRTCMain::onDataChannelDisconnected(WebRTCPipeline *pipeline)
{

}

void WebRTCMain::onDataChannel(WebRTCPipeline *pipeline, std::string& message)
{
  g_print("    onDataChannel: %s\n", message.c_str());

  std::string text("Echo: ");
  text += message;
  pipeline->sendMessage(text);
}
