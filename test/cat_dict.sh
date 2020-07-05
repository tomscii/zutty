#!/bin/bash

cd $(dirname $0)
source testbase.sh

export VERIFY_SNAPS=no # Override profile setting

DICT=/usr/share/dict/british-english-huge
CHECK_FILES ${DICT}

IN "time cat ${DICT} && touch .complete\r"

WAIT_FOR_DOT_COMPLETE

SNAP cat_dict
