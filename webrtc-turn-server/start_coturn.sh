#!/bin/bash

echo "Starting TURN/STUN server"

# Public IP アドレスを取得
PUBLIC_IP=`curl https://api.ipify.org`

# ローカル IP アドレスを取得
LOCAL_IP=`hostname -I`

echo "-----------------------"
echo "PublicIP: "$PUBLIC_IP
echo "LocalIP:  "$LOCAL_IP
echo "-----------------------"

turnserver -c /usr/local/etc/turnserver.conf -u username:password -L $LOCAL_IP-X $PUBLIC_IP
