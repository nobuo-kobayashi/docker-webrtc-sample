var webrtc = (function(parent, global) {
  let mHtml5VideoElement;
  let mWebsocketConnection;
  let mWebrtcPeerConnection;
  let mWebrtcConfiguration;
  let mReportError;
  let mSendDataChannel;
  let mRecvDataChannelCallback;

  /**
   * SDP の設定を接続先に送り返す。
   * 
   * @param {*} desc 
   */
  function onLocalDescription(desc) { 
    // console.log("Local description: " + JSON.stringify(desc));
    mWebrtcPeerConnection.setLocalDescription(desc).then(function() { 
      mWebsocketConnection.send(JSON.stringify({
        'type': 'sdp', 
        'data': mWebrtcPeerConnection.localDescription
      }));
    }).catch(mReportError);
  } 

  /**
   * Websocket から SDP の設定が渡されてきた場合に呼び出される。
   * 
   * 送られてきた接続先の SDP の設定を WebRTC に設定する。
   * 
   * @param {*} sdp 
   */
  function onIncomingSDP(sdp) { 
    console.log("Incoming SDP: " + JSON.stringify(sdp));

    switch (sdp.type) {
      case 'answer':
        mWebrtcPeerConnection.setRemoteDescription(sdp).catch(mReportError);
        break;
      case 'offer':
        mWebrtcPeerConnection.setRemoteDescription(sdp).catch(mReportError);
        mWebrtcPeerConnection.createAnswer().then(onLocalDescription).catch(mReportError);
        break;
      default:
        console.log("Unknown type. type=" + sdp.type);
        break;
    }
  } 

  /**
   * Websocket から ICE の設定が渡されてきた場合に呼び出される。
   * 
   * 送られてきた接続先の ICE の設定を WebRTC に設定する。
   * 
   * @param {*} ice 
   */
  function onIncomingICE(ice) { 
    console.log("Incoming ICE: " + JSON.stringify(ice));
    let candidate = new RTCIceCandidate(ice);
    mWebrtcPeerConnection.addIceCandidate(candidate).catch(mReportError);
  } 

  /**
   * WebRTC から ICE の設定が渡される場合に呼び出される。
   * 
   * ICE の設定を Websocket を経由して相手に送信する。
   * 
   * @param {*} event 
   * @returns 
   */
  function onIceCandidate(event) { 
    if (event.candidate == null) {
      return;
    }
    // console.log("Sending ICE candidate out: " + JSON.stringify(event.candidate));
    mWebsocketConnection.send(JSON.stringify({
      'type': 'ice', 
      'data': event.candidate
    }));
  }

  /**
   * Websocket からメッセージが届いた時に呼び出される。
   * 
   * @param {*} event 
   * @returns 
   */
  function onWebsocketMessage(event) { 
    let text = event.data
    if (text === 'playerConnected') {
    } else if (text === 'playerDisconnected') {
    } else {
      let msg;
      try { 
        msg = JSON.parse(text);
      } catch (e) { 
        return;
      }
  
      switch (msg.type) { 
        case 'sdp':
          onIncomingSDP(msg.data);
          break;
        case 'ice':
          onIncomingICE(msg.data);
          break;
        default:
          console.log('unknown type. type=' + msg.type);
          break;
      } 
    }
  }

  /**
   * WebRTC の状態が変更された時の呼び出される。
   * 
   * @param {*} event 
   */
  function onConnectionStateChange(event) {
    console.log('mWebrtcPeerConnection: ' + mWebrtcPeerConnection.connectionState)
    switch(mWebrtcPeerConnection.connectionState) {
      case 'connected':
        // The connection has become fully connected
        break;
      case 'disconnected':
      case 'failed':
        // One or more transports has terminated unexpectedly or in an error
        break;
      case 'closed':
        // The connection has been closed
        break;
    }
  }

  /**
   * 接続先から映像のストリームの追加要求があった場合に呼び出される。
   * @param {*} event 
   */
  function onAddRemoteStream(event) { 
    mHtml5VideoElement.srcObject = event.streams[0];
  } 

  /**
   * 接続先からデータチャンネルの追加要求があった場合に呼び出される。
   * 
   * @param {*} event 
   */
  function onDataChannel(event) {
    let receiveChannel = event.channel;
    receiveChannel.onopen = function (event) {
      console.log('datachannel::onopen', event);
    }

    receiveChannel.onmessage = function (event) {
      console.log('datachannel::onmessage:', event.data);

      if (mRecvDataChannelCallback) {
        mRecvDataChannelCallback(event.data);
      }
    }

    receiveChannel.onerror = function (event) {
      console.log('datachannel::onerror', event);
    }

    receiveChannel.onclose = function (event) {
      console.log('datachannel::onclose', event);
    }
  }

  /**
   * RTCPeerConnection の作成を行う。
   */
  function createWebRTC() {
    if (mWebrtcPeerConnection) {
      destroyWebRTC();
    }

    mWebrtcPeerConnection = new RTCPeerConnection(mWebrtcConfiguration);
    mWebrtcPeerConnection.onconnectionstatechange = onConnectionStateChange;
    mWebrtcPeerConnection.ontrack = onAddRemoteStream;
    mWebrtcPeerConnection.onicecandidate = onIceCandidate;
    mWebrtcPeerConnection.ondatachannel = onDataChannel;
    
    mSendDataChannel = mWebrtcPeerConnection.createDataChannel('datachannel', null);
    mSendDataChannel.onmessage = function (event) {
      console.log('onmessage', event.data);
    };

    mSendDataChannel.onopen = function (event) {
      console.log('onopen', event);
    };

    mSendDataChannel.onclose = function () {
      console.log('onclose');
    };
  }

  /**
   * RTCPeerConnection の後始末を行う。
   */
  function destroyWebRTC() {
    if (mWebrtcPeerConnection) {
      if (mSendDataChannel) {
        mSendDataChannel.close()
        mSendDataChannel = null;
      }

      mWebrtcPeerConnection.close();
      mWebrtcPeerConnection = null;
    }
  }

  /**
   * シグナリングサーバとの接続を行う Websocket を作成する。
   * 
   * @param {*} wsUrl 
   */
  function createWebsocket(wsUrl) {
    mWebsocketConnection = new WebSocket(wsUrl);
    mWebsocketConnection.addEventListener('open', function (event) {
      // ブラウザ側から offer を行う場合は以下のメソッドを呼び出す
      // sendOffer();
    });
    mWebsocketConnection.addEventListener('close', function (event) {
      stopStream();
    });
    mWebsocketConnection.addEventListener('message', onWebsocketMessage);
  }

  function sendOffer() {
    // 映像・音声のデータを受信するだけにの設定を行います。
    const videoTransceiver = mWebrtcPeerConnection.addTransceiver('video');
    videoTransceiver.direction = 'recvonly';
    const audioTransceiver = mWebrtcPeerConnection.addTransceiver('audio');
    audioTransceiver.direction = 'recvonly';

    mWebrtcPeerConnection.createOffer().then((sdp) => {
      mWebrtcPeerConnection.setLocalDescription(sdp).then(function() { 
        mWebsocketConnection.send(JSON.stringify({
          'type': 'sdp', 
          'data': sdp
        }));
      }).catch(mReportError);
    });
  }
  parent.sendOffer = sendOffer;

  /**
   * Websocket の後始末を行う。
   */
  function destroyWebsocket() {
    if (mWebsocketConnection) {
      mWebsocketConnection.close();
      mWebsocketConnection = null;
    }
  }

  function createWebsocketUrl(hostname, port, path) {
    var l = window.location;
    var wsScheme = (l.protocol.indexOf('https') == 0) ? 'wss' : 'ws';
    var wsHost = (hostname != undefined) ? hostname : l.hostname;
    var wsPort = (port != undefined) ? port : l.port;
    var wsPath = (path != undefined) ? path : '';
    if (wsPort) {
      wsPort = ':' + wsPort;
    }
    return wsScheme + '://' + wsHost + wsPort + '/' + wsPath;
  }

  /**
   * WebRTC の接続を開始する。
   * 
   * @param {*} videoElement 映像を表示する video タグのエレメント
   * @param {*} hostname ホスト名 省略された場合は HTML が置いてあるサーバのホスト名
   * @param {*} port ポート番号
   * @param {*} path パス
   * @param {*} configuration WebRTC の設定
   * @param {*} dataChannelCB データチャンネルのメッセージを通知するコールバック
   * @param {*} reportErrorCB エラーを通知するコールバック
   */
  function playStream(videoElement, hostname, port, path, configuration, dataChannelCB, reportErrorCB) { 
    mHtml5VideoElement = videoElement;
    mWebrtcConfiguration = configuration;
    mRecvDataChannelCallback = dataChannelCB;
    mReportError = (reportErrorCB != undefined) ? reportErrorCB : function(text) {};
    createWebRTC();
    createWebsocket(createWebsocketUrl(hostname, port, path));
  } 
  parent.playStream = playStream;

  /**
   * WebRTC を停止する。
   */
  function stopStream() {
    destroyWebRTC();
    destroyWebsocket();
  }
  parent.stopStream = stopStream;

  /**
   * データチャンネルでメッセージを送信する。
   * @param {*}} message 
   */
  function sendDataChannel(message) {
    if (mSendDataChannel && message) {
      mSendDataChannel.send(message);
    }
  }
  parent.sendDataChannel = sendDataChannel;

  return parent;
})(webrtc || {}, this.self || global);
