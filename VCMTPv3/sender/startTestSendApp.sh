#!/bin/bash

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

./testSendApp $TCPADDR $TCPPORT $MCASTADDR $MCASTPORT $INTERFACE day1NGRID.csv
mv VCMTPv3_SENDER.log $filename$run$i$dot$ext

#mail -s "Experiment done" sschenshawn@gmail.com < /dev/null
