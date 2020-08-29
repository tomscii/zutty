#!/bin/bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_EXE ../build/src/zutty

export UUT_EXE="../build/src/zutty -geometry 80x24 -v"

# set to non-empty to enable test paths that rely on VT220 support
export SUPPORTS_VT220=yes
