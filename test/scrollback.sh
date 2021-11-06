#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

CHECK_DEPS setxkbmap

function BASIC_TEST {
    IN "source scrollback_inc.sh\r"

    IN "\S\[Page_Up]\D1"
    SNAP scrollback_01 79872480f485e1db04ccf521f3273479

    IN "\S\[Page_Up]\D1"
    SNAP scrollback_02 62109072a8df5853fcbdb53a0e7b1827

    for x in {0..9} ; do
        IN "\S\[Page_Up]\S\[Page_Up]\S\[Page_Up]\S\[Page_Up]"
    done
    SNAP scrollback_03 44940cbc78ebd01f68c892108be08ac7

    # Identical to previous snap; already at top of history
    IN "\S\[Page_Up]\D1"
    SNAP scrollback_04 44940cbc78ebd01f68c892108be08ac7

    # Test jump-to-bottom on input:
    IN "f"
    SNAP scrollback_05 4083e797c0f80b427cb4e3a074f22ca2

    IN "or x in {0..9} ; do echo \$x; done\r"

    IN "\S\[Page_Up]\D1"
    SNAP scrollback_06 84921add415a70a93abe0f8db3303b76

    IN "\S\[Page_Up]\S\[Page_Up]\S\[Page_Up]\D1"
    SNAP scrollback_07 6ed43310442afa8e9e814edc0e2a5055

    IN "\S\[Page_Down]\D1"
    SNAP scrollback_08 15dc6f1a71bbdbf5d53abfd02aa82b9e

    IN "\S\[Page_Down]\S\[Page_Down]\D1"
    SNAP scrollback_09 84921add415a70a93abe0f8db3303b76

    IN "\S\[Page_Down]\S\[Page_Down]\D1"
    SNAP scrollback_10 7db02afdd39f676edca19de71bbcfa84

    IN "printf \"\\\\e[H\\\\e[3J\"\r" # clear display incl. scrollback
    IN "\S\[Page_Up]\S\[Page_Up]\S\[Page_Up]\S\[Page_Up]\D1"
    SNAP scrollback_11 96c997cd31ac8aa77cd419d08bcdad34
}

BASIC_TEST
