#!/bin/bash

cd $(dirname $0)
source testbase.sh

function INS_DEL_PGUPDN {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing Ins/Del/PageUp/PageDown in normal mode. \D1"
    IN "No modifiers: \D1"
    IN "\[Insert] \[Delete] \[Page_Up] \[Page_Down] \D1"
    IN "\[KP_Insert] \[KP_Delete] \[KP_Page_Up] \[KP_Page_Down] \D1"

    IN "Shift: \D1"
    # Exclude Shift-Insert b/c it is bound to "Paste"
    IN "\S\[Delete] \S\[Page_Up] \S\[Page_Down] \D1"
    # N.B.: Shift state is not set on XKeyPressedEvent for keypad keys
    # (may be a xvkbd limitation), so omit testing those

    IN "Alt: \D1"
    IN "\A\[Insert] \A\[Delete] \A\[Page_Up] \A\[Page_Down] \D1"
    IN "\A\[KP_Insert] \A\[KP_Delete] \A\[KP_Page_Up] \A\[KP_Page_Down] \D1"

    IN "Control: \D1"
    IN "\C\[Insert] \C\[Delete] \C\[Page_Up] \C\[Page_Down] \D1"
    IN "\C\[KP_Insert] \C\[KP_Delete] \C\[KP_Page_Up] \C\[KP_Page_Down] \D1"

    IN "Alt-Shift: \D1"
    IN "\A\S\[Delete] \A\S\[Page_Up] \A\S\[Page_Down] \D1"

    IN "Control-Shift: \D1"
    IN "\C\S\[Delete] \C\S\[Page_Up] \C\S\[Page_Down] \D1"

    IN "Control-Alt: \D1"
    IN "\C\A\[Insert] \C\A\[Delete] \C\A\[Page_Up] \C\A\[Page_Down] \D1"
    IN "\C\A\[KP_Insert] \C\A\[KP_Delete] \C\A\[KP_Page_Up] \C\A\[KP_Page_Down] \D1"

    IN "Control-Alt-Shift: \D1"
    IN "\C\A\S\[Delete] \C\A\S\[Page_Up] \C\A\S\[Page_Down] \D1"

    SNAP keys_01 c5f7e954e68cec59e4ad1e22d4aa519b

    IN "\Cd\Cd\D3"
}

function FUNCTION_KEYS {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing function keys in normal mode. \D1"
    IN "No modifiers: \D1"
    IN "\[F1] \[F2] \[F3] \[F4] \[F5] \[F6] \[F7] \[F8] \[F9] \[F10] \[F11] \D2"
    IN "\[F12] \[F13] \[F14] \[F15] \[F16] \[F17] \[F18] \[F19] \[F20] \D2"

    # Sending Shift does not seem to work (xvkbd limitation?)

    IN "Alt: \D1"
    IN "\A\[F1] \A\[F2] \A\[F3] \A\[F4] \A\[F5] \A\[F6] \A\[F7] \A\[F8] \D2"
    IN "\A\[F9] \A\[F10] \A\[F11] \A\[F12] \A\[F13] \A\[F14] \A\[F15] \D2"
    IN "\A\[F16] \A\[F17] \A\[F18] \A\[F19] \A\[F20] \D2"

    IN "Control: \D1"
    IN "\C\[F1] \C\[F2] \C\[F3] \C\[F4] \C\[F5] \C\[F6] \C\[F7] \C\[F8] \D2"
    IN "\C\[F9] \C\[F10] \C\[F11] \C\[F12] \C\[F13] \C\[F14] \C\[F15] \D2"
    IN "\C\[F16] \C\[F17] \C\[F18] \C\[F19] \C\[F20] \D2"

    # Skip Control-Alt and Control-Alt-Shift, because it switches VT

    SNAP keys_02 21b33a9ecf4d78123aa75d04ec986b7a

    IN "\Cd\Cd\D3"
}

function CURSOR_KEYS {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing cursor keys in normal mode. \D1"
    IN "No modifiers: \D1"
    IN "\[Up] \[Down] \[Right] \[Left] \[Home] \[End] \D2"

    IN "Shift: \D1"
    IN "\S\[Up] \S\[Down] \S\[Right] \S\[Left] \S\[Home] \S\[End] \D2"

    IN "Alt: \D1"
    IN "\A\[Up] \A\[Down] \A\[Right] \A\[Left] \A\[Home] \A\[End] \D2"

    IN "Control: \D1"
    IN "\C\[Up] \C\[Down] \C\[Right] \C\[Left] \C\[Home] \C\[End] \D2"

    IN "Alt-Shift: \D1"
    IN "\A\S\[Up] \A\S\[Down] \A\S\[Right] \A\S\[Left] \A\S\[Home] \A\S\[End] \D2"

    IN "Control-Shift: \D1"
    IN "\C\S\[Up] \C\S\[Down] \C\S\[Right] \C\S\[Left] \C\S\[Home] \C\S\[End] \D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[Up] \C\A\[Down] \C\A\[Right] \C\A\[Left] \C\A\[Home] \C\A\[End] \D2"

    IN "Control-Alt-Shift: \D1"
    IN "\C\A\S\[Up] \C\A\S\[Down] \C\A\S\[Right] \C\A\S\[Left] \C\A\S\[Home] \C\A\S\[End] \D2"

    SNAP keys_03 0721436bc76609f83491fbb652045c5d

    IN "\Cd\Cd\D3"
}

function KEYPAD_KEYS {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing keypad keys in normal mode. \D1"
    IN "No modifiers: \D1"
    IN "\[KP_Enter] \[KP_Tab] \[KP_Space] \[KP_Multiply] \[KP_Divide] "
    IN "\[KP_Add] \[KP_Subtract] \[KP_Separator] \[KP_Decimal] \[KP_Equal] "
    IN "\[KP_0] \[KP_1] \[KP_2] \[KP_3] \[KP_4] "
    IN "\[KP_5] \[KP_6] \[KP_7] \[KP_8] \[KP_9] \D2"

    SNAP keys_04 472833543e22148464efaebc61283156

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[H\\\\e[J\\\\e=\" && cat -vT\r"

    IN "Testing keypad keys in application keypad mode. \D1"
    IN "No modifiers: \D1"
    IN "\[KP_Enter] \[KP_Tab] \[KP_Space] \[KP_Multiply] \[KP_Divide] "
    IN "\[KP_Add] \[KP_Subtract] \[KP_Separator] \[KP_Decimal] \[KP_Equal] "
    IN "\[KP_0] \[KP_1] \[KP_2] \[KP_3] \[KP_4] "
    IN "\[KP_5] \[KP_6] \[KP_7] \[KP_8] \[KP_9] \D2"

    # Omitting Shift as it does not get through for all keys (xvkbd?)

    IN "Alt: \D1"
    IN "\A\[KP_Enter] \A\[KP_Tab] \A\[KP_Space] \A\[KP_Multiply] \A\[KP_Divide] "
    IN "\A\[KP_Add] \A\[KP_Subtract] \A\[KP_Separator] \A\[KP_Decimal] \A\[KP_Equal] "
    IN "\A\[KP_0] \A\[KP_1] \A\[KP_2] \A\[KP_3] \A\[KP_4] "
    IN "\A\[KP_5] \A\[KP_6] \A\[KP_7] \A\[KP_8] \A\[KP_9] \D2"

    IN "Control: \D1"
    IN "\C\[KP_Enter] \C\[KP_Tab] \C\[KP_Space] \C\[KP_Multiply] \C\[KP_Divide] "
    IN "\C\[KP_Add] \C\[KP_Subtract] \C\[KP_Separator] \C\[KP_Decimal] \C\[KP_Equal] "
    IN "\C\[KP_0] \C\[KP_1] \C\[KP_2] \C\[KP_3] \C\[KP_4] "
    IN "\C\[KP_5] \C\[KP_6] \C\[KP_7] \C\[KP_8] \C\[KP_9] \D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[KP_Enter] \C\A\[KP_Tab] \C\A\[KP_Space] "
    IN "\C\A\[KP_Separator] \C\A\[KP_Decimal] \C\A\[KP_Equal] "
    IN "\C\A\[KP_0] \C\A\[KP_1] \C\A\[KP_2] \C\A\[KP_3] \C\A\[KP_4] "
    IN "\C\A\[KP_5] \C\A\[KP_6] \C\A\[KP_7] \C\A\[KP_8] \C\A\[KP_9] \D2"

    SNAP keys_05 ff17e245b367c04151b9a5bed35b30ae

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e>\"\r"
}

function ALT_SENDS_ESC {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing Alt-sends-Esc in normal mode. \D1"
    IN "a A \Aa \AA \C\Aa \C\AA 2 \A2 \C\A2 # \A# \C\A#"
    IN "/ ? \A/ \A? \C\A/ \C\A?"

    SNAP keys_06 86428d69167433775c8481df47f15a87

    IN "\Cd\Cd\D3"
    IN "stty -echo && printf \"\\\\e[H\\\\e[J\\\\e[12l\" && cat -vT\r"

    IN "Testing Alt-sends-Esc with local echo on. \D1"
    IN "a A \Aa \AA \C\Aa \C\AA 2 \A2 \C\A2 # \A# \C\A# "
    IN "/ ? \A/ \A? \C\A/ \C\A?"

    SNAP keys_07 be8f0c856c9e80a14c9e0f959d78b6f9

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[12h\" && stty echo\r"
}

INS_DEL_PGUPDN
FUNCTION_KEYS
CURSOR_KEYS
KEYPAD_KEYS
ALT_SENDS_ESC
