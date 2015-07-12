#!/bin/bash

# get the second IP address in OS
#bindip=`hostname -I | awk -F ' ' '{print $2}'`
#./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 $bindip
#./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0

run="_run"
dot="."

for i in {1..10}
do
    ./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 $bindip
    newlog=$(ls -t logs | head -n1)
    filename="${newlog%.*}"
    ext="${newlog##*.}"
    mv logs/$newlog logs/$filename$run$i$dot$ext
    sleep 6
done
