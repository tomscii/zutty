#!/usr/bin/env bash

cd $(dirname $0)

echo "Running all automated tests with --ci-mode $@ ..." && \
    ./keys.sh --ci-mode $@ && \
    ./nonascii.sh --ci-mode $@ && \
    ./scrollback.sh --ci-mode $@ && \
    ./title.sh --ci-mode $@ && \
    ./truecolor.sh --ci-mode $@ && \
    ./utf8.sh --ci-mode $@ && \
    ./vttest.sh --ci-mode $@ && \
    ./wraptest.sh --ci-mode $@ && \
    echo "All tests ran successfully, no errors detected!"
