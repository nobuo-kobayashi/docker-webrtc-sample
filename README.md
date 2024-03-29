# GStreamer 上で WebRTC を動作させるサンプル

GStreamer 上で WebRTC を動作させるためのサンプルプログラムになります。

|フォルダ|備考|
|:--|:--|
|data|クライアントとして使用する HTML|
|signaling|シグナリングサーバのサンプル|
|webrtc-sample|WebRTC のサンプル|


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

webrtc-sample/gst-webrtc-sample/src/gst-webrtc-main.cc のソースコードを変更することで、配信する映像・音声を変更することができます。

webrtcbin に格納する映像・音声を変更することで、切り替えることができます。

```
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
```

以下のように videotestsrc から v4l2src に変更することで、映像を Web カメラに変更することができます。

```
void WebRTCMain::startPipeline()
{
  stopPipeline();

  // webrtcbin エレメント名前は固定にしておく必要があります
  // webrtcbin name=webrtcbin を変更する場合には、呼び出している箇所も全て変更する必要があります。
  std::string bin = "webrtcbin name=webrtcbin bundle-policy=max-bundle latency=100 stun-server=stun://stun.l.google.com:19302 \
        v4l2src device=/dev/video0 \
         ! image/jpeg,width=1280,height=720,framerate=30/1 \
         ! jpegdec \
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
```

