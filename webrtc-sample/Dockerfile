FROM ubuntu:18.04

# ビルド処理を行うフォルダの設定
ENV BUILD_FOLDER /tmp/build

# プログラムを実行するフォルダ
ENV WORK_FOLDER /opt

# インストール先のプレフィックス
ENV PREFIX /usr

# gstreamer のバージョン
ENV GSTREAMER_VERSION 1.18.6

# SRT のバージョン
ENV SRT_VERSION 1.4.2

# ORC のバージョン
ENV ORC_VERSION 0.4.32

# タイムゾーンの設定
RUN apt-get update && apt-get install -y --no-install-recommends tzdata
ENV TZ=Asia/Tokyo

# 必要なライブラリをインストール
RUN apt-get update && apt-get install -y --no-install-recommends \
    alsa \
    autoconf `# libnice` \
    automake `# libnice` \
    autopoint \
    avahi-daemon \
    avahi-utils \
    bison \
    build-essential \
    ca-certificates \
    clang \
    cmake \
    cmake-data \
    curl \
    flex \
    fonts-ipaexfont \
    gcc \
    gettext \
    git \
    gnutls-dev `# libnice` \
    gtk-doc-tools `# libnice` \
    libasound2-dev \
    libass-dev \
    libavahi-client-dev \
    libavahi-common-dev \
    libavcodec-dev \
    libavformat-dev \
    libdbus-1-dev \
    libexpat-dev \
    libfdk-aac-dev \
    libffi-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libgles2-mesa-dev \
    libglib2.0-dev \
    libgpac-dev \
    libgtk-3-dev \
    libjson-glib-dev \
    libjpeg-dev \
    libmp3lame-dev \
    libnice-dev \
    libomxil-bellagio-bin \
    libopus-dev \
    libpcre3-dev \
    libpng-dev \
    libsoup-gnome2.4-dev \
    librtmp-dev \
    libsdl-dev \
    libsdl-gfx1.2-dev \
    libsrtp2-dev \
    libssl-dev `# needed for DTLS requirement`\
    libswscale-dev \
    libtheora-dev \
    libtool `# libnice` \
    libudev-dev \
    libva-dev \
    libvdpau-dev \
    libvpx-dev \
    libvorbis-dev \
    libx264-dev \
    libx265-dev \
    libxext-dev \
    libxfixes-dev \
    libxslt-dev \
    libxml2-dev \
    locales \
    mount \
    nasm \
    ninja-build \
    perl \
    pkg-config \
    python \
    python3-pip \
    python3-setuptools \
    tclsh \
    texi2html \
    v4l-utils \
    wget \
    yasm \
    zlib1g

# meson のインストール
RUN pip3 install wheel
RUN pip3 install meson==0.59.2

# ビルドオプション
ENV BUILD_OPTION="-Dgupnp=disabled -Dwith_x11=no -Db_lto=true"

# ビルドタイプ
ENV BUILD_TYPE="release"

# ビルド用のフォルダを作成
RUN mkdir -p ${BUILD_FOLDER} 

# -----------------------------------------------------------------------------
# openh264 のインストール
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& git clone https://github.com/cisco/openh264.git --depth 1 \
&& cd openh264 \
&& meson --buildtype=${BUILD_TYPE} build \
&& ninja -C build install

# -----------------------------------------------------------------------------
# SRT のインストール
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& git clone -b v${SRT_VERSION} https://github.com/Haivision/srt.git --depth 1 \
&& cd srt/ \
&& ./configure \
&& make \
&& make install \
&& ldconfig

# ffmpeg のビルドとインストール
RUN cd ${BUILD_FOLDER} \
&& wget https://ffmpeg.org/releases/ffmpeg-4.3.2.tar.gz \
&& tar -xvzof ffmpeg-4.3.2.tar.gz \
&& cd ffmpeg-4.3.2/ \
&& ./configure \
  --extra-libs="-lpthread -lm -lz" \
  --enable-gpl \
  --enable-libass \
  --enable-libfdk-aac \
  --enable-libfreetype \
  --enable-libmp3lame \
  --enable-libopus \
  --enable-libtheora \
  --enable-libvorbis \
  --enable-libvpx \
  --enable-libx264 \
  --enable-libx265 \
  --enable-nonfree \
&& make \
&& make install

# -----------------------------------------------------------------------------
# gstreamer のインストール
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gstreamer-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gstreamer-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# orc のインストール
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/orc/orc-${ORC_VERSION}.tar.xz \
&& tar xvfJ orc-${ORC_VERSION}.tar.xz > /dev/null \
&& cd orc-${ORC_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# gst-plugins-base
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-plugins-base-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-plugins-base-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# libnice
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& git clone https://gitlab.freedesktop.org/libnice/libnice.git \
&& cd libnice \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# gst-plugins-good
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-plugins-good-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-plugins-good-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# gst-plugins-bad
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-plugins-bad-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-plugins-bad-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install
 
# -----------------------------------------------------------------------------
# gst-plugins-ugly
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gst-plugins-ugly/gst-plugins-ugly-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-plugins-ugly-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-plugins-ugly-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# -----------------------------------------------------------------------------
# gst-libav
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER}  \
&& wget https://gstreamer.freedesktop.org/src/gst-libav/gst-libav-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-libav-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-libav-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install 

# -----------------------------------------------------------------------------
# gst-rtsp-server
# -----------------------------------------------------------------------------

RUN cd ${BUILD_FOLDER} \
&& wget https://gstreamer.freedesktop.org/src/gst-rtsp-server/gst-rtsp-server-${GSTREAMER_VERSION}.tar.xz \
&& tar xvfJ gst-rtsp-server-${GSTREAMER_VERSION}.tar.xz > /dev/null \
&& cd gst-rtsp-server-${GSTREAMER_VERSION} \
&& meson --buildtype=${BUILD_TYPE} --prefix=${PREFIX} build ${BUILD_OPTION} \
&& ninja -C build install

# WebRTC への接続サンプルのビルド
# ビルド用のフォルダを作成
RUN mkdir -p ${WORK_FOLDER}/gst-webrtc-sample

COPY gst-webrtc-sample ${WORK_FOLDER}/gst-webrtc-sample

RUN mkdir -p ${WORK_FOLDER}/gst-webrtc-sample/build \
&& cd ${WORK_FOLDER}/gst-webrtc-sample/build \
&& cmake .. \
&& make

# TURN/STUN server
EXPOSE 3478:3478
EXPOSE 3478:3478/udp
EXPOSE 59000-65000:59000-65000/udp
# ICE server
EXPOSE 3033:3033
