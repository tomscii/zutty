#!/bin/bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS xterm

export UUT_EXE="xterm -j -u8 -geometry 80x24"

# set to non-empty to enable test paths that rely on VT52 support
export SUPPORTS_VT52=yes

# set to non-empty to enable test paths that rely on VT220 support
export SUPPORTS_VT220=yes

export MISSING_ANSWERBACK=yes
