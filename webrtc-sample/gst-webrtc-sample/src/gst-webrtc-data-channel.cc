#include "gst-webrtc-data-channel.h"

WebRTCDataChannel::WebRTCDataChannel(GstElement *webrtcbin)
{
  mWebRTCBin = webrtcbin;
  mDataChannel = nullptr;
  mListener = nullptr;
  mOpenHandleId = 0;
  mCloseHandleId = 0;
  mMessageHandleId = 0;
  mErrorHandleId = 0;
}

WebRTCDataChannel::~WebRTCDataChannel()
{
  disconnect();
}

void WebRTCDataChannel::connect(GstWebRTCDataChannel *dataChannel)
{
  mDataChannel = dataChannel;
  setCallback();
}

void WebRTCDataChannel::connect(std::string& name)
{
  // 送信用のデータチャンネルを作成
  g_signal_emit_by_name(mWebRTCBin, "create-data-channel", name.c_str(), NULL, &mDataChannel);
  if (mDataChannel) {
    setCallback();
  } else {
    g_print("Could not create data channel, is usrsctp available?\n");
  }
}

void WebRTCDataChannel::disconnect()
{
  if (mOpenHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mDataChannel), mOpenHandleId);
    mOpenHandleId = 0;
  }

  if (mCloseHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mDataChannel), mCloseHandleId);
    mCloseHandleId = 0;
  }

  if (mMessageHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mDataChannel), mMessageHandleId);
    mMessageHandleId = 0;
  }

  if (mErrorHandleId) {
    g_signal_handler_disconnect(G_OBJECT(mDataChannel), mErrorHandleId);
    mErrorHandleId = 0;
  }

  if (mDataChannel) {
    gst_webrtc_data_channel_close(mDataChannel);
    mDataChannel = nullptr;
  }
}

void WebRTCDataChannel::sendMessage(std::string& message)
{
  if (mDataChannel) {
    gst_webrtc_data_channel_send_string(mDataChannel, message.c_str());
  }
}

// private functions.

void WebRTCDataChannel::setCallback()
{
  mErrorHandleId = g_signal_connect(mDataChannel, "on-error", 
      G_CALLBACK(WebRTCDataChannel::onError), this);
  mOpenHandleId = g_signal_connect(mDataChannel, "on-open", 
      G_CALLBACK(WebRTCDataChannel::onOpen), this);
  mCloseHandleId = g_signal_connect(mDataChannel, "on-close", 
      G_CALLBACK(WebRTCDataChannel::onClose),  this);
  mMessageHandleId = g_signal_connect(mDataChannel, "on-message-string", 
      G_CALLBACK(WebRTCDataChannel::onMessageString), this);
}

// callback functions.

void WebRTCDataChannel::onError(GObject *dc, gpointer userData)
{

}

void WebRTCDataChannel::onOpen(GObject *dc, gpointer userData)
{
  WebRTCDataChannel *channel = (WebRTCDataChannel *) userData;
  if (channel && channel->mListener) {
    channel->mListener->onConnected(channel);
  }
}

void WebRTCDataChannel::onClose(GObject *dc, gpointer userData)
{
  WebRTCDataChannel *channel = (WebRTCDataChannel *) userData;
  if (channel && channel->mListener) {
    channel->mListener->onDisconnected(channel);
  }
}

void WebRTCDataChannel::onMessageString(GObject *dc, gchar *message, gpointer userData)
{
  WebRTCDataChannel *channel = (WebRTCDataChannel *) userData;
  if (channel && channel->mListener) {
    std::string msg(message);
    channel->mListener->onMessage(channel, msg);
  }
}
