#!/usr/bin/bash
#
#ffmpeg -i rec1.ts -c copy rec1a.ts
#ffmpeg -i rec1a.ts -c copy rec1.mp4
#
#
if [ $# -ne 2 ]; then
    echo "Supply original file and new file"
    exit 1
fi
#
# Give time for stream to close?
sleep 4
#
TSF=$1
M4F=$2
#
TMP=$(mktemp)".ts"
echo ffmpeg -i ${TSF} -c copy ${TMP}
R1=$(ffmpeg -analyzeduration 2147483647 -probesize 2147483647 -y -nostats -loglevel 0 -i ${TSF} -c copy ${TMP})
echo ffmpeg -i ${TMP} -c copy ${M4F}
R2=$(ffmpeg -analyzeduration 2147483647 -probesize 2147483647 -y -nostats -loglevel 0 -i ${TMP} -c copy ${M4F})
rm ${TMP}
