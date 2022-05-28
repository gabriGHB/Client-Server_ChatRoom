#!/bin/sh

# set up environment variables for server IP & port
export SERVER_IP=$1
export SERVER_PORT=$2

cd build
app/server -p $SERVER_PORT > /dev/null 2>&1 &
python ../python/serverWS.py > /dev/null 2>&1 &
python ../python/tests.py
pkill -SIGINT '^server$'
pkill -SIGTERM '^python$'