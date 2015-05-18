#!/bin/sh

# get the second IP address in OS
#bindip=`hostname -I | awk -F ' ' '{print $2}'`
#./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 $bindip
./testRecvApp 127.0.0.1 1234 233.0.225.123 5173 0.0.0.0
