#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

which wraptest >/dev/null
if [ $? -gt 0 ] ; then
    printf "${YELLOW}Please run test/deps/install_wraptest.sh${DFLT}\n"
fi
CHECK_DEPS wraptest

IN "wraptest | head -23 && sleep 2\r"
SNAP wraptest_01 3210527d9b8d4f947950da94cc93f27f

IN "wraptest | tail -23 && sleep 2\r"
SNAP wraptest_02 40f7ea02451cdf255ab0f6f50d202275

IN "source wraptest_inc.sh\r"
SNAP wraptest_03 61dc7ff7001304ee6810afd95c82ac49

IN "\r"
SNAP wraptest_04 b10b13359004efdd5e5e728953caf3a7
