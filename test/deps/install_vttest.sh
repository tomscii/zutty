#!/usr/bin/env bash

cd $(dirname $0)

urlbase=ftp://ftp.invisible-island.net/vttest
vttest=vttest-20210210
instdir=$(pwd)
mkdir -p $instdir/bin

echo "===[ Downloading archive:"
wget ${urlbase}/${vttest}.tgz || exit 1;

echo "===[ Checking integrity of downloaded archive:"
grep ${vttest} MD5SUM | md5sum -c - || exit 1;

echo "===[ Unpack, compile, install ..."
tar xzf ${vttest}.tgz || exit 1;
cd ${vttest}
./configure --prefix=${instdir} && make && make install || exit 1;

echo
echo "===[ SUCCESS"
echo "vttest installed as: ${instdir}/bin/vttest"
