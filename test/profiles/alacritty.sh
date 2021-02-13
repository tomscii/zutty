#!/usr/bin/env bash
if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "This script should not be directly executed!"
    exit 1
fi

CHECK_DEPS alacritty

export UUT_EXE="alacritty -d 80 24"

export MISSING_ANSWERBACK=yes
export MISSING_SECONDARY_DA=yes
