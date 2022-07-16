#include "gst-websocket-client.h"

WebsocketClient::WebsocketClient() {
  mConnection = nullptr;
  mListener = nullptr;
  mDisconnectHandleId = 0;
  mMessageHandleId = 0;
}

WebsocketClient::~WebsocketClient()
{
  disconnect();
}

void WebsocketClient::connectAsync(std::string& url, std::string& origin)
{
  std::vector<std::string> protocols;
  this->connectAsync(url, origin, protocols);
}

void WebsocketClient::connectAsync(std::string& url, std::string& origin, std::vector<std::string>& protocols)
{
  std::string userAgent = "gst-webrtc-sample";
  this->connectAsync(url, origin, protocols, userAgent, true);
}

void WebsocketClient::connectAsync(std::string& url, std::string& origin, std::vector<std::string>& protocols, std::string& userAgent, bool isLogger)
{
  SoupMessage *message;
  SoupSession *session;

  const gchar *t_protocols[protocols.size() + 1];
  for(size_t i = 0; i < protocols.size(); i++) {
    t_protocols[i] = protocols[i].c_str();
  }
  t_protocols[protocols.size()] = NULL;

  session = soup_session_new_with_options(SOUP_SESSION_USER_AGENT, userAgent.c_str(), NULL);
  g_object_set(G_OBJECT(session), SOUP_SESSION_SSL_STRICT, FALSE, NULL);

  if (isLogger) {
    SoupLogger *logger = soup_logger_new(SOUP_LOGGER_LOG_BODY, -1);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger));
    g_object_unref(logger);
  }

  message = soup_message_new(SOUP_METHOD_GET, url.c_str());

  soup_session_websocket_connect_async(session, message, origin.c_str(), 
      (gchar **) t_protocols, NULL, 
      (GAsyncReadyCallback) WebsocketClient::onServerConnected, this);
}

void WebsocketClient::disconnect()
{
 if (mConnection) {
    if (mDisconnectHandleId) {
      g_signal_handler_disconnect(G_OBJECT(mConnection), mDisconnectHandleId);
      mDisconnectHandleId = 0;
    }

    if (mMessageHandleId) {
      g_signal_handler_disconnect(G_OBJECT(mConnection), mMessageHandleId);
      mMessageHandleId = 0;
    }

    if (soup_websocket_connection_get_state(mConnection) == SOUP_WEBSOCKET_STATE_OPEN) {
      soup_websocket_connection_close(mConnection, 1000, "disconnect");
    }

    g_object_unref(mConnection);
    mConnection = nullptr;
  }
}

void WebsocketClient::sendMessage(std::string& message)
{
  if (mConnection) {
    soup_websocket_connection_send_text(mConnection, message.c_str());
  }
}

// private functions.

void WebsocketClient::onServerConnected(SoupSession *session, GAsyncResult *res, gpointer userData)
{
  GError *error = NULL;
  SoupWebsocketConnection *wsConn = soup_session_websocket_connect_finish(session, res, &error);
  if (error) {
    g_error_free(error);
    return;
  }

  WebsocketClient *client = (WebsocketClient *) userData;
  if (client) {
    client->mConnection = wsConn;
    client->mDisconnectHandleId = g_signal_connect(wsConn, "closed", 
        G_CALLBACK(WebsocketClient::onServerClosed), userData);
    client->mMessageHandleId = g_signal_connect(wsConn, "message", 
        G_CALLBACK(WebsocketClient::onServerMessage), userData);

    if (client->mListener) {
      client->mListener->onConnected(client);
    }
  }
}

void WebsocketClient::onServerClosed(SoupWebsocketConnection *conn G_GNUC_UNUSED, gpointer userData)
{
  WebsocketClient *client = (WebsocketClient *) userData;
  if (client && client->mListener) {
    client->mListener->onDisconnected(client);
  }
}

void WebsocketClient::onServerMessage(SoupWebsocketConnection *conn, SoupWebsocketDataType type, GBytes *message, gpointer userData)
{
  WebsocketClient *client = (WebsocketClient *) userData;

  switch (type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
      g_printerr("Received unknown binary message, ignoring.\n");
      return;
    case SOUP_WEBSOCKET_DATA_TEXT: {
      gsize size;
      const gchar *data = (const gchar *) g_bytes_get_data(message, &size);
      gchar *text = g_strndup(data, size);
      if (text) {
        std::string msg(text);
        if (client && client->mListener) {
          client->mListener->onMessage(client, msg);
        }
        g_free(text);
      }
    } break;
    default:
      g_assert_not_reached();
  }
}
