#!/usr/bin/env bash

cd $(dirname $0)

# Use a specific version (current latest commit):
url=https://github.com/mattiase/wraptest/raw/d68475bc0ef9776289090874b7b6b9bbbfe5c210/wraptest.c
instdir=$(pwd)
mkdir -p $instdir/bin

echo "===[ Downloading archive:"
wget ${url} || exit 1;

echo "===[ Checking integrity of downloaded source:"
grep wraptest.c MD5SUM | md5sum -c - || exit 1;

echo "===[ Compile, install ..."
make wraptest && mv wraptest bin/ || exit 1;

echo
echo "===[ SUCCESS"
echo "wraptest installed as: ${instdir}/bin/wraptest"
