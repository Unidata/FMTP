#!/bin/bash

#./testSendApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0 day1NGRID.csv

fullname="VCMTPv3_SENDER.log"
filename="${fullname%.*}"
ext="${fullname##*.}"
run="_run"
dot="."

TCPADDR="10.10.1.1"
TCPPORT="1234"
MCASTADR="224.0.0.1"
MCASTPORT="5173"
INTERFACE="10.10.1.1"

for i in {1..10}
do
    ./testSendApp $TCPADDR $TCPPORT $MCASTADDR $MCASTPORT $INTERFACE day1NGRID.csv
    mv VCMTPv3_SENDER.log $filename$run$i$dot$ext
    sleep 2
done

#mail -s "Experiment done" sschenshawn@gmail.com < /dev/null
