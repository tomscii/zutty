#!/bin/bash

cd $(dirname $0)
source testbase.sh

# Ncurses example programs used as validation tests
# To run this, install the ncurses-examples package (or similar).

CHECK_FILES /usr/lib/ncurses/examples/background
CHECK_FILES /usr/lib/ncurses/examples/testaddch

IN "/usr/lib/ncurses/examples/background\r"
SNAP ncex_01 1b67bc38a2a3c4032d29b4362277cfa1
IN "\r"
SNAP ncex_02 28e2e3411fdb59b26c5ed5386ad48211
IN "\r"
SNAP ncex_03 3594aabac24e58581be0a4dee1bf7e3b
IN "\r"
SNAP ncex_04 3bfa6caf332c3d685c9d8a71386cf86f
IN "\r"
SNAP ncex_05 14e9c1b1ace308857977d53d0dbd2280
IN "\r"
SNAP ncex_06 f768a681e54f6bf5800f3973f9cc5952
IN "\r"
SNAP ncex_07 c9f82407b358a8dfc08901a7f34f20ef
IN "\r"
SNAP ncex_08 cc6da4f06d1a623d5fdafd09d5a6b37e
IN "\r"
SNAP ncex_09 9303ef9d92d5fe1a2d640c536bd0008d
IN "\r"
SNAP ncex_10 31c399a1fb09450c14366960ca20e0ef
IN "\r"
SNAP ncex_11 e98ef8f21d8b769abb8338c0faef165f
IN "\r"

IN "/usr/lib/ncurses/examples/testaddch\r"
SNAP ncex_12 6042d7538cd12fa2dcb02a8b36ed60a6
IN "\r"
