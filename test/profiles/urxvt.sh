#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS urxvt

FONT=-misc-fixed-medium-r-normal--18-120-100-100-c-90-iso10646-1
export UUT_EXE="urxvt -geometry 80x24 -fn $FONT -rv +sb"
