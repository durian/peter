#!/bin/sh
#
# sed -i 's/record-file=.*$/record-file=\/home\/pberck\/latest2.ts/' ../.config/mpv/mpv.conf 
# [pberck@margretetorp Rec]$ cat ../.config/mpv/mpv.conf
# record-file=/home/pberck/latest.ts
# vo=caca
#
# ./laucher.sh foo.ts caca
# ./laucher.sh foo.ts
#
HOME='/home/pberck/'
#
if [ $# -eq 2 ]; then
    # arg 2 is ignored, if present make it caca output
    cp ../.config/mpv/mpv.conf_X ../.config/mpv/mpv.conf
    TSF=$1
    echo sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf 
    sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf 
fi
if [ $# -eq 1 ]; then
    # one arg is just output to latest and caca
    TSF=latest.ts
    cp ../.config/mpv/mpv.conf_X ../.config/mpv/mpv.conf
    echo sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf 
    sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf
fi
if [ $# -eq 0 ]; then
    TSF=latest.ts
    cp ../.config/mpv/mpv.conf_caca ../.config/mpv/mpv.conf
    echo sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf 
    sed -i "s|record-file=.*$|record-file=${HOME}${TSF}|" ../.config/mpv/mpv.conf
fi
#cp ../.config/mpv/mpv.conf_caca ../.config/mpv/mpv.conf
#
ACEID=$(cat aceid.txt)
acestream-launcher acestream://${ACEID}
