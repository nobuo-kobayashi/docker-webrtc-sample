FROM node:16

# プログラムを実行するフォルダ
ENV WORK_FOLDER /opt

RUN cd ${WORK_FOLDER} \
&& npm install express \
&& npm install express-ws \
&& npm install request \
&& npm install websocket

# 作業用のフォルダ指定
WORKDIR ${WORK_FOLDER}/

# 実行
CMD cd ${WORK_FOLDER}/signaling/src \
&& node app.js
