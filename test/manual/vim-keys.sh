#!/bin/bash

# Run this script inside Zutty.
# Test by manually pressing the keys listed below and verifying
# that the modeline confirms the combination being recognized.

vim -Nu NONE \
    +'nno <C-i>     :echom "C-i was pressed"<cr>' \
    +'nno <tab>     :echom "Tab was pressed"<cr>' \
    +'nno <S-tab>   :echom "S-Tab was pressed"<cr>' \
    +'nno <C-a>     :echom "C-a was pressed"<cr>' \
    +'nno <C-S-a>   :echom "C-S-a was pressed"<cr>' \
    +'nno <A-b>     :echom "A-b was pressed"<cr>' \
    +'nno <A-C-b>   :echom "A-C-b was pressed"<cr>' \
    +'nno <C-3>     :echom "C-3 was pressed"<cr>' \
    +'nno <C-;>     :echom "C-; was pressed"<cr>' \
    +'nno <C-space> :echom "C-space was pressed"<cr>' \
    $0
