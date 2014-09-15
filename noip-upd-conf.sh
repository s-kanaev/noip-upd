#!/bin/bash

if [ -z "$1" ]; then
   echo "USAGE - $0 <sonfig-filename-to-write-configuration-to>";
   exit 1;
fi;

echo -n "Type in no-ip login - "
read LOGIN
echo -n "Type in no-ip password - "
read PASSWORD
echo -n "Type in no-ip hostname - "
read HOSTNAME
echo -n "Type in no-ip dynamic update host (or just type in dynupdate.no-ip.com) - "
read UPD_HOST

#echo -e "$LOGIN\n$PASSWORD\n$HOSTNAME\n$UPD_HOST"

LN_PASS_ENC=`echo $LOGIN:$PASSWORD| base64`;
echo -e "$LN_PASS_ENC\n$HOSTNAME\n$UPD_HOST" > $1
