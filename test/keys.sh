#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

CHECK_DEPS setxkbmap

function INS_DEL_PGUPDN {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing Ins/Del/PageUp/PageDown in normal mode. \n\D1"
    IN "No modifiers: \D1"
    IN "\[Insert] \[Delete] \[Page_Up] \[Page_Down] \D1"

    IN "Shift: \D1"
    # Exclude Shift-Insert b/c it is bound to "Paste"
    # Exclude Shift-PageUp and Shift-PageDown (bound to scroll up/down)
    IN "\S\[Delete] \D1"

    IN "Alt: \D1"
    IN "\A\[Insert] \A\[Delete] \A\[Page_Up] \A\[Page_Down] \D1"

    IN "Control: \D1"
    IN "\C\[Insert] \C\[Delete] \C\[Page_Up] \C\[Page_Down] \D1"

    IN "Alt-Shift: \D1"
    IN "\A\S\[Delete] \A\S\[Page_Up] \A\S\[Page_Down] \D1"

    IN "Control-Shift: \D1"
    IN "\C\S\[Delete] \C\S\[Page_Up] \C\S\[Page_Down] \D1"

    IN "Control-Alt: \D1"
    IN "\C\A\[Insert] \C\A\[Delete] \C\A\[Page_Up] \C\A\[Page_Down] \D1"

    IN "Control-Alt-Shift: \D1"
    IN "\C\A\S\[Delete] \C\A\S\[Page_Up] \C\A\S\[Page_Down] \D1"

    SNAP keys_01 778895af5c80b2a19acc2bb2b9b048b5

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

    IN "Testing cursor keys in normal mode (DECCKM).\n\D1"
    IN "No modifiers: \D1"
    IN "\[Up] \[Down] \[Right] \[Left] \[Home] \[End] \n\D2"

    IN "Shift: \D1"
    IN "\S\[Up] \S\[Down] \S\[Right] \S\[Left] \S\[Home] \S\[End] \n\D2"

    IN "Alt: \D1"
    IN "\A\[Up] \A\[Down] \A\[Right] \A\[Left] \A\[Home] \A\[End] \n\D2"

    IN "Control: \D1"
    IN "\C\[Up] \C\[Down] \C\[Right] \C\[Left] \C\[Home] \C\[End] \n\D2"

    IN "Alt-Shift: \D1"
    IN "\A\S\[Up] \A\S\[Down] \A\S\[Right] \A\S\[Left] \A\S\[Home] \A\S\[End] \n\D2"

    IN "Control-Shift: \D1"
    IN "\C\S\[Up] \C\S\[Down] \C\S\[Right] \C\S\[Left] \C\S\[Home] \C\S\[End] \n\D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[Up] \C\A\[Down] \C\A\[Right] \C\A\[Left] \C\A\[Home] \C\A\[End] \n\D2"

    IN "Control-Alt-Shift: \D1"
    IN "\C\A\S\[Up] \C\A\S\[Down] \C\A\S\[Right] \C\A\S\[Left] \C\A\S\[Home] \C\A\S\[End] \n \D2"

    SNAP keys_03 f1a4d62a34b3a9cbbc79369eb320ff4e

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[H\\\\e[J\\\\e[?1h\" && cat -vT\r"

    IN "Testing cursor keys in application mode (DECAKM).\n\D1"
    IN "No modifiers: \D1"
    IN "\[Up] \[Down] \[Right] \[Left] \[Home] \[End] \n\D2"

    IN "Shift: \D1"
    IN "\S\[Up] \S\[Down] \S\[Right] \S\[Left] \S\[Home] \S\[End] \n\D2"

    IN "Alt: \D1"
    IN "\A\[Up] \A\[Down] \A\[Right] \A\[Left] \A\[Home] \A\[End] \n\D2"

    IN "Control: \D1"
    IN "\C\[Up] \C\[Down] \C\[Right] \C\[Left] \C\[Home] \C\[End] \n\D2"

    IN "Alt-Shift: \D1"
    IN "\A\S\[Up] \A\S\[Down] \A\S\[Right] \A\S\[Left] \A\S\[Home] \A\S\[End] \n\D2"

    IN "Control-Shift: \D1"
    IN "\C\S\[Up] \C\S\[Down] \C\S\[Right] \C\S\[Left] \C\S\[Home] \C\S\[End] \n\D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[Up] \C\A\[Down] \C\A\[Right] \C\A\[Left] \C\A\[Home] \C\A\[End] \n\D2"

    IN "Control-Alt-Shift: \D1"
    IN "\C\A\S\[Up] \C\A\S\[Down] \C\A\S\[Right] \C\A\S\[Left] \C\A\S\[Home] \C\A\S\[End] \n \D2"

    SNAP keys_04 b94f9ce3b7363c0ec559c9de338273ab

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[?1l\"\r"
}

function KEYPAD_KEYS {
    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing keypad keys in normal mode (DECKPNM).\n\D1"
    IN "No modifiers: \D1"
    # VT100 Numeric mode - test both "Numlock on" and "NumLock off" keysyms
    IN "\[KP_Tab] \[KP_Space] \[KP_Multiply] \[KP_Divide] "
    IN "\[KP_Add] \[KP_Subtract] \[KP_Separator] \[KP_Decimal] \[KP_Equal] "
    IN "\[KP_0] \[KP_1] \[KP_2] \[KP_3] \[KP_4] "
    IN "\[KP_5] \[KP_6] \[KP_7] \[KP_8] \[KP_9] \D2"
    IN "\[KP_Insert] \[KP_End] \[KP_Down] \[KP_Page_Down] \[KP_Left] "
    IN "\[KP_Begin] \[KP_Right] \[KP_Home] \[KP_Up] \[KP_Page_Up] "
    IN "\[KP_Enter]\D2"

    IN "Alt: \D1"
    IN "\A\[KP_Tab] \A\[KP_Space] \A\[KP_Multiply] \A\[KP_Divide] "
    IN "\A\[KP_Add] \A\[KP_Subtract] \A\[KP_Separator] \A\[KP_Decimal] \A\[KP_Equal] "
    IN "\A\[KP_0] \A\[KP_1] \A\[KP_2] \A\[KP_3] \A\[KP_4] "
    IN "\A\[KP_5] \A\[KP_6] \A\[KP_7] \A\[KP_8] \A\[KP_9] \D2"
    IN "\A\[KP_Insert] \A\[KP_End] \A\[KP_Down] \A\[KP_Page_Down] \A\[KP_Left] "
    IN "\A\[KP_Begin] \A\[KP_Right] \A\[KP_Home] \A\[KP_Up] \A\[KP_Page_Up] "
    IN "\A\[KP_Enter]\D2"

    IN "Control: \D1"
    IN "\C\[KP_Tab] \C\[KP_Space] \C\[KP_Multiply] \C\[KP_Divide] "
    IN "\C\[KP_Add] \C\[KP_Subtract] \C\[KP_Separator] \C\[KP_Decimal] \C\[KP_Equal] "
    IN "\C\[KP_0] \C\[KP_1] \C\[KP_2] \C\[KP_3] \C\[KP_4] "
    IN "\C\[KP_5] \C\[KP_6] \C\[KP_7] \C\[KP_8] \C\[KP_9] \D2"
    IN "\C\[KP_Insert] \C\[KP_End] \C\[KP_Down] \C\[KP_Page_Down] \C\[KP_Left] "
    IN "\C\[KP_Begin] \C\[KP_Right] \C\[KP_Home] \C\[KP_Up] \C\[KP_Page_Up] "
    IN "\C\[KP_Enter]\D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[KP_Tab] \C\A\[KP_Space] \C\A\[KP_Multiply] \C\A\[KP_Divide] "
    IN "\C\A\[KP_Add] \C\A\[KP_Subtract] \C\A\[KP_Separator] \C\A\[KP_Decimal] \C\A\[KP_Equal] "
    IN "\C\A\[KP_0] \C\A\[KP_1] \C\A\[KP_2] \C\A\[KP_3] \C\A\[KP_4] "
    IN "\C\A\[KP_5] \C\A\[KP_6] \C\A\[KP_7] \C\A\[KP_8] \C\A\[KP_9] \D2"
    IN "\C\A\[KP_Insert] \C\A\[KP_End] \C\A\[KP_Down] \C\A\[KP_Page_Down] \C\A\[KP_Left] "
    IN "\C\A\[KP_Begin] \C\A\[KP_Right] \C\A\[KP_Home] \C\A\[KP_Up] \C\A\[KP_Page_Up] "
    IN "\C\A\[KP_Enter] \D2"

    SNAP keys_05 cd54062d5b291a30fd09bea0fb122e86

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[H\\\\e[J\\\\e=\" && cat -vT\r"

    IN "Testing keypad keys in application keypad mode (DECKPAM).\n\D1"
    IN "No modifiers: \D1"
    # VT100 Application mode - test both "Numlock on" and "NumLock off" keysyms
    IN "\[KP_Tab] \[KP_Space] \[KP_Multiply] \[KP_Divide] "
    IN "\[KP_Add] \[KP_Subtract] \[KP_Separator] \[KP_Decimal] \[KP_Equal] "
    IN "\[KP_0] \[KP_1] \[KP_2] \[KP_3] \[KP_4] "
    IN "\[KP_5] \[KP_6] \[KP_7] \[KP_8] \[KP_9] \D2"
    IN "\[KP_Insert] \[KP_End] \[KP_Down] \[KP_Page_Down] \[KP_Left] "
    IN "\[KP_Begin] \[KP_Right] \[KP_Home] \[KP_Up] \[KP_Page_Up] "
    IN "\[KP_Enter]    \D2"

    IN "Alt: \D1"
    IN "\A\[KP_Tab] \A\[KP_Space] \A\[KP_Multiply] \A\[KP_Divide] "
    IN "\A\[KP_Add] \A\[KP_Subtract] \A\[KP_Separator] \A\[KP_Decimal] \A\[KP_Equal] "
    IN "\A\[KP_0] \A\[KP_1] \A\[KP_2] \A\[KP_3] \A\[KP_4] "
    IN "\A\[KP_5] \A\[KP_6] \A\[KP_7] \A\[KP_8] \A\[KP_9] \D2"
    IN "\A\[KP_Insert] \A\[KP_End] \A\[KP_Down] \A\[KP_Page_Down] \A\[KP_Left] "
    IN "\A\[KP_Begin] \A\[KP_Right] \A\[KP_Home] \A\[KP_Up] \A\[KP_Page_Up] "
    IN "\A\[KP_Enter]    \D2"

    IN "Control: \D1"
    IN "\C\[KP_Tab] \C\[KP_Space] \C\[KP_Multiply] \C\[KP_Divide] "
    IN "\C\[KP_Add] \C\[KP_Subtract] \C\[KP_Separator] \C\[KP_Decimal] \C\[KP_Equal] "
    IN "\C\[KP_0] \C\[KP_1] \C\[KP_2] \C\[KP_3] \C\[KP_4] "
    IN "\C\[KP_5] \C\[KP_6] \C\[KP_7] \C\[KP_8] \C\[KP_9] \D2"
    IN "\C\[KP_Insert] \C\[KP_End] \C\[KP_Down] \C\[KP_Page_Down] \C\[KP_Left] "
    IN "\C\[KP_Begin] \C\[KP_Right] \C\[KP_Home] \C\[KP_Up] \C\[KP_Page_Up] "
    IN "\C\[KP_Enter]    \D2"

    IN "Control-Alt: \D1"
    IN "\C\A\[KP_Tab] \C\A\[KP_Space] \C\A\[KP_Multiply] \C\A\[KP_Divide] "
    IN "\C\A\[KP_Add] \C\A\[KP_Subtract] \C\A\[KP_Separator] \C\A\[KP_Decimal] \C\A\[KP_Equal] "
    IN "\C\A\[KP_0] \C\A\[KP_1] \C\A\[KP_2] \C\A\[KP_3] \C\A\[KP_4] "
    IN "\C\A\[KP_5] \C\A\[KP_6] \C\A\[KP_7] \C\A\[KP_8] \C\A\[KP_9] \D2"
    IN "\C\A\[KP_Insert] \C\A\[KP_End] \C\A\[KP_Down] \C\A\[KP_Page_Down] \C\A\[KP_Left] "
    IN "\C\A\[KP_Begin] \C\A\[KP_Right] \C\A\[KP_Home] \C\A\[KP_Up] \C\A\[KP_Page_Up] "
    IN "\C\A\[KP_Enter]    \D2"

    SNAP keys_06 fb590f65c41ca5cf5b49bf17d366059b

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e>\"\r"
}

function ALT_SENDS_ESC {
    IN "printf \"\\\\e[>4;0m\\\\e[?1036h\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing Alt-sends-Esc in normal mode. \D1"
    IN "a A \Aa \AA \C\Aa \C\AA 2 \A2 \C\A2 # \A# \C\A# "
    IN "/ ? \A/ \A? \C\A/ \C\A? "
    IN "\A0 \A1 \A2 \A3 \A4 \A5 \A6 \A7 \A8 \A9 "
    IN "\C\A2 \C\A3 \C\A5 \C\A6 \C\A7 "

    SNAP keys_07 fa78d452ab0fde532cc24ffcd8bbbf7f

    IN "\Cd\Cd\D3"
    IN "stty -echo && printf \"\\\\e[H\\\\e[J\\\\e[12l\" && cat -vT\r"

    IN "Testing Alt-sends-Esc with local echo on. \D1"
    IN "a A \Aa \AA \C\Aa \C\AA 2 \A2 \C\A2 # \A# \C\A# "
    IN "/ ? \A/ \A? \C\A/ \C\A? "
    IN "\A0 \A1 \A2 \A3 \A4 \A5 \A6 \A7 \A8 \A9 "
    IN "\C\A2 \C\A3 \C\A5 \C\A6 \C\A7 "

    SNAP keys_08 67051d652516e3a0e2198b9e28219c25

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[12h\" && stty echo\r"

    IN "printf \"\\\\e[?1036l\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing some keys with Alt-sends-Esc disabled. \D1"
    IN "\Aa \Ab \Ac \Ad \Ae \Af \Ag \Ah \Ai \Aj \Ak \Al \Am "
    IN "\An \Ao \Ap \Aq \Ar \As \At \Au \Av \Aw \Ax \Ay \Az "
    IN "\AA \AB \AC \AD \AE \AF \AG \AH \AI \AJ \AK \AL \AM "
    IN "\AN \AO \AP \AQ \AR \AS \AT \AU \AV \AW \AX \AY \AZ "
    IN "\A1 \A2 \A3 \A4 \A5 \A6 \A7 \A8 \A9 \A0 \A- \A= \A/ "
    IN "\A! \A@ \A# \A$ \A% \A^ \A& \A* \A( \A) \A_ \A+ \A? "
    IN "\A\' \A, \A. \A\" \A< \A> \A; \A\` \A\\\\ \A: \A~ \A| "
    IN "\A[ \A] \A{ \A} "
    IN "\C\Aa \C\AA \C\A0 \C\A1 \C\A2 \C\A9 \A# \C\A# "
    IN "\C\A/ \C\A? "

    SNAP keys_09 0b532193433fb81966582cc255fa18a8

    IN "\Cd\Cd\D3"
    IN "printf \"\\\\e[?1036h\"\r"
}

function COMPOSE_CHARS {

    setxkbmap us -option compose

    IN "printf \"\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing composed characters:\D1"
    IN " a"
    IN " \[Multi_key]\'a"
    IN " \[Multi_key]\"a"
    IN " \[Multi_key]\`a"
    IN " \[Multi_key]^a"
    IN " \[Multi_key]~a"
    IN " \[Multi_key]oa"
    IN " A"
    IN " \[Multi_key]\'A"
    IN " \[Multi_key]\"A"
    IN " \[Multi_key]\`A"
    IN " \[Multi_key]^A"
    IN " \[Multi_key]~A"
    IN " \[Multi_key]oA"
    IN " c"
    IN " \[Multi_key],c"
    IN " \[Multi_key]^c"
    IN " \[Multi_key]oc"
    IN " C"
    IN " \[Multi_key],C"
    IN " \[Multi_key]^C"
    IN " e"
    IN " \[Multi_key]\'e"
    IN " \[Multi_key]\"e"
    IN " \[Multi_key]\`e"
    IN " \[Multi_key]^e"
    IN " \[Multi_key]~e"
    IN " \[Multi_key]oe"
    IN " \[Multi_key]=e"
    IN " E"
    IN " \[Multi_key]\'E"
    IN " \[Multi_key]\"E"
    IN " \[Multi_key]\`E"
    IN " \[Multi_key]^E"
    IN " i"
    IN " \[Multi_key]\'i"
    IN " \[Multi_key]\"i"
    IN " \[Multi_key]\`i"
    IN " \[Multi_key].i"
    IN " \[Multi_key]~i"
    IN " \[Multi_key]^i"
    IN " I"
    IN " \[Multi_key]\'I"
    IN " \[Multi_key]\"I"
    IN " \[Multi_key]\`I"
    IN " \[Multi_key].I"
    IN " \[Multi_key]~I"
    IN " \[Multi_key]^I"
    IN " o"
    IN " \[Multi_key]\'o"
    IN " \[Multi_key]\"o"
    IN " \[Multi_key]\`o"
    IN " \[Multi_key]^o"
    IN " \[Multi_key]~o"
    IN " \[Multi_key]=o"
    IN " O"
    IN " \[Multi_key]\'O"
    IN " \[Multi_key]\"O"
    IN " \[Multi_key]\`O"
    IN " \[Multi_key]^O"
    IN " \[Multi_key]~O"
    IN " \[Multi_key]=O"
    IN " u"
    IN " \[Multi_key]\'u"
    IN " \[Multi_key]\"u"
    IN " \[Multi_key]\`u"
    IN " \[Multi_key]^u"
    IN " \[Multi_key]~u"
    IN " \[Multi_key]=u"
    IN " U"
    IN " \[Multi_key]\'U"
    IN " \[Multi_key]\"U"
    IN " \[Multi_key]\`U"
    IN " \[Multi_key]^U"
    IN " \[Multi_key]~U"
    IN " \[Multi_key]=U"
    IN " y"
    IN " \[Multi_key]\"y"
    IN " \[Multi_key].y"
    IN " Y"
    IN " \[Multi_key]\"Y"
    IN " \[Multi_key].Y"
    IN " \[Multi_key]=Y"
    IN " \[Multi_key]or"
    IN " \[Multi_key]!!"
    IN " \[Multi_key]??"

    SNAP keys_10 60554ad0c7ff32768f1536016cafde4d

    IN "\Cd\Cd\D3"
}

function SPECIAL_KEYS {

    IN "printf \"\\\\e[>4;0m\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing special key combinations; modifyOtherKeys=0. \D1"

    IN "Shift-a: A \D2"
    IN "Shift-Space \S  \D2"
    IN "Shift-Tab: \[ISO_Left_Tab] \D2"
    IN "Ctrl-Return: \C\[Return] \D2"
    IN "Ctrl-Space: \C  \D2"
    IN "Ctrl-Alt-Space: \C\A  \D2"
    IN "Ctrl-Shift-Space: \C\S  \D2"
    IN "Ctrl-Tab: \C\[Tab] \D2"
    IN "Ctrl-0: \C\[0] \D2"
    IN "Ctrl-1: \C\[1] \D2"
    IN "Ctrl-2: \C\[2] \D2"
    IN "Ctrl-3: \C\[3] \D2"
    IN "Ctrl-5: \C\[5] \D2"
    IN "Ctrl-6: \C\[6] \D2"
    IN "Ctrl-7: \C\[7] \D2"
    IN "Ctrl-8: .\C\[8] \D2"
    IN "Ctrl-9: \C\[9] \D2"
    IN "Ctrl-!: \C! \D2"
    IN "Ctrl-@: \C@ \D2"
    IN "Ctrl-#: \C# \D2"
    IN "Ctrl-$: \C$ \D2"
    IN "Ctrl-%: \C% \D2"
    IN "Ctrl-^: \C^ \D2"
    IN "Ctrl-&: \C& \D2"
    IN "Ctrl-*: \C* \D2"
    IN "Ctrl-(: \C( \D2"
    IN "Ctrl-): \C) \D2"
    IN "Ctrl-[: \C[ \D2"
    IN "Ctrl-]: \C] \D2"
    IN "Ctrl-{: \C{ \D2"
    IN "Ctrl-}: \C} \D2"
    IN "Ctrl-/: \C/ \D2"
    IN "Ctrl-i: \Ci \D2"
    IN "Ctrl-Shift-i: \CI \D2"
    IN "Ctrl-x: \Cx \D2"
    IN "Ctrl-Alt-x: \C\Ax \D2"
    IN "Ctrl-;: \C; \D2"
    IN "Ctrl-:: \C: \D2"
    IN "Ctrl-~: \C~ \D2"
    IN "Alt-Space: \A  \D2"
    IN "Alt-Tab: \A\[Tab] \D2"
    IN "Alt-x: \Ax \D2"

    SNAP keys_11 a415b0df0b027d5bb9b66fe6e6902574

    IN "\Cd\Cd\D3"

    IN "printf \"\\\\e[>4;1m\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "Testing special key combinations; modifyOtherKeys=1. \D1"

    IN "Shift-a: A \D2"
    IN "Shift-Space \S  \D2"
    IN "Shift-Tab: \[ISO_Left_Tab] \D2"
    IN "Ctrl-Return: \C\[Return] \D2"
    IN "Ctrl-Shift-Return: \C\S\[Return] \D2"
    IN "Ctrl-Alt-Return: \C\A\[Return] \D2"
    IN "Ctrl-Space: \C  \D2"
    IN "Ctrl-Alt-Space: \C\A  \D2"
    IN "Ctrl-Shift-Space: \C\S  \D2"
    IN "Ctrl-Tab: \C\[Tab] \D2"
    IN "Ctrl-0: \C\[0] \D2"
    IN "Ctrl-1: \C\[1] \D2"
    IN "Ctrl-2: \C\[2] \D2"
    IN "Ctrl-3: \C\[3] \D2"
    IN "Ctrl-5: \C\[5] \D2"
    IN "Ctrl-6: \C\[6] \D2"
    IN "Ctrl-7: \C\[7] \D2"
    IN "Ctrl-8: .\C\[8] \D2"
    IN "Ctrl-9: \C\[9] \D2"
    IN "Ctrl-!: \C! \D2"
    IN "Ctrl-@: \C@ \D2"
    IN "Ctrl-#: \C# \D2"
    IN "Ctrl-$: \C$ \D2"
    IN "Ctrl-%: \C% \D2"
    IN "Ctrl-^: \C^ \D2"
    IN "Ctrl-&: \C& \D2"
    IN "Ctrl-*: \C* \D2"
    IN "Ctrl-(: \C( \D2"
    IN "Ctrl-): \C) \D2"
    IN "Ctrl-[: \C[ \D2"
    IN "Ctrl-]: \C] \D2"
    IN "Ctrl-{: \C{ \D2"
    IN "Ctrl-}: \C} \D2"
    IN "Ctrl-/: \C/ \D2"
    IN "Ctrl-i: \Ci \D2"
    IN "Ctrl-Shift-i: \CI \D2"
    IN "Ctrl-x: \Cx \D2"
    IN "Ctrl-Alt-x: \C\Ax \D2"
    IN "Ctrl-;: \C; \D2"
    IN "Ctrl-:: \C: \D2"
    IN "Ctrl-~: \C~ \D2"
    IN "Alt-Return: \A\[Return] \D2"
    IN "Alt-Space: \A  \D2"
    IN "Alt-Tab: \A\[Tab] \D2"
    IN "Alt-x: \Ax \D2"

    SNAP keys_12 dc0833c6fcfc7a5f6668597158502345

    IN "\Cd\Cd\D3"

    IN "printf \"\\\\e[>4;2m\\\\e[H\\\\e[J\" && cat -vT\r"

    IN "testing special key combinations - modify-other-keys is 2  \D1"

    IN "shift-a A \D2"
    IN "shift-space \S  \D2"
    IN "shift-tab \[ISO_Left_Tab] \D2"
    IN "ctrl-return \C\[Return] \D2"
    IN "ctrl-shift-return \C\S\[Return] \D2"
    IN "ctrl-alt-return \C\A\[Return] \D2"
    IN "ctrl-space \C  \D2"
    IN "ctrl-alt-space \C\A  \D2"
    IN "ctrl-shift-space \C\S  \D2"
    IN "ctrl-tab \C\[Tab] \D2"
    IN "ctrl-0 \C\[0] \D2"
    IN "ctrl-1 \C\[1] \D2"
    IN "ctrl-2 \C\[2] \D2"
    IN "ctrl-3 \C\[3] \D2"
    IN "ctrl-4 \C\[4] \D2"
    IN "ctrl-5 \C\[5] \D2"
    IN "ctrl-6 \C\[6] \D2"
    IN "ctrl-7 \C\[7] \D2"
    IN "ctrl-8 \C\[8] \D2"
    IN "ctrl-9 \C\[9] \D2"
    IN "ctrl-! \C! \D2"
    IN "ctrl-@ \C@ \D2"
    IN "ctrl-# \C# \D2"
    IN "ctrl-$ \C$ \D2"
    IN "ctrl-% \C% \D2"
    IN "ctrl-^ \C^ \D2"
    IN "ctrl-& \C& \D2"
    IN "ctrl-* \C* \D2"
    IN "ctrl-( \C( \D2"
    IN "ctrl-) \C) \D2"
    IN "ctrl-[ \C[ \D2"
    IN "ctrl-] \C] \D2"
    IN "ctrl-{ \C{ \D2"
    IN "ctrl-} \C} \D2"
    IN "ctrl-/ \C/ \D2"
    IN "ctrl-i \Ci \D2"
    IN "ctrl-shift-i \CI \D2"
    IN "ctrl-x \Cx \D2"
    IN "ctrl-alt-x \C\Ax \D2"
    IN "ctrl-; \C; \D2"
    IN "ctrl-: \C: \D2"
    IN "ctrl-\\\\ \C\\\\ \D2"
    IN "ctrl-| \C| \D2"
    IN "ctrl-~ \C~ \D2"
    IN "alt-return \A\[Return] \D2"
    IN "alt-space \A  \D2"
    IN "alt-tab \A\[Tab] \D2"
    IN "alt-x \Ax \D2"

    SNAP keys_13 0a9dd49f6b5754fda882458c3a5be730

    IN "\Cd\Cd\D3"
}

INS_DEL_PGUPDN
FUNCTION_KEYS
CURSOR_KEYS
KEYPAD_KEYS
ALT_SENDS_ESC
COMPOSE_CHARS
SPECIAL_KEYS
