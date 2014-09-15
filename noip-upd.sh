#!/bin/bash
. ./noip-upd.inc

#touch $ALLOW_UPD_F
if [ ! -e $ALLOW_UPD_F ]; then
    echo $ALLOW_WORD > $ALLOW_UPD_F;
fi;

ALLOW_UPD=`cat $ALLOW_UPD_F`
if [ -z $ALLOW_UPD ]; then
    echo $ALLOW_WORD > $ALLOW_UPD_F;
    ALLOW_UPD=`cat $ALLOW_UPD_F`
fi;

if [ ! -x $NOIP_UPD ]; then
    echo $NO_EXEC > $ALLOW_UPD_F
    echo "No executable @ $NOIP_UPD"
    exit 1;
fi;

if [ $ALLOW_UPD = $DISALLOW_WORD ]; then
    echo "noip update process is disallowed"
    exit 1;
fi;

if [ $ALLOW_UPD = $ALLOW_WORD ]; then
    $NOIP_UPD $NOIP_UPD_CONF
    STATUS=$?;
    if [ $STATUS -gt 0 -a $STATUS -lt 7 ]; then
	echo $DISALLOW_WORD > $ALLOW_UPD_F
	exit 1;
    fi;
    TIME=0;
    let "TIME+=TIMEm"
    let "TIME+=(TIMEh*60)"
    echo "$0" | at now + $TIME minutes;
fi;