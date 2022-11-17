# GStreamer 上で WebRTC を動作させるサンプル

GStreamer 上で WebRTC を動作させるためのサンプルプログラムになります。

|フォルダ|備考|
|:--|:--|
|data|クライアントとして使用する HTML|
|signaling|シグナリングサーバのサンプル|
|webrtc-sample|WebRTC のサンプル|
|webrtc-turn-server|TURN サーバのサンプル|

以下のページで簡単な説明がありますので、参照してください。<br>
[https://www.gclue.jp/2022/07/gstreamer-webrtc.html](https://www.gclue.jp/2022/07/gstreamer-webrtc.html)

## ビルド

以下のコマンドを実行することで、GStreamer の環境ごと作成します。

```
$ docker-compose build
```

## 実行

以下のコマンドを実行することで、シグナリングサーバと WebRTC のサンプルプログラムを実行します。

```
$ docker-compose up
```

上記コマンドの起動後に、Chrome ブラウザで、以下の URL を開くことで、WebRTC に接続されます。

```
http://{DOCKERのIPアドレス}:9449
```

## 配信する映像・音声の変更

docker-compose.yml を変更することで、配信する映像・音声を変更することができます。

webrtcbin に格納する映像・音声を変更することで、切り替えることができます。

```yml
    command: >
      /opt/gst-webrtc-sample/build/gst-webrtc-sample "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 
        videotestsrc is-live=true 
         ! videoconvert 
         ! queue 
         ! vp8enc target-bitrate=10240000 deadline=1 
         ! rtpvp8pay 
         ! application/x-rtp,media=video,encoding-name=VP8,payload=96 
         ! webrtcbin. 
        audiotestsrc is-live=true 
         ! audioconvert 
         ! audioresample 
         ! queue 
         ! opusenc 
         ! rtpopuspay 
         ! application/x-rtp,media=audio,encoding-name=OPUS,payload=97 
         ! webrtcbin. "
```

以下のように videotestsrc から v4l2src に変更することで、映像を Web カメラに変更することができます。

```yml
    command: >
      /opt/gst-webrtc-sample/build/gst-webrtc-sample "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 
        v4l2src device=/dev/video0 
         ! image/jpeg,width=1280,height=720,framerate=30/1 
         ! jpegdec 
         ! videoconvert 
         ! queue 
         ! vp8enc target-bitrate=10240000 deadline=1 
         ! rtpvp8pay 
         ! application/x-rtp,media=video,encoding-name=VP8,payload=96 
         ! webrtcbin. 
        audiotestsrc is-live=true 
         ! audioconvert 
         ! audioresample 
         ! queue 
         ! opusenc 
         ! rtpopuspay 
         ! application/x-rtp,media=audio,encoding-name=OPUS,payload=97 
         ! webrtcbin. ";
```

# TURN サーバ

TURN サーバを使用して、WebRTC に接続する場合には、docker-compose-turn.yml を実行することで確認することができます。

## 設定

docker-compose-turn.yml をテキストエディタで開き、turn-server の値を修正する必要があります。<br>

turn-server=turn://user1:pass1@localhost:3479 と設定されていますので、localhost の部分を実行している端末の IP アドレスに変更してください。

```yml
    command: >
      /opt/gst-webrtc-sample/build/gst-webrtc-sample "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 ice-transport-policy=1 turn-server=turn://user1:pass1@localhost:3479  
        videotestsrc is-live=true 
         ! videoconvert 
         ! queue 
         ! vp8enc target-bitrate=10240000 deadline=1 
         ! rtpvp8pay 
         ! application/x-rtp,media=video,encoding-name=VP8,payload=96 
         ! webrtcbin. 
        audiotestsrc is-live=true 
         ! audioconvert 
         ! audioresample 
         ! queue 
         ! opusenc 
         ! rtpopuspay 
         ! application/x-rtp,media=audio,encoding-name=OPUS,payload=97 
         ! webrtcbin. "
```

data/index.html に以下のようにして、TURN サーバへの設定が行われています。<br>
サンプルでは、同じ端末に TURN サーバが存在することが前提になっていますので、変更する場合には下記の部分を修正します。

```javascript
let config = {
  iceServers: [
    { urls: 'stun:stun.l.google.com:19302' },
    { 
      urls: 'turn:' + location.hostname + ':3479', 
      username: "user2", 
      credential: "pass2"
    }
  ],
  iceTransportPolicy : "relay" 
};
```

## ビルド

以下のコマンドを実行することで、GStreamer の環境ごと作成します。

```
$ docker-compose -f docker-compose-turn.yml build
```

## 実行

以下のコマンドを実行することで、シグナリングサーバと WebRTC のサンプルプログラムを実行します。

```
$ docker-compose -f docker-compose-turn.yml up
```

上記コマンドの起動後に、Chrome ブラウザで、以下の URL を開くことで、WebRTC に接続されます。

```
http://{DOCKERのIPアドレス}:9449
```
