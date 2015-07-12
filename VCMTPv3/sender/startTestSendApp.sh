#!/bin/bash

#./testSendApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0 day1NGRID.csv

fullname="VCMTPv3_SENDER.log"
filename="${fullname%.*}"
ext="${fullname##*.}"
run="_run"
dot="."

mkdir sender-log
for i in {1..10}
do
    ./testSendApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0 day1NGRID.csv
    mv VCMTPv3_SENDER.log sender-log/$filename$run$i$dot$ext
done

mail -s "Experiment done" sschenshawn@gmail.com < /dev/null
