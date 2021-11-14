#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

function show {
    offset=$1
    IN "printf \"\\\\e[H\\\\e[J\" && tail +${offset} UTF-8-test.txt | head -23 && sleep 2\r"
}

show 63
SNAP utf8_01 2bf11d4ae3cced3cafa84bdf611f0a46
show 89
SNAP utf8_02 67e158e17a32cf6227fa76cabede3dbe
show 112
SNAP utf8_03 185e4a2474fb8bd203be4fd4beb8fa8b
show 132
SNAP utf8_04 9584f8145fc4621e55a9b1878a401770
show 152
SNAP utf8_05 cd79027cf70c539f0daa5935d28ca233
show 171
SNAP utf8_06 de9c9611cde53ed0166617634409c4d3
show 203
SNAP utf8_07 452d347b373e25ed113f69312c7cc37c
show 226
SNAP utf8_08 d1f380e30af15d0d3900c390f75ec6a6
show 245
SNAP utf8_09 9fe99776dfa6640d299fceea845dbb17
show 286
SNAP utf8_10 e4ecf831f88757880fc1ee9ad0d7f95c
