FROM ubuntu:18.04

# ビルド処理を行うフォルダの設定
ENV BUILD_FOLDER /tmp/build

RUN apt-get update -y \
&& apt-get install -y \
  curl \
  libssl-dev \
  libevent-dev \
  sqlite3 \
  make \
  gcc \
  pkg-config \
  wget

# ビルド用のフォルダを作成
RUN mkdir -p ${BUILD_FOLDER} 

# coturn のバージョンを設定
ENV COTURN_VERSION 4.5.1.3

RUN cd ${BUILD_FOLDER} \
&& wget http://turnserver.open-sys.org/downloads/v${COTURN_VERSION}/turnserver-${COTURN_VERSION}.tar.gz \
&& tar xvfz turnserver-${COTURN_VERSION}.tar.gz \
&& cd turnserver-${COTURN_VERSION} \
&& ./configure \
&& make \
&& make install

COPY conf/turnserver.conf /usr/local/etc/turnserver.conf

ADD start_coturn.sh start_coturn.sh
RUN chmod +x start_coturn.sh

# turnutils_uclient -t -p 3479 -u 'username' -w 'password' 127.0.0.1

CMD ["./start_coturn.sh"]
