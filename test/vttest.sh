#!/bin/bash

cd $(dirname $0)
source testbase.sh

function VT_1 {
    IN "1\r"
    SNAP vt_01_01 7d5405859f5e3c7e4048c4d91630a120
    IN "\r"
    SNAP vt_01_02 ca50f175beda3d3dea0af919057abaac
    IN "\r"
    SNAP vt_01_03 28f96b8b64bfb9e853b86750600eff71
    IN "\r"
    SNAP vt_01_04 28f96b8b64bfb9e853b86750600eff71
    IN "\r"
    SNAP vt_01_05 a68bd220520d221cece4a1b4fd4daa39
    IN "\r"
    SNAP vt_01_06 c34d6834edfbfdf0b4ecacafa67851f4
    IN "\r"
}

function VT_2 {
    IN "2\r"
    SNAP vt_02_01 f67230812cec96d6c6a094bf31bbb605
    IN "\r"
    SNAP vt_02_02 0cc2d61757a68e3e02239ad26bd6a5be
    IN "\r"
    SNAP vt_02_03 b41388b16347906c8c3e180fedae8267
    IN "\r"
    SNAP vt_02_04 adc39b4bd1ccb344bfacbee4fa593219
    IN "\r"
    SNAP vt_02_05 7de79955c357a4f60efb47a8cfcdc72c
    IN "\r"
    SNAP vt_02_06 e0ea9ef0cbbe1718b5132e7f68a8b908
    IN "\r"
    SNAP vt_02_07 65d594ae25e3699e3cb213bffd9af8ca
    IN "\r"
    SNAP vt_02_08 034efb50f304aec92a03d942084c985b
    IN "\r"
    SNAP vt_02_09 4de672462329fb5261d2da03447305c2
    IN "\r"
    SNAP vt_02_10 c2777f595192357f7e30d28b2e2d1e62
    IN "\r"
    SNAP vt_02_11 22e12fed30a9c919ba1e6b8c0b747df8
    IN "\r"
    SNAP vt_02_12 dac4ea726c8a77803d2729ab8743ad7a
    IN "\r"
    SNAP vt_02_13 c29503978a49775901a79e0c3d55c814
    IN "\r"
    SNAP vt_02_14 350475943a0c7d2ca478d1a00fe2a56b
    IN "\r"
    SNAP vt_02_15 bf2193f7e0b7e8281b45cc67457e5ace
    IN "\r"
}

function VT_3_VT220 {
    IN "3\r"
    IN "8\r" # Test VT100 Character Sets
    SNAP vt_03_01 66c2029c0a00dfded0fc906c80080b15
    IN "\r"
    IN "9\r" # Test Shift In/Shift Out (SI/SO)
    SNAP vt_03_02 83414f4055237b7c902bfe52bfc4c239
    IN "\r"
    IN "7\r" # Specify G3
    IN "2\r" # -> DEC Special Graphics and Line Drawing
    IN "10\r" # Test VT220 Locking Shifts
    SNAP vt_03_03 5b5791a7fa5befcfac152f345e400548
    IN "\r"
    IN "11\r" # Test VT220 Single Shifts
    SNAP vt_03_04 2c1866cbe7c54aa2530b62d766c93d1e
    IN "\r"
    SNAP vt_03_05 ca17bcc12a61dd1d39b85da9613f4755
    IN "\r"
    IN "6\r" # Specify G2
    IN "4\r" # -> DEC Supplemental Graphic
    IN "7\r" # Specify G3
    IN "5\r" # -> DEC Technical
    IN "10\r" # Test VT220 Locking Shifts
    SNAP vt_03_06 0d13ce0a8dae591836cbdd62b7f8a871
    IN "\r"
    IN "11\r" # Test VT220 Single Shifts
    SNAP vt_03_07 141ca65fcf6652d191eeb0ca495f7622
    IN "\r"
    SNAP vt_03_08 7bbf27873c5b2cfc3387a8b189f329fd
    IN "\r"
    IN "0\r"
}

function VT_3_VT100 {
    IN "3\r"
    SNAP vt_03_01
    IN "\r"
}

function VT_3 {
    if [ ! -z ${SUPPORTS_VT220} ] ; then
        VT_3_VT220
    else
        VT_3_VT100
    fi
}

function VT_5 {
    IN "5\r"
    IN "3\r" # Keyboard Layout
    if [ -z ${SUPPORTS_VT220} ] ; then
        IN "0\r" # Standard American ANSI layout
    fi
    IN " \t\{BackSpace}"
    IN "\`1234567890-=[];',./\\\\"
    IN "~!@#$%^&*()_+{}:\"<>?|"
    IN "abcdefghijklmnopqrstuvwxyz"
    IN "ABCDEFGHIJKLMNOPQRSTUVWXYZ\r"
    SNAP vt_05_01 5a440969e504e0eb973250b46bcc1faa
    IN "\r"
    IN "4\r" # Cursor Keys
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_02 e957806fd23c055a18fadf0ce5cd615d
    IN "\t\D1"
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_03 2c3cef495d2185c9d6187cf8c63637fe
    IN "\t\D1"
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_04 efc54badc0daf8730e26e15272a4ecde
    IN "\t\D1"
    IN "\r"

    IN "5\r" # Numeric Keypad
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_05 314fbc833efac9c7910e65820b395bfc
    IN "\t\D1"
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_06 c3952de0a8b7fe42e70eacce9eda7d8e
    IN "\t\D1"
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_07 2b64d110de25d733106835d50cf1a4fb
    IN "\t\D1"
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_08 9a636fd488ee7ae96ebb88d34042c71f
    IN "\t\D1"
    IN "\r"

    if [ ! -z ${SUPPORTS_VT220} ] ; then
        IN "6\r" # Editing Keypad
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_09 21d4c41d6206706b6d860b201ee50c44
        IN "\t\D1"
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_10 f3ca89d8a3b56975a8625ff78168121d
        IN "\t\D1"
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_11 9126f222d2035d8e7eb3880e603f0b44
        IN "\r"

        IN "7\r" # Function Keys
        IN "\{F1}\D1\{F2}\D1\{F3}\D1\{F4}\D1\{F5}\D1\{F6}\D1\{F7}\D1\{F8}"
        IN "\{F9}\D1\{F10}\D1\{F11}\D1\{F12}\D1\{F13}\D1\{F14}"
        SNAP vt_05_12 29ab475286f9d341d6630d45e4f8fa3b
        IN "\t\D1"
        IN "\{F1}\D1\{F2}\D1\{F3}\D1\{F4}\D1\{F5}\D1\{F6}\D1\{F7}\D1\{F8}"
        IN "\{F9}\D1\{F10}\D1\{F11}\D1\{F12}\D1\{F13}\D1\{F14}"
        SNAP vt_05_13 c7f20649a0572537a1b852e6e25b1d85
        IN "\t\D1"
        IN "\r"
    fi

    IN "9\r" # Control Keys
    IN "\C@\C@\Ca\Ca\Cb\Cb\Cc\Cc\Cd\Cd\Ce\Ce\Cf\Cf\Cg\Cg"
    IN "\Ch\Ch\Ci\Ci\Cj\Cj\Ck\Ck\Cl\Cl\Cm\Cm\Cn\Cn\Co\Co"
    IN "\Cp\Cp\Cq\Cq\Cr\Cr\Cs\Cs\Ct\Ct\Cu\Cu\Cv\Cv\Cw\Cw"
    IN "\Cx\Cx\Cy\Cy\Cz\Cz\C[\C[\C_\C_\C]\C]\C^\C^\C\\\\\C\\\\"
    IN "\b"
    SNAP vt_05_14 5953ac54f1579f61d6da7e31019850ac
    IN "\r"
    IN "0\r"
 }

function VT_6 {
    IN "6\r"
    IN "1\r" # <ENQ> (AnswerBack Message)
    if [ ! -z ${MISSING_ANSWERBACK} ] ; then
        IN "\r"
    fi
    SNAP vt_06_01 319f8b46434cf041f48819205076829e
    IN "\r"
    IN "2\r" # Set/Reset Mode - LineFeed / Newline
    IN "\r\D5\r"
    SNAP vt_06_02 449cfacdd97d791a87da684da2dcd192
    IN "\r"
    IN "3\r\D5" # Device Status Report (DSR)
    if [ ! -z ${MISSING_DSR} ] ; then
        IN "\r"
    fi
    SNAP vt_06_03 9f6f30cadabaf096ff7e8cef3e821f45
    IN "\r"
    IN "4\r" # Primary DA
    SNAP vt_06_04 a4283f43fc4d3c401867b84f122f9ef2
    IN "\r"
    IN "5\r" # Secondary DA
    if [ ! -z ${MISSING_SECONDARY_DA} ] ; then
        IN "\r"
    fi
    SNAP vt_06_05 959ad729cd450f388a6f867e831a61cf
    IN "\r"
    IN "0\r"
}

function VT_8 {
    IN "8\r"
    SNAP vt_08_01 302124cb5528a8f2a021f3ee4a1161e1
    IN "\r"
    SNAP vt_08_02 c4ff5875448e0e8caaabfdd030aae722
    IN "\r"
    SNAP vt_08_03 79b4492b8aa6ee36e59c6b8e8ea29da4
    IN "\r"
    SNAP vt_08_04 7c0ec72c5278e80d764ab22baeeba906
    IN "\r"
    SNAP vt_08_05 eea427271cc01316395dd632f0dd4d65
    IN "\r"
    SNAP vt_08_06 9903233c8869716c2e27d0abd5ef41e3
    IN "\r"
    SNAP vt_08_07 58ac2efce64dda51c9802978f5363303
    IN "\r"
    SNAP vt_08_08 c326482b9f5bcb5d362a2ec2d5d85e8c
    IN "\r"
    SNAP vt_08_09 7ca23580451a3952d57e4b32aaf16c69
    IN "\r"
    SNAP vt_08_10 b36e41d063b8680cc73374324c70d82f
    IN "\r"
    SNAP vt_08_11 7fcfd2e3ec7b532d77dfbca299788345
    IN "\r"
    SNAP vt_08_12 2eb2ee193a419e1f6c0b66fdad8e50e0
    IN "\r"
    SNAP vt_08_13 3bf4dc7177433ef3b25e952f19aa69c9
    IN "\r"
    SNAP vt_08_14 58ac2efce64dda51c9802978f5363303
    IN "\r"
}

function VT_9 {
    IN "9\r"
    IN "1\r"
    IN "\r"
    IN "\r\D1\r\D1\r\D1\r\D1\r\D1\r\D1\r\D1\r\D1\r\D1\r"
    SNAP vt_09_01 cbbbaec8fb7b5a4f4e351df3d7e0d0db
    IN "\r"

    IN "2\r"
    IN "\r"
    SNAP vt_09_02 df1928ef76fad9fffe5a70793c6f5587
    IN "\r"

    IN "3\r"
    SNAP vt_09_03 82a24d23d32ccf3c97f6ca12d4e72a4f
    IN "\r"

    IN "4\r"
    SNAP vt_09_04 d7be860bde3096ffa79ef74bf86d5adc
    IN "0\r"
    IN "\r"

    IN "5\r"
    SNAP vt_09_05 2ce789395af8d33a01a444e909bff6aa
    IN "\r"

    IN "6\r"
    SNAP vt_09_06 b1b0cf9a3a271b973f45bef0c3d1e131
    IN "\r"
    SNAP vt_09_07 b381c349d9fa01d2936f706f201cc610
    IN "\r"

    IN "7\r"
    SNAP vt_09_08 51bf13b813e87d4155662e06ecc0de08
    IN "\r"

    IN "8\r"
    SNAP vt_09_09 9fca11a9039826c24b72299ccdc3d180
    IN "\r"
    SNAP vt_09_10 457d330e227aa7f60d311649e25f22fe
    IN "\r"
    SNAP vt_09_11 ca52af254a085661e0ef216425fabd6e
    IN "\r"
    SNAP vt_09_12 3fd7f67d53167870ff1863c3d1292cf1
    IN "\r"
    SNAP vt_09_13 9c2339a8e5c633ab9ae95c95996bbf1b
    IN "\r"
    SNAP vt_09_14 f59018efc0091842eb6637df701107c1
    IN "\r"

    IN "9\r"
    SNAP vt_09_15 8e3e5228b695e039b4cbafb63e2e8d6e
    IN "\r"
    SNAP vt_09_16 8e3e5228b695e039b4cbafb63e2e8d6e
    IN "\r"

    IN "0\r"
}

function VT_10 {
    IN "10\r"
    IN "1\r"
    IN "\r"
    sleep 5
    SNAP vt_10_01 91efa515b74939913ba5729bc22ae468
    IN "\r"

    IN "3\r"
    IN "\r"
    SNAP vt_10_02 8843a44eb8e00983582732da2ac1fdd6
    IN "\r"

    IN "0\r"
}

function VT_11 {
    IN "11\r"

    IN "1\r" # VT220 Tests
    IN "2\r" # VT220 Screen-Display Tests
    IN "1\r" # Test Send/Receive Mode
    IN "Subidubidoo\D1Subidubidoo\D1"
    SNAP vt_11_1_2_1 1a9a69e29be97d820182522ece721a13
    IN "\r"
    IN "2\r" # Test Visible/Invisible Cursor
    SNAP vt_11_1_2_2_01 15a8b8f28a10ff3091690bd083f78ceb
    IN "\r"
    SNAP vt_11_1_2_2_02 d29c49e9be5012964ba486b66f196fe8
    IN "\r"
    IN "3\r" # Test Erase Char (ECH)
    SNAP vt_11_1_2_3 c823f2c76af91d992dccb9d3bd6448c8
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "2\r" # VT320 Tests
    IN "2\r" # Test cursor-movement
    IN "1\r" # Test Pan Down (SU)
    SNAP vt_11_2_2_1 62764215a24fe1f90e81c9678d79735a
    IN "\r"
    IN "2\r" # Test Pan Up (SD)
    SNAP vt_11_2_2_2 f23ba052fae3349bb1be427194674827
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "3\r" # VT420 Tests
    IN "2\r" # Test cursor-movement
    IN "6\r" # Color test-regions
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_3_2_7_01 c73df47f269b196208f120f6b9050e98
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_3_2_8_01 b6b573c3c52c7eba0894599dbb8b0518
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_3_2_9_01 5608bc562810942583d18aef90460c7c
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_3_2_10_01 b4e7474e6b38b502a1892de11671907a
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to top half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_3_2_7_02 1f5aea6b05413e37b5d91f553fdbfb3c
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_3_2_8_02 a995ec9bcbb38e81cfd23db4140064a3
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_3_2_9_02 176271c0268dc1d144ed68e649488dcd
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_3_2_10_02 b0ff797bbb49a6b199279303ac3be783
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to bottom half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_3_2_7_03 82f14ccd50d2fa0bf9af3d4803b14955
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_3_2_8_03 48d4ab0e4d9fd186dd01c0e8f6a2711d
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_3_2_9_03 36c1e1e6c87399b5df587163640dba47
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_3_2_10_03 01aa82172bf7658f632a02277899bf78
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to middle half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_3_2_7_04 d67bed18602417f1a12f37542e666210
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_3_2_8_04 b9a5fcccd079411375fe1b953ff3cbb0
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_3_2_9_04 8da318aa9008052775859caa426c8574
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_3_2_10_04 709198649248b78ce16e77560f8ef7ae
    IN "\r"
    IN "0\r"

    IN "3\r" # VT420 Editing Sequences
    IN "5\r" # Color test-regions
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_3_3_10_01a a51f160f6157be9549b7a2b3d383ffa7
    IN "\r\r"
    SNAP vt_11_3_3_10_01b 923945b9833bc38700bf84cf6a8fb8dc
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_3_3_11_01a 923945b9833bc38700bf84cf6a8fb8dc
    IN "\r\r"
    SNAP vt_11_3_3_11_01b a51f160f6157be9549b7a2b3d383ffa7
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_3_3_12_01a 84db4a39b09ef3b22aedf07ffa3b604b
    IN "\r"
    SNAP vt_11_3_3_12_01b b5d951243eb3d5ca83cd855bcae150f5
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_3_3_13_01 ad7fc21351dfe257154578e42d37b1f3
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to top half of screen
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_3_3_10_02a 93419dff4f7104eb2708fa2065de5690
    IN "\r\r"
    SNAP vt_11_3_3_10_02b 9b983697440687ba09ed9aea2143ac89
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_3_3_11_02a 20d6d5f3f7d97ad101893770b4038ae7
    IN "\r\r"
    SNAP vt_11_3_3_11_02b 278f30bcb78754ee8437e13596637e38
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    echo "N.B.: Same bug in Xterm!"
    SNAP vt_11_3_3_12_02a 7992fc76ed99837bee083070a9a812ee
    IN "\r"
    echo "N.B.: Same bug in XTerm!"
    SNAP vt_11_3_3_12_02b 5114e8b2c4911412edafc3477aa24b70
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_3_3_13_02 c33aec43990b0a4021648259f659deaf
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to bottom half of screen
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_3_3_10_03a 75d7b3c2dcdc4d1b5a0036a5fdce573e
    IN "\r\r"
    SNAP vt_11_3_3_10_03b 4cdce08a6117bb0ad5a10df4da8530de
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_3_3_11_03a 53fc3c83787c24fe0867ba5717eed6d3
    IN "\r\r"
    SNAP vt_11_3_3_11_03b 498f4234b7f7d4aff1ff244d1370aa3a
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_3_3_12_03a cca8fd61608865e1edead1420cccb894
    IN "\r"
    SNAP vt_11_3_3_12_03b e53798c199bc8c4f0f140934972b90ca
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_3_3_13_03 3d99e0a735ec7b24382e9b8d811479ea
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to middle half of screen
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_3_3_10_04a e89fde9af7a3d9497b9df5547caddba5
    IN "\r\r"
    SNAP vt_11_3_3_10_04b 4684f6ebc60a3c8fc8dce622cc5fa0d3
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_3_3_11_04a d6dd3caba4eff9130dbfc3ab27a04a58
    IN "\r\r"
    SNAP vt_11_3_3_11_04b d3f1504e407000a0d487bce9232dfe32
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    echo "N.B.: Same bug in Xterm!"
    SNAP vt_11_3_3_12_04a ce043bd68d2b96101d2ed61af275f39a
    IN "\r"
    echo "N.B.: Same bug in Xterm!"
    SNAP vt_11_3_3_12_04b 9c65d98b94b345613730e41b399fe87f
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_3_3_13_04 6926417da2a660a1f9243c709d41f268
    IN "\r"
    IN "0\r"

    IN "4\r" # VT420 Keyboard-Control Tests
    IN "1\r" # Test Backarrow Key (DECBKM)
    IN "\b\D3\b"
    SNAP vt_11_3_4_1 716027ee9e70d37505ddc82460dcdd1c
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "4\r" # VT520 Tests
    IN "2\r" # VT520 cursor-movement
    IN "6\r" # Color test-regions
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_4_2_7_01 f8d112a4b8e5b323341a1e86e10f6ded
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_4_2_8_01 9c5747ba41d02335eab245717101aabc
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_4_2_9_01 054e3b2c1f74e66d75cf2efa83d85927
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_4_2_10_01 05f365f663773ca29db9cc2fa8af130f
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_4_2_11_01 12d9217894f587c73f5c2c699244c8ee
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_4_2_12_01 9f81b832ebb15f6ae26a4e2ebca7254d
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_4_2_13_01 7aa0d70e2b4e7678fb234c8c419169db
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_4_2_14_01 c3dadd63adee7028ea9defe1ea73198a
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_4_2_15_01 09fb397a4d5f89351ea9c520ffc0907f
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to top half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_4_2_7_02 12fd48c0bdbdf660b162822447c1c855
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_4_2_8_02 8a3cf937d076f123ba9a51ae7adc2194
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_4_2_9_02 0fb535bd9c272ecc4743cae6805cd2db
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_4_2_10_02 95d9c1e68ee9e167bb85be907d14c05c
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_4_2_11_02 bb18c0bb03559fdafeeb0ceb979dcc11
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_4_2_12_02 764857cde6ec7769a401f8226b4b1c54
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_4_2_13_02 48f56c298f45fb8379a9c6b9e9d0e49c
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_4_2_14_02 c34f23ef8074af0820361381700b6a93
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_4_2_15_02 829b988147b2b1d3e11fe73a9064c5c3
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to bottom half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_4_2_7_03 5d916d32819e9e76f4cf4803d2ffa018
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_4_2_8_03 761fcdb51653ec885984448ea5e1763a
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_4_2_9_03 3b40ecfecd7e1e3ef829cb47e232fb5e
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_4_2_10_03 79c7c6e6e91eedbba99ac26070d14fe5
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_4_2_11_03 8f647267f7b5b072019d7587b875ff76
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_4_2_12_03 738083af59a22bcff0a6db3ed0e030f1
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_4_2_13_03 f603244cee5d7c87e5ea1f3540066122
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_4_2_14_03 3c44a061629fc40f48f21623c30a81f7
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_4_2_15_03 ae54e95fe7647920ea7d7545078788c2
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to middle half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_4_2_7_04 76bac3175ffe96834bb366502f4077db
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_4_2_8_04 21bde13d638ac58664e7d7f902165c85
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_4_2_9_04 b4d276df63b6a8cf6a1edf16504dd3a7
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_4_2_10_04 d41000aa84285f4ea08308a35a5ed973
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_4_2_11_04 3bed021e02e7ffa4830839acd1c4d0fd
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_4_2_12_04 c0227f3aea2ca1d9317e58e980dcbdb6
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_4_2_13_04 c41886ebaffb2593805baf85cfd13882
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_4_2_14_04 6528a29ec10de921d43aa5e0efc0553c
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_4_2_15_04 5329ca8525fb86006a1b376a68fe81da
    IN "\r"
    IN "0\r"
    IN "0\r"

    # At this point we restart vttest, to avoid bumping into
    # a bug that produces a flawed input sequence for vt_11_6_4
    # resulting in wrong output (regardless of terminal emulator).
    echo "Restarting vttest to work-around bug in 11.6.4 ..."
    IN "0\r"
    IN "0\r"
    IN "vttest\r"
    IN "11\r"

    IN "6\r" # ISO 6429 colors
    IN "2\r" # Display color test-pattern
    SNAP vt_11_6_2 85594a0e6c9227455e8854005befbcde
    IN "\r"
    IN "3\r" # Test SGR-0 color reset
    SNAP vt_11_6_3 5a99f807644c8cc00b0486aed5e870b1
    IN "\r"
    IN "4\r" # Test BCE-style clear line/display (ED, EL)
    SNAP vt_11_6_4_01 9271b0457a34cfc77c576a6b663d9a5a
    IN "\r"
    SNAP vt_11_6_4_02 ac9e38462210e4728468df4ee7cdf7b8
    IN "\r"
    IN "5\r" # Test BCE-style clear line/display (ECH, Indexing)
    SNAP vt_11_6_5_01 9271b0457a34cfc77c576a6b663d9a5a
    IN "\r"
    SNAP vt_11_6_5_02 ac9e38462210e4728468df4ee7cdf7b8
    IN "\r"
    IN "6\r" # Test VT102-style features with BCE
    IN "1\r" # Test of cursor movements
    SNAP vt_11_6_6_1_01 820ab5f2ea42e8ad18a81ac2162ddb26
    IN "\r"
    SNAP vt_11_6_6_1_02 5d6906505130adce5dc1ad95b33241a0
    IN "\r"
    SNAP vt_11_6_6_1_03 d1ba90fbc13d0ce631248c180311d1e7
    IN "\r"
    SNAP vt_11_6_6_1_04 d1ba90fbc13d0ce631248c180311d1e7
    IN "\r"
    SNAP vt_11_6_6_1_05 f19773669d594168cbd4a64ed9a1aaaf
    IN "\r"
    IN "2\r" # Test of screen features
    SNAP vt_11_6_6_2_01 eeae32bd506c31f532a6bef8f3067cb5
    IN "\r"
    SNAP vt_11_6_6_2_02 b614f08011ff5bccdf747cdbb48601f7
    IN "\r"
    SNAP vt_11_6_6_2_03 3deab298e01d13fda5b580a7836bc450
    IN "\r"
    SNAP vt_11_6_6_2_04 0a430388b71c8adb1399f07c41efcee3
    IN "\r"
    SNAP vt_11_6_6_2_05 af42d657dfcbf2a2f1fbc05b2aeffef4
    IN "\r"
    SNAP vt_11_6_6_2_06 b7726e558be9bbedf7898c610dc5244a
    IN "\r"
    SNAP vt_11_6_6_2_07 c4822669f986e31c5ce0e487058e5edc
    IN "\r"
    SNAP vt_11_6_6_2_08 529d16dbdebae1992b783e0f2b192706
    IN "\r"
    SNAP vt_11_6_6_2_09 968161c8636d7bd86ab9265f7d0c8466
    IN "\r"
    SNAP vt_11_6_6_2_10 255870a3dac49dfcdc0c5e4313a3d67f
    IN "\r"
    SNAP vt_11_6_6_2_11 e2d1e3de019a4d2ba24548d40e5f8e3d
    IN "\r"
    SNAP vt_11_6_6_2_12 2c2736a8c240471ea4a8620c0d8293af
    IN "\r"
    SNAP vt_11_6_6_2_13 f8b963bf967baf3292bc01054fc8e661
    IN "\r"
    SNAP vt_11_6_6_2_14 62905438e5f2c97f6db25bd8c8e0e967
    IN "\r"
    IN "3\r" # Test Insert/Delete Char/Line
    SNAP vt_11_6_6_3_01 b57aeb71dbb3294afa000039b0f82789
    IN "\r"
    SNAP vt_11_6_6_3_02 5cbc1e5d9979aa774f4c8cfd22e13b82
    IN "\r"
    SNAP vt_11_6_6_3_03 2b3c533386119a3b88c8d8481366018e
    IN "\r"
    SNAP vt_11_6_6_3_04 e603578ceab0b9cd66c2be5b57da8716
    IN "\r"
    SNAP vt_11_6_6_3_05 57cee0006447560c738d1b545978a629
    IN "\r"
    SNAP vt_11_6_6_3_06 9deccce96c3f6a15240445aebab02616
    IN "\r"
    SNAP vt_11_6_6_3_07 f98037f2b34dfc3010d4ed1d6a302ed6
    IN "\r"
    SNAP vt_11_6_6_3_08 79e2fdc1500bf05bf5b7a72c66ee4c20
    IN "\r"
    SNAP vt_11_6_6_3_09 7f03f47141ec0308cb885b2c3efbc98b
    IN "\r"
    SNAP vt_11_6_6_3_10 3317f07a3c71ab689aa8438d47eb94c5
    IN "\r"
    SNAP vt_11_6_6_3_11 5ced155983077ee9343a6a7f80a18980
    IN "\r"
    SNAP vt_11_6_6_3_12 9f31c91ebcfa62fb03f650c6752bac65
    IN "\r"
    SNAP vt_11_6_6_3_13 eab2a1759a651b933a4a06a6b174a867
    IN "\r"
    SNAP vt_11_6_6_3_14 c4202fb6c152c9835ede484045644e26
    IN "\r"
    SNAP vt_11_6_6_3_15 748c6bcd8461a4b45a7729aa6dcd5fb7
    IN "\r"
    SNAP vt_11_6_6_3_16 7f03f47141ec0308cb885b2c3efbc98b
    IN "\r"
    IN "0\r"
    IN "7\r" # Miscellaneous ISO-6429 (ECMA-48) Tests
    IN "2\r" # Test Repeat (REP)
    SNAP vt_11_6_7_2 64d7dededa61e00b610ff002f5342b7b
    IN "\r"
    IN "3\r" # Test Scroll-Down (SD)
    SNAP vt_11_6_7_3 862914ff7ca4f3550c093fdd4f8e0c25
    IN "\r"
    IN "4\r" # Test Scroll-Left (SL)
    SNAP vt_11_6_7_4 16956e47a8d7b2bb939454b3b776765a
    IN "\r"
    IN "5\r" # Test Scroll-Right (SR)
    SNAP vt_11_6_7_5 277756b7821371219cc016009d4237ba
    IN "\r"
    IN "6\r" # Test Scroll-Up (SU)
    SNAP vt_11_6_7_6 d8b5d5458b63bc2c53e65ad53748b58a
    IN "\r"
    IN "0\r"
    IN "8\r" # Test screen features with BCE
    SNAP vt_11_6_8_01 c4822669f986e31c5ce0e487058e5edc
    IN "\r"
    SNAP vt_11_6_8_02 529d16dbdebae1992b783e0f2b192706
    IN "\r"
    SNAP vt_11_6_8_03 968161c8636d7bd86ab9265f7d0c8466
    IN "\r"
    SNAP vt_11_6_8_04 255870a3dac49dfcdc0c5e4313a3d67f
    IN "\r"
    SNAP vt_11_6_8_05 131a270641fa06a7c1d2c44a50aae92c
    IN "\r"
    SNAP vt_11_6_8_06 85c78dddb428ae1b84a1106d0092da5e
    IN "\r"
    IN "9\r" # Test screen features with ISO 6429 SGR 22-27 codes
    SNAP vt_11_6_9_01 b3ee924a5a150b7f62664ba5567cbdff
    IN "\r"
    SNAP vt_11_6_9_02 4a5ee5b84e05afcf109fee04058498ec
    IN "\r"
    SNAP vt_11_6_9_03 b3ee924a5a150b7f62664ba5567cbdff
    IN "\r"
    IN "0\r"

    IN "7\r" # Miscellaneous ISO-6429 (ECMA-48) Tests
    IN "2\r" # Test Repeat (REP)
    SNAP vt_11_7_2 b9e4018e9830bce69133fd2c62e46032
    IN "\r"
    IN "3\r" # Test Scroll-Down (SD)
    SNAP vt_11_7_3 e2d11e6b2704eb8e583436675408f33f
    IN "\r"
    IN "4\r" # Test Scroll-Left (SL)
    SNAP vt_11_7_4 0ef10be2a79f548bed58df62e8eac433
    IN "\r"
    IN "5\r" # Test Scroll-Right (SR)
    SNAP vt_11_7_5 ef5780a9c7c9c44c9db0037deb25c30d
    IN "\r"
    IN "6\r" # Test Scroll-Up (SU)
    SNAP vt_11_7_6 25ced4449004f338a3d6dd673eb8954e
    IN "\r"
    IN "0\r"

    IN "8\r" # XTERM special features
    IN "7\r" # Alternate-Screen features
    IN "1\r" # Switch to/from alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_8_7_01 c5b33c89c294737c60206ddac57c59d3
    IN "\r"

    IN "2\r" # Improved alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_8_7_02 b27206a789930441252ea9815d178699
    IN "\r"

    IN "3\r" # Better alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_8_7_03 c76d332c63733d2419d18b83d4672c57
    IN "\r"

    IN "0\r"
    IN "0\r"
    IN "0\r"
}

IN "vttest\r"

VT_1
VT_2
VT_3
VT_5
VT_6
VT_8
VT_9
VT_10
VT_11

IN "0\r"
