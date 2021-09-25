#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

if [ ${DEBUG} == "yes" ]; then
    EXE=zutty.dbg
else
    EXE=zutty
fi
CHECK_EXE ../build/src/${EXE}

# Report actually tested version
export UUT_VERSION=$(../build/src/${EXE} -h | head -1)

export UUT_EXE="../build/src/${EXE} -geometry 80x24 -v"

# set to non-empty to enable test paths that rely on VT52 support
export SUPPORTS_VT52=yes

# set to non-empty to enable test paths that rely on VT220 support
export SUPPORTS_VT220=yes

export MISSING_ANSWERBACK=yes
