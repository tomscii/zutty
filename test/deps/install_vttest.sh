#!/bin/bash

cd $(dirname $0)

urlbase=https://tomscii.sig7.se/files/zutty
vttest=vttest-20200610
instdir=$(pwd)

echo "===[ Downloading archive:"
wget ${urlbase}/${vttest}.tar.gz || exit 1;

echo "===[ Checking integrity of downloaded archive:"
md5sum -c MD5SUM || exit 1;

echo "===[ Unpack, compile, install ..."
tar xzf ${vttest}.tar.gz || exit 1;
cd ${vttest}
./configure --prefix=${instdir} && make && make install || exit 1;

echo
echo "===[ SUCCESS"
echo "vttest installed as: ${instdir}/bin/vttest"
