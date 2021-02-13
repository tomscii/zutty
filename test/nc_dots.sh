#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

export VERIFY_SNAPS=no # Override profile setting

CHECK_FILES /usr/lib/ncurses/examples/dots

IN "timeout -s ALRM 10 /usr/lib/ncurses/examples/dots\r"
sleep 12
SNAP nc_dots
