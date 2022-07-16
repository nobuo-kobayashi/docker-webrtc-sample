#include "gst-webrtc-pipeline.h"
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

WebRTCPipeline::WebRTCPipeline()
{
  mListener = nullptr;
  mPipeline = nullptr;
  mWebRTCBin = nullptr;
  mSendDataChannel = nullptr;
  mNegotiationNeededHandleId = 0;
  mSendIceCandidateHandleId = 0;
  mIceGatheringStateNotifyHandleId = 0;
  mIncomingStreamHandleId = 0;
  mDataChannelHandleId = 0;
}

WebRTCPipeline::~WebRTCPipeline()
{
  stopPipeline();
}

void WebRTCPipeline::startPipeline(std::string& bin)
{
  GError *error = NULL;

  mPipeline = gst_parse_launch(bin.c_str(), &error);

  if (error) {
    g_printerr("Failed to parse launch: %s.\n", error->message);
    g_error_free(error);
    return;
  }

  // webrtcbin を取得
  mWebRTCBin = gst_bin_get_by_name(GST_BIN(mPipeline), "webrtcbin");

  if (!mWebRTCBin) {
    g_printerr("Not found a webrtcbin named webrtcbin.\n");
    return;
  }

  // 送信専用に設定
  GArray *transceivers = NULL;
  g_signal_emit_by_name(mWebRTCBin, "get-transceivers", &transceivers);
  if (transceivers) {
    GstWebRTCRTPTransceiver *trans = g_array_index(transceivers, GstWebRTCRTPTransceiver *, 0);
    trans->direction = GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY;
    g_array_unref(transceivers);
  }

  // 接続するためのネゴシエーションを行うためのコールバックを設定
  mNegotiationNeededHandleId = g_signal_connect(mWebRTCBin, "on-negotiation-needed", 
      G_CALLBACK(WebRTCPipeline::onNegotiationNeeded), this);
  mSendIceCandidateHandleId = g_signal_connect(mWebRTCBin, "on-ice-candidate", 
      G_CALLBACK(WebRTCPipeline::onSendIceCandidate), this);
  mIceGatheringStateNotifyHandleId = g_signal_connect(mWebRTCBin, "notify::ice-gathering-state", 
      G_CALLBACK(WebRTCPipeline::onIceGatheringStateNotify), this);

  gst_element_set_state(mPipeline, GST_STATE_READY);

  // 映像受信用のコールバック
  mIncomingStreamHandleId = g_signal_connect(mWebRTCBin, "pad-added", 
      G_CALLBACK(WebRTCPipeline::onIncomingStream), this);

  // 受信用データチャンネルのコールバック
  mDataChannelHandleId = g_signal_connect(mWebRTCBin, "on-data-channel", 
      G_CALLBACK(WebRTCPipeline::onDataChannel), this);

  // 送信用データチャンネルを作成
  std::string name("send-channel");
  mSendDataChannel = new WebRTCDataChannel(mWebRTCBin);
  mSendDataChannel->setListener(this);
  mSendDataChannel->connect(name);

  // パイプラインの再生を開始
  gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PLAYING);
}

void WebRTCPipeline::stopPipeline()
{
  if (mNegotiationNeededHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mWebRTCBin), mNegotiationNeededHandleId);
    mNegotiationNeededHandleId = 0;
  }

  if (mSendIceCandidateHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mWebRTCBin), mSendIceCandidateHandleId);
    mSendIceCandidateHandleId = 0;
  }

  if (mIceGatheringStateNotifyHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mWebRTCBin), mIceGatheringStateNotifyHandleId);
    mIceGatheringStateNotifyHandleId = 0;
  }

  if (mIncomingStreamHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mWebRTCBin), mIncomingStreamHandleId);
    mIncomingStreamHandleId = 0;
  }

  if (mDataChannelHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mWebRTCBin), mDataChannelHandleId);
    mDataChannelHandleId = 0;
  }

  if (mSendDataChannel) {
    delete mSendDataChannel;
    mSendDataChannel = nullptr;
  }

  for (auto itr = mReceiveDataChannels.begin(); itr != mReceiveDataChannels.end(); ++itr) {
    delete *itr;
  }
  mReceiveDataChannels.clear();

  if (mWebRTCBin) {
    gst_object_unref(mWebRTCBin);
    mWebRTCBin = nullptr;
  }

  if (mPipeline) {
    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_NULL);
    g_clear_object(&mPipeline);
    mPipeline = nullptr;
  }
}

void WebRTCPipeline::sendMessage(std::string& message)
{
  if (mSendDataChannel) {
    mSendDataChannel->sendMessage(message);
  }
}

void WebRTCPipeline::onOfferReceived(const gchar *sdpString) 
{
  GstSDPMessage *sdp = NULL;

  int ret = gst_sdp_message_new(&sdp);
  g_assert_cmphex(ret, ==, GST_SDP_OK);

  ret = gst_sdp_message_parse_buffer((guint8 *) sdpString, strlen(sdpString), sdp);
  if (ret != GST_SDP_OK) {
    g_error ("Could not parse SDP string\n");
    return;
  }

  onOfferReceived(sdp);
}

void WebRTCPipeline::onAnswerReceived(const gchar *sdpString)
{
  GstSDPMessage *sdp = NULL;

  int ret = gst_sdp_message_new(&sdp);
  g_assert_cmphex(ret, ==, GST_SDP_OK);

  ret = gst_sdp_message_parse_buffer((guint8 *) sdpString, strlen(sdpString), sdp);
  if (ret != GST_SDP_OK) {
    g_error ("Could not parse SDP string\n");
    return;
  }

  onAnswerReceived(sdp);
}

void WebRTCPipeline::onIceReceived(guint mlineIndex, const gchar *candidateString)
{
  g_signal_emit_by_name(mWebRTCBin, "add-ice-candidate", mlineIndex, candidateString);
}

// private functions.

void WebRTCPipeline::onAnswerReceived(GstSDPMessage *sdp)
{
  GstWebRTCSessionDescription *answer = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER, sdp);
  if (answer) {
    GstPromise *promise = gst_promise_new();
    g_signal_emit_by_name(mWebRTCBin, "set-remote-description", answer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);
    gst_webrtc_session_description_free(answer);
  }
}

void WebRTCPipeline::onOfferReceived(GstSDPMessage *sdp)
{
  GstWebRTCSessionDescription *offer = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_OFFER, sdp);
  if (offer) {
    GstPromise *promise = gst_promise_new();
    g_signal_emit_by_name(mWebRTCBin, "set-remote-description", offer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);
    gst_webrtc_session_description_free(offer);

    promise = gst_promise_new_with_change_func(WebRTCPipeline::onAnswerCreated, this, NULL);
    g_signal_emit_by_name(mWebRTCBin, "create-answer", NULL, promise);
  }
}

void WebRTCPipeline::sendSdp(GstWebRTCSessionDescription *desc)
{
  if (mListener) {
    gchar *sdp_string = gst_sdp_message_as_text(desc->sdp);
    mListener->onSendSdp(this, desc->type, sdp_string);
    g_free(sdp_string);
  }
}

void WebRTCPipeline::sendIceCandidate(guint mlineindex, gchar *candidate)
{
  if (mListener) {
    mListener->onSendIceCandidate(this, mlineindex, candidate);
  }
}

void WebRTCPipeline::addStream(GstPad *pad)
{
  if (mListener) {
    mListener->onAddStream(this, pad);
  }
}

void WebRTCPipeline::createReceiveDataChannel(GstWebRTCDataChannel *dataChannel)
{
  WebRTCDataChannel *channel = new WebRTCDataChannel(mWebRTCBin);
  if (channel) {
    channel->setListener(this);
    channel->connect(dataChannel);
    mReceiveDataChannels.push_back(channel);
  }
}

// callback static functions.

void WebRTCPipeline::onNegotiationNeeded(GstElement *webrtcbin, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;
  GstPromise *promise = gst_promise_new_with_change_func(WebRTCPipeline::onOfferCreated, userData, NULL);
  g_signal_emit_by_name(pipeline->mWebRTCBin, "create-offer", NULL, promise);
}

// offer が作成された場合
void WebRTCPipeline::onOfferCreated(GstPromise *promise, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;

  g_assert_cmphex(gst_promise_wait(promise), ==, GST_PROMISE_RESULT_REPLIED);

  GstWebRTCSessionDescription *offer = NULL;

  const GstStructure *reply = gst_promise_get_reply(promise);
  gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
  gst_promise_unref(promise);

  promise = gst_promise_new();
  g_signal_emit_by_name(pipeline->mWebRTCBin, "set-local-description", offer, promise);
  gst_promise_interrupt(promise);
  gst_promise_unref(promise);

  pipeline->sendSdp(offer);

  gst_webrtc_session_description_free(offer);
}

void WebRTCPipeline::onAnswerCreated(GstPromise *promise, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;

  g_assert_cmphex(gst_promise_wait(promise), ==, GST_PROMISE_RESULT_REPLIED);

  GstWebRTCSessionDescription *answer = NULL;

  const GstStructure *reply = gst_promise_get_reply(promise);
  gst_structure_get(reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
  gst_promise_unref(promise);

  promise = gst_promise_new();
  g_signal_emit_by_name(pipeline->mWebRTCBin, "set-local-description", answer, promise);
  gst_promise_interrupt(promise);
  gst_promise_unref(promise);

  pipeline->sendSdp(answer);

  gst_webrtc_session_description_free(answer);
}

void WebRTCPipeline::onSendIceCandidate(GstElement *webrtcbin, guint mlineindex, gchar *candidate, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;
  pipeline->sendIceCandidate(mlineindex, candidate);
}

void WebRTCPipeline::onIceGatheringStateNotify(GstElement *webrtcbin, GParamSpec *pspec, gpointer userData)
{
  GstWebRTCICEGatheringState ice_gather_state;
  const gchar *new_state = "unknown";

  g_object_get(webrtcbin, "ice-gathering-state", &ice_gather_state, NULL);
  switch (ice_gather_state) {
  case GST_WEBRTC_ICE_GATHERING_STATE_NEW:
    new_state = "new";
    break;
  case GST_WEBRTC_ICE_GATHERING_STATE_GATHERING:
    new_state = "gathering";
    break;
  case GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE:
    new_state = "complete";
    break;
  }
  g_print("ICE gathering state changed to %s.\n", new_state);
}

// 新規ストリームの追加
void WebRTCPipeline::onIncomingStream(GstElement *webrtcbin, GstPad *pad, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;
  pipeline->addStream(pad);
}

// 新規データチャンネルの接続
void WebRTCPipeline::onDataChannel(GstElement *webrtcbin, GObject *dataChannel, gpointer userData)
{
  WebRTCPipeline *pipeline = (WebRTCPipeline *) userData;
  pipeline->createReceiveDataChannel((GstWebRTCDataChannel *)dataChannel);
}

// WebRTCDataChannelListener implements.

void WebRTCPipeline::onConnected(WebRTCDataChannel *channel)
{
  // MEMO データチャンネルが接続されたときに呼びだれます。

  if (mListener) {
    mListener->onDataChannelConnected(this);
  }
}

void WebRTCPipeline::onDisconnected(WebRTCDataChannel *channel)
{
  // MEMO データチャンネルが切断されたときに呼びだれます。
  
  if (mListener) {
    mListener->onDataChannelDisconnected(this);
  }
}

void WebRTCPipeline::onMessage(WebRTCDataChannel *channel, std::string& message)
{
  // MEMO データチャンネルにメッセージ送られてきた時に呼び出されます。

  if (mListener) {
    mListener->onDataChannel(this, message);
  }
}