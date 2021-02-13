#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS kitty

# Geometry set via config file ~/.config/kitty/kitty.conf:
#
# initial_window_width  80c
# initial_window_height 24c

export UUT_EXE="kitty"

export MISSING_ANSWERBACK=yes

# set to non-empty to enable test paths that rely on VT220 support
export SUPPORTS_VT220=yes
