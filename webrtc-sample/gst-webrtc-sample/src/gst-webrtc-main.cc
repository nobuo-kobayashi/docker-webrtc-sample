#include "gst-webrtc-main.h"

static gchar *get_string_from_json_object(JsonObject *object)
{
  JsonNode *root = json_node_init_object(json_node_alloc(), object);
  JsonGenerator *generator = json_generator_new();
  json_generator_set_root(generator, root);
  gchar *text = json_generator_to_data(generator, NULL);
  g_object_unref(generator);
  json_node_free(root);
  return text;
}

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
  std::string bin = "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 \
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
  const gchar *text = message.c_str();

  JsonParser *json_parser = json_parser_new();
  if (!json_parser_load_from_data(json_parser, text, -1, NULL)) {
    g_error("Unknown message \"%s\", ignoring.", text);
    return;
  }

  JsonNode *root_json = json_parser_get_root(json_parser);
  if (!JSON_NODE_HOLDS_OBJECT(root_json)) {
    g_error("Received message without json.\n\n");
    g_object_unref(G_OBJECT(json_parser));
    return;
  }
  JsonObject *root_json_object = json_node_get_object(root_json);

  if (!json_object_has_member(root_json_object, "type")) {
    g_error("Received message without type field.\n\n");
    g_object_unref(G_OBJECT(json_parser));
    return;
  }
  const gchar *type_string = json_object_get_string_member(root_json_object, "type");

  if (!json_object_has_member(root_json_object, "data")) {
    g_error("Received message without data field.\n\n");
    g_object_unref(G_OBJECT(json_parser));
    return;
  }

  JsonObject *data_json_object = json_object_get_object_member(root_json_object, "data");

  if (g_strcmp0 (type_string, "sdp") == 0) {
    parseSdp(data_json_object);
  } else if (g_strcmp0 (type_string, "ice") == 0) {
    parseIce(data_json_object);
  } else {
    g_print("Received unknown type. %s\n", type_string);
  }
  
  g_object_unref(G_OBJECT(json_parser));
}

void WebRTCMain::parseSdp(JsonObject *data_json_object)
{
  if (!json_object_has_member(data_json_object, "type")) {
    g_error ("Received SDP message without type field\n");
    return;
  }
  const gchar *sdp_type_string = json_object_get_string_member(data_json_object, "type");

  if (!json_object_has_member(data_json_object, "sdp")) {
    g_error ("Received SDP message without SDP string\n");
    return;
  }
  const gchar *sdp_string = json_object_get_string_member(data_json_object, "sdp");

  if (g_strcmp0(sdp_type_string, "answer") == 0) {
    mPipeline->onAnswerReceived(sdp_string);
  } else if (g_strcmp0(sdp_type_string, "offer") == 0) {
    mPipeline->onOfferReceived(sdp_string);
  }
}

void WebRTCMain::parseIce(JsonObject *data_json_object)
{
  if (!json_object_has_member(data_json_object, "sdpMLineIndex")) {
    g_error("Received ICE message without mline index\n\n");
    return;
  }
  guint mline_index = json_object_get_int_member(data_json_object, "sdpMLineIndex");

  if (!json_object_has_member(data_json_object, "candidate")) {
    g_error("Received ICE message without ICE candidate string\n\n");
    return;
  }
  const gchar *candidate_string = json_object_get_string_member(data_json_object, "candidate");

  mPipeline->onIceReceived(mline_index, candidate_string);
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
  JsonObject *sdp = json_object_new();

  if (type == GST_WEBRTC_SDP_TYPE_OFFER) {
    json_object_set_string_member(sdp, "type", "offer");
  } else if (type == GST_WEBRTC_SDP_TYPE_ANSWER) {
    json_object_set_string_member(sdp, "type", "answer");
  } else {
    g_assert_not_reached();
  }

  json_object_set_string_member(sdp, "sdp", sdp_string);

  JsonObject *sdp_json = json_object_new();
  json_object_set_string_member(sdp_json, "type", "sdp");
  json_object_set_object_member(sdp_json, "data", sdp);

  gchar *json_string = get_string_from_json_object(sdp_json);
  if (json_string) {
    if (mClient) {
      std::string message(json_string);
      mClient->sendMessage(message);
    }
    g_free(json_string);
  }

  json_object_unref(sdp_json);
}

void WebRTCMain::onSendIceCandidate(WebRTCPipeline *pipeline, guint mlineindex, gchar *candidate)
{
  JsonObject *ice_json = json_object_new();
  json_object_set_string_member(ice_json, "type", "ice");

  JsonObject *ice_data_json = json_object_new();
  json_object_set_int_member(ice_data_json, "sdpMLineIndex", mlineindex);
  json_object_set_string_member(ice_data_json, "candidate", candidate);
  json_object_set_object_member(ice_json, "data", ice_data_json);

  gchar *json_string = get_string_from_json_object(ice_json);
  if (json_string) {
    if (mClient) {
      std::string message(json_string);
      mClient->sendMessage(message);
    }
    g_free(json_string);
  }

  json_object_unref(ice_json);
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
