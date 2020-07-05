#!/bin/bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS rxvt

export UUT_EXE="rxvt -geometry 80x24 -fn x:9x15 -rv +sb"
