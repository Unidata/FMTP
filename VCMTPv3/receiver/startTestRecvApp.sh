#!/bin/bash

#./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0

# get the second IP address in OS
#bindip=`hostname -I | awk -F ' ' '{print $2}'`

TCPADDR="10.10.1.1"
TCPPORT="1234"
MCASTADDR="224.0.0.1"
MCASTPORT="5173"

run="_run"
dot="."

for i in {1..10}
do
    ./testRecvApp $TCPADDR $TCPPORT $MCASTADDR $MCASTPORT $bindip
    newlog=$(ls -t logs | head -n1)
    filename="${newlog%.*}"
    ext="${newlog##*.}"
    mv logs/$newlog logs/$filename$run$i$dot$ext
    sleep 6
done
