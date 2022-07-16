# GStreamer 上で WebRTC を動作させるサンプル

GStreamer 上で WebRTC を動作させるためのサンプルプログラムになります。

|フォルダ|備考|
|:--|:--|
|data|クライアントとして使用する HTML|
|signaling|シグナリングサーバのサンプル|
|webrtc-sample|WebRTC のサンプル|

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
