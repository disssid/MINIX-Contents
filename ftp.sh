#!/bin/bash
HOST=''
USER=''
PASSWD=''
FNM=$1

ftp -inv $HOST <<END_SCRIPT
user $USER $PASSWD
get $FNM
quit
END_SCRIPT

exit 0
