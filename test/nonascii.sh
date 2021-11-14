#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

IN "source nonascii_inc.sh\r"
SNAP nonascii_01 1b542851d2a28909ccdaafa1f82815f6

IN "\r"
SNAP nonascii_02 d3b46685db03f87300996a736d25483b

IN "\r"
SNAP nonascii_03 7a2b1d880bd064ce353dd846f2a223f4
