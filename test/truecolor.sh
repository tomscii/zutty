#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

IN "source truecolor_inc_01.sh\r"
SNAP truecolor_01 33a31e4d3b9fbe486c27b01764dc1823

IN "source truecolor_inc_02.sh\r"
SNAP truecolor_02 d1c9a0265db41f2aa237c95510565dd2
