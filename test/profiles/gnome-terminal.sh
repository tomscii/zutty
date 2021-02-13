#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS gnome-terminal

# Manual setup:
# To avoid interfering with the keyboard tests, remove F11 from
# being assigned as a hot-key at: Shortcuts / View / Full-screen

export UUT_EXE="gnome-terminal --geometry=80x24 --hide-menubar"

# The real pid is a child of the one we started
export UUT_PID_FROM_NAME="gnome-terminal-server"

export MISSING_ANSWERBACK=yes

# set to non-empty to enable test paths that rely on VT220 support
export SUPPORTS_VT220=yes
