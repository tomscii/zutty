#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

CHECK_DEPS vttest setxkbmap
VttestReqVsn="VT100 test program, version 2.7 (20210210)"
if [[ $(vttest -V) != $VttestReqVsn ]]
then
    printf "${RED}Error: vttest version mismatch${DFLT}\n"
    echo "   Found version: $(vttest -V)"
    echo "Required version: $VttestReqVsn"
    printf "${YELLOW}Please run test/deps/install_vttest.sh${DFLT}\n"
    exit 1
fi

LangReq="en_US.UTF-8"
if [[ $LANG != $LangReq ]]
then
    echo "Error: need LANG set to $LangReq, found $LANG"
    exit 1
fi

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
    SNAP vt_03_08 66c2029c0a00dfded0fc906c80080b15
    IN "\r"
    IN "9\r" # Test Shift In/Shift Out (SI/SO)
    SNAP vt_03_09 83414f4055237b7c902bfe52bfc4c239
    IN "\r"
    IN "7\r" # Specify G3
    IN "2\r" # -> DEC Special Graphics and Line Drawing
    IN "10\r" # Test VT220 Locking Shifts
    SNAP vt_03_10_01 5b5791a7fa5befcfac152f345e400548
    IN "\r"
    IN "11\r" # Test VT220 Single Shifts
    SNAP vt_03_11_01 2c1866cbe7c54aa2530b62d766c93d1e
    IN "\r"
    SNAP vt_03_11_02 ca17bcc12a61dd1d39b85da9613f4755
    IN "\r"
    IN "6\r" # Specify G2
    IN "4\r" # -> DEC Supplemental Graphic
    IN "7\r" # Specify G3
    IN "5\r" # -> DEC Technical
    IN "10\r" # Test VT220 Locking Shifts
    SNAP vt_03_10_02 0d13ce0a8dae591836cbdd62b7f8a871
    IN "\r"
    IN "11\r" # Test VT220 Single Shifts
    SNAP vt_03_11_03 141ca65fcf6652d191eeb0ca495f7622
    IN "\r"
    SNAP vt_03_11_04 7bbf27873c5b2cfc3387a8b189f329fd
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
    SNAP vt_05_03 5a440969e504e0eb973250b46bcc1faa
    IN "\r"
    IN "4\r" # Cursor Keys
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_04_01 e957806fd23c055a18fadf0ce5cd615d
    IN "\t\D1"
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_04_02 2c3cef495d2185c9d6187cf8c63637fe
    IN "\t\D1"
    IN "\{Up}\D1\{Down}\D1\{Right}\D1\{Left}\D1"
    SNAP vt_05_04_03 efc54badc0daf8730e26e15272a4ecde
    IN "\t\D1"
    IN "\r"

    IN "5\r" # Numeric Keypad
    # VT100 Numeric mode - send "NumLock on" keysyms
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_05_01 314fbc833efac9c7910e65820b395bfc
    IN "\t\D1"
    # VT100 Application mode - send "NumLock off" keysyms
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_Insert}\D1\{KP_End}\D1\{KP_Down}\D1\{KP_Page_Down}\D1\{KP_Left}"
    IN "\{KP_Begin}\D1\{KP_Right}\D1\{KP_Home}\D1\{KP_Up}\D1\{KP_Page_Up}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Delete}\D1\{KP_Enter}"
    SNAP vt_05_05_02 c3952de0a8b7fe42e70eacce9eda7d8e
    IN "\t\D1"
    # VT52 Numeric mode
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_0}\D1\{KP_1}\D1\{KP_2}\D1\{KP_3}\D1\{KP_4}"
    IN "\{KP_5}\D1\{KP_6}\D1\{KP_7}\D1\{KP_8}\D1\{KP_9}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_05_03 2b64d110de25d733106835d50cf1a4fb
    IN "\t\D1"
    # VT52 Application mode
    IN "\{KP_F1}\D1\{KP_F2}\D1\{KP_F3}\D1\{KP_F4}"
    IN "\{KP_Insert}\D1\{KP_End}\D1\{KP_Down}\D1\{KP_Page_Down}\D1\{KP_Left}"
    IN "\{KP_Begin}\D1\{KP_Right}\D1\{KP_Home}\D1\{KP_Up}\D1\{KP_Page_Up}"
    IN "\{KP_Subtract}\D1\{KP_Separator}\D1\{KP_Decimal}\D1\{KP_Enter}"
    SNAP vt_05_05_04 9a636fd488ee7ae96ebb88d34042c71f
    IN "\t\D1"
    IN "\r"

    if [ ! -z ${SUPPORTS_VT220} ] ; then
        IN "6\r" # Editing Keypad
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_06_01 21d4c41d6206706b6d860b201ee50c44
        IN "\t\D1"
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_06_02 f3ca89d8a3b56975a8625ff78168121d
        IN "\t\D1"
        IN "\{Insert}\D1\{Delete}\D1\{Page_Up}\D1\{Page_Down}"
        SNAP vt_05_06_03 9126f222d2035d8e7eb3880e603f0b44
        IN "\r"

        IN "7\r" # Function Keys
        IN "\{F1}\D1\{F2}\D1\{F3}\D1\{F4}\D1\{F5}\D1\{F6}\D1\{F7}\D1\{F8}"
        IN "\{F9}\D1\{F10}\D1\{F11}\D1\{F12}\D1\{F13}\D1\{F14}"
        SNAP vt_05_07_01 29ab475286f9d341d6630d45e4f8fa3b
        IN "\t\D1"
        IN "\{F1}\D1\{F2}\D1\{F3}\D1\{F4}\D1\{F5}\D1\{F6}\D1\{F7}\D1\{F8}"
        IN "\{F9}\D1\{F10}\D1\{F11}\D1\{F12}\D1\{F13}\D1\{F14}"
        SNAP vt_05_07_02 c7f20649a0572537a1b852e6e25b1d85
        IN "\t\D1"
        IN "\r"
    fi

    IN "9\r" # Control Keys
    IN "\C@\C@\Ca\Ca\Cb\Cb\Cc\Cc\Cd\Cd\Ce\Ce\Cf\Cf\Cg\Cg"
    IN "\Ch\Ch\Ci\Ci\Cj\Cj\Ck\Ck\Cl\Cl\Cm\Cm\Cn\Cn\Co\Co"
    IN "\Cp\Cp\Cq\Cq\Cr\Cr\Cs\Cs\Ct\Ct\Cu\Cu\Cv\Cv\Cw\Cw"
    IN "\Cx\Cx\Cy\Cy\Cz\Cz\C[\C[\C_\C_\C]\C]\C^\C^\C\\\\\C\\\\"
    IN "\b"
    SNAP vt_05_09 5953ac54f1579f61d6da7e31019850ac
    IN "\r"
    IN "0\r"
 }

function VT_6 {
    IN "6\r"
    IN "1\r" # <ENQ> (AnswerBack Message)
    if [ ! -z ${MISSING_ANSWERBACK} ] ; then
        IN "\r"
    fi
    SNAP vt_06_01 c4fa4cd986bdecef7ca99e872b1e8ed6
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

function VT_7 {
    if [ -z ${SUPPORTS_VT52} ] ; then
        return
    fi

    IN "7\r"
    SNAP vt_07_01 d20fcd4b59595d467e3021c711ed1afe
    IN "\r"
    SNAP vt_07_02 abe1febaf84466943dad86bb0b0b98a8
    IN "\r"
    SNAP vt_07_03 5a1757347e8810d51ecfb5755d16b0de
    IN "\r"
    SNAP vt_07_04 d5247dd822b75c2846ada97a789f8c28
    IN "\r"
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
    SNAP vt_09_06_01 b1b0cf9a3a271b973f45bef0c3d1e131
    IN "\r"
    SNAP vt_09_06_02 b381c349d9fa01d2936f706f201cc610
    IN "\r"

    IN "7\r"
    SNAP vt_09_07 51bf13b813e87d4155662e06ecc0de08
    IN "\r"

    IN "8\r"
    SNAP vt_09_08_01 9fca11a9039826c24b72299ccdc3d180
    IN "\r"
    SNAP vt_09_08_02 457d330e227aa7f60d311649e25f22fe
    IN "\r"
    SNAP vt_09_08_03 ca52af254a085661e0ef216425fabd6e
    IN "\r"
    SNAP vt_09_08_04 3fd7f67d53167870ff1863c3d1292cf1
    IN "\r"
    SNAP vt_09_08_05 9c2339a8e5c633ab9ae95c95996bbf1b
    IN "\r"
    SNAP vt_09_08_06 f59018efc0091842eb6637df701107c1
    IN "\r"

    IN "9\r"
    SNAP vt_09_09_01 8e3e5228b695e039b4cbafb63e2e8d6e
    IN "\r"
    SNAP vt_09_09_02 8e3e5228b695e039b4cbafb63e2e8d6e
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
    SNAP vt_10_03 8843a44eb8e00983582732da2ac1fdd6
    IN "\r"

    IN "0\r"
}

function VT_11 {
    IN "11\r"

    IN "1\r" # VT220 Tests
    IN "2\r" # VT220 Screen-Display Tests
    IN "1\r" # Test Send/Receive Mode
    IN "Subidubidoo\D1Subidubidoo\D1"
    SNAP vt_11_01_02_01 1a9a69e29be97d820182522ece721a13
    IN "\r"
    IN "2\r" # Test Visible/Invisible Cursor
    SNAP vt_11_01_02_02_01 15a8b8f28a10ff3091690bd083f78ceb
    IN "\r"
    SNAP vt_11_01_02_02_02 d29c49e9be5012964ba486b66f196fe8
    IN "\r"
    IN "3\r" # Test Erase Char (ECH)
    SNAP vt_11_01_02_03 c823f2c76af91d992dccb9d3bd6448c8
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "2\r" # VT320 Tests
    IN "2\r" # Test cursor-movement
    IN "1\r" # Test Pan Down (SU)
    SNAP vt_11_02_02_01 62764215a24fe1f90e81c9678d79735a
    IN "\r"
    IN "2\r" # Test Pan Up (SD)
    SNAP vt_11_02_02_02 f23ba052fae3349bb1be427194674827
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "3\r" # VT420 Tests
    IN "2\r" # Test cursor-movement
    IN "6\r" # Color test-regions
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_01 c73df47f269b196208f120f6b9050e98
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_01 b6b573c3c52c7eba0894599dbb8b0518
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_01 0a9ec15345713e9bfbd92bf2c829dede
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_01 64fedbd3b2c2b4d2ec84c3d426d5e9cf
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to top half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_02 1f5aea6b05413e37b5d91f553fdbfb3c
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_02 a995ec9bcbb38e81cfd23db4140064a3
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_02 176271c0268dc1d144ed68e649488dcd
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_02 b0ff797bbb49a6b199279303ac3be783
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to bottom half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_03 82f14ccd50d2fa0bf9af3d4803b14955
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_03 48d4ab0e4d9fd186dd01c0e8f6a2711d
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_03 36c1e1e6c87399b5df587163640dba47
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_03 01aa82172bf7658f632a02277899bf78
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to middle half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_04 d67bed18602417f1a12f37542e666210
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_04 b9a5fcccd079411375fe1b953ff3cbb0
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_04 8da318aa9008052775859caa426c8574
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_04 709198649248b78ce16e77560f8ef7ae
    IN "\r"
    IN "4\r" # Top/Bottom margins are reset
    IN "3\r" # Enable DECLRMM (left/right mode)
    IN "5\r" # Left/Right margins are set to left half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_05 f4733b367ad0d92cf68499b56d1a0967
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_05 71464ccf5d191e957612c9676cb774ef
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_05 7a8aaaa6e9633b134d3c325bad26aa8b
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_05 bb1b2f465352bc1be5e47250c447a331
    IN "\r"
    IN "5\r" # Left/Right margins are set to right half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_06 b55ae9eb3ab0e96b02ee40e796aa5ae2
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_06 5b08d36cfe69faa62d546a5560e88e91
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_06 fb66ea22e6c9684e2b0ddb88f95e2547
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_06 d57bcc097ad7229f71855682ee7bba22
    IN "\r"
    IN "5\r" # Left/Right margins are set to middle half of screen
    IN "7\r" # Test Back Index (BI)
    SNAP vt_11_03_02_07_07 968eef03ef9fcc8b41ea1f513455a538
    IN "\r"
    IN "8\r" # Test Forward Index (FI)
    SNAP vt_11_03_02_08_07 4a0d1cea14942f839e331b1cd444f82d
    IN "\r"
    IN "9\r" # Test cursor movement within margins
    SNAP vt_11_03_02_09_07 d6b8514303928a596f8593719bae7af8
    IN "\r"
    IN "10\r" # Test other movement within margins
    SNAP vt_11_03_02_10_07 9b5dd2a33f79a98fea7fa6cbf46bafb9
    IN "\r"
    IN "0\r"

    IN "3\r" # VT420 Editing Sequences
    IN "5\r" # Color test-regions
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_01a 14bbfcab4696f19b634c71f00f7e7b60
    IN "\r"
    SNAP vt_11_03_03_09_01b 22adba4322e3b2fa323496e80b477643
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_01a 9a310e01d6d5b0bf494d7d88d730f1c0
    IN "\r\r"
    SNAP vt_11_03_03_10_01b 35d423164fe2acb6d6a3d0dcfb4e1965
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_01a 35d423164fe2acb6d6a3d0dcfb4e1965
    IN "\r\r"
    SNAP vt_11_03_03_11_01b 9a310e01d6d5b0bf494d7d88d730f1c0
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_01a 84db4a39b09ef3b22aedf07ffa3b604b
    IN "\r"
    SNAP vt_11_03_03_12_01b b5d951243eb3d5ca83cd855bcae150f5
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_01 ad7fc21351dfe257154578e42d37b1f3
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to top half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_02a 7574137aa9b9c9b5fe0931369d94db97
    IN "\r"
    SNAP vt_11_03_03_09_02b 77faad730a97e04ceaf1be6d721bffb2
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_02a 93419dff4f7104eb2708fa2065de5690
    IN "\r\r"
    SNAP vt_11_03_03_10_02b e485499fc8051d91514570407ab57a94
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_02a 20d6d5f3f7d97ad101893770b4038ae7
    IN "\r\r"
    SNAP vt_11_03_03_11_02b b099fb20c8f42149f431e33058bc4b1e
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_02a 5a85eef7521c36f13a7e1a645ded7c25
    IN "\r"
    SNAP vt_11_03_03_12_02b 914ee70e615964d1893c6f11fcea5cca
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_02 c33aec43990b0a4021648259f659deaf
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to bottom half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_03a 99f5f02dd644dd755adbd255c3a46c18
    IN "\r"
    SNAP vt_11_03_03_09_03b fddf91000e0e50d8ece09d87649d063d
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_03a 75d7b3c2dcdc4d1b5a0036a5fdce573e
    IN "\r\r"
    SNAP vt_11_03_03_10_03b a158e865ca72fee44d219e38cd1ae71b
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_03a 53fc3c83787c24fe0867ba5717eed6d3
    IN "\r\r"
    SNAP vt_11_03_03_11_03b 3dfd44706afc5f6c1eda3af0cafed082
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_03a cca8fd61608865e1edead1420cccb894
    IN "\r"
    SNAP vt_11_03_03_12_03b e53798c199bc8c4f0f140934972b90ca
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_03 3d99e0a735ec7b24382e9b8d811479ea
    IN "\r"
    IN "3\r" # Top/Bottom margins are set to middle half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_04a 2b2000586bba2c903b3235fdfbea6d08
    IN "\r"
    SNAP vt_11_03_03_09_04b 183959b69a6dbc6f916dc156d7388b26
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_04a e89fde9af7a3d9497b9df5547caddba5
    IN "\r\r"
    SNAP vt_11_03_03_10_04b 0460e562c77325fedb10cc0547927906
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_04a d6dd3caba4eff9130dbfc3ab27a04a58
    IN "\r\r"
    SNAP vt_11_03_03_11_04b e5516846b232b14b90d4fb7b7f526eac
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_04a 2357a1d1e25eb5b0b869448b9da63ba7
    IN "\r"
    SNAP vt_11_03_03_12_04b 3916258123268ae994a71c85bcb83884
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_04 6926417da2a660a1f9243c709d41f268
    IN "\r"
    IN "3\r" # Top/Bottom margins are reset
    IN "2\r" # Enable DECLRMM (left/right mode)
    IN "4\r" # Left/Right margins are set to left half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_05a b6c7b64efeb2c4bb2366610ca3f96dbd
    IN "\r"
    SNAP vt_11_03_03_09_05b bd808658e99213cceffc99697d7e4153
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_05a 33bfb1ec1703b75108b61f7a4d71ee5b
    IN "\r\r"
    SNAP vt_11_03_03_10_05b b0ae034833228f898954ab951e3bbeea
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_05a d3e27ca86e54bb4e0b6529f8c8b31da2
    IN "\r\r"
    SNAP vt_11_03_03_11_05b a076afeb72458eb8505a3555b108bc18
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_05a 57c4499b67374737b3b643f433142af6
    IN "\r"
    SNAP vt_11_03_03_12_05b ed1a9e38f21071506f19f82752326930
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_05 d9157c0da7b2dfe8a7531cf40436ac1d
    IN "\r"
    IN "4\r" # Left/Right margins are set to right half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_06a c5f83e9dca026fd2366c3beca415f47b
    IN "\r"
    SNAP vt_11_03_03_09_06b 02dd71b281004a2333abc0d47d1f2965
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_06a df58da35fd49a0efdda76bb644dda213
    IN "\r\r"
    SNAP vt_11_03_03_10_06b 0e9e493a6ea7fe4a4dbb8e68be09b084
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_06a cd881e1b8d80c70154ecbdb1686a0939
    IN "\r\r"
    SNAP vt_11_03_03_11_06b 3e567cec673863215fb9cba66f495de1
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_06a 2bd35dd458506cdab1199792bda5b861
    IN "\r"
    SNAP vt_11_03_03_12_06b e24b7d57ea33ed7b71312d5ad9588487
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_06 0ac7759b643f4878becf594eb55cc100
    IN "\r"
    IN "4\r" # Left/Right margins are set to middle half of screen
    IN "9\r" # Test insert/delete column (DECIC, DECDC)
    SNAP vt_11_03_03_09_07a afc872967688bead9a83368e5747f57f
    IN "\r"
    SNAP vt_11_03_03_09_07b bebe1dfe44102b01cfadf51f35dcfc5e
    IN "\r"
    IN "10\r\r" # Test vertical scrolling (IND, RI)
    SNAP vt_11_03_03_10_07a 46e6867ed643642614a6bba671181e08
    IN "\r\r"
    SNAP vt_11_03_03_10_07b 2c1a82bfdc314387a5076016b11eed13
    IN "\r"
    IN "11\r\r" # Test insert/delete line (IL, DL)
    SNAP vt_11_03_03_11_07a 2881b12c9bffde4ad2f6f3a1a410e4f4
    IN "\r\r"
    SNAP vt_11_03_03_11_07b d6a126d5244b127928549d0457335eb7
    IN "\r"
    IN "12\r" # Test insert/delete char (ICH, DCH)
    SNAP vt_11_03_03_12_07a 816eb609f41c6694d4799c0f259d6662
    IN "\r"
    SNAP vt_11_03_03_12_07b 78ca06ffabfd7fb914840f8d3904a19c
    IN "\r"
    IN "13\r" # Test ASCII formatting (BS, CR, TAB)
    SNAP vt_11_03_03_13_07 9622e6ff07a4abda08f1f12203c58a14
    IN "\r"
    IN "0\r"

    IN "4\r" # VT420 Keyboard-Control Tests
    IN "1\r" # Test Backarrow Key (DECBKM)
    IN "\b\D3\b"
    SNAP vt_11_03_04_01 716027ee9e70d37505ddc82460dcdd1c
    IN "\r"
    IN "0\r"
    IN "0\r"

    IN "4\r" # VT520 Tests
    IN "2\r" # VT520 cursor-movement
    IN "6\r" # Color test-regions
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_01 f8d112a4b8e5b323341a1e86e10f6ded
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_01 9c5747ba41d02335eab245717101aabc
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_01 054e3b2c1f74e66d75cf2efa83d85927
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_01 05f365f663773ca29db9cc2fa8af130f
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_01 12d9217894f587c73f5c2c699244c8ee
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_01 9f81b832ebb15f6ae26a4e2ebca7254d
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_01 7aa0d70e2b4e7678fb234c8c419169db
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_01 c3dadd63adee7028ea9defe1ea73198a
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_01 09fb397a4d5f89351ea9c520ffc0907f
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to top half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_02 12fd48c0bdbdf660b162822447c1c855
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_02 8a3cf937d076f123ba9a51ae7adc2194
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_02 0fb535bd9c272ecc4743cae6805cd2db
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_02 95d9c1e68ee9e167bb85be907d14c05c
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_02 bb18c0bb03559fdafeeb0ceb979dcc11
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_02 764857cde6ec7769a401f8226b4b1c54
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_02 48f56c298f45fb8379a9c6b9e9d0e49c
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_02 c34f23ef8074af0820361381700b6a93
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_02 829b988147b2b1d3e11fe73a9064c5c3
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to bottom half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_03 5d916d32819e9e76f4cf4803d2ffa018
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_03 761fcdb51653ec885984448ea5e1763a
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_03 3b40ecfecd7e1e3ef829cb47e232fb5e
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_03 79c7c6e6e91eedbba99ac26070d14fe5
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_03 8f647267f7b5b072019d7587b875ff76
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_03 738083af59a22bcff0a6db3ed0e030f1
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_03 f603244cee5d7c87e5ea1f3540066122
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_03 3c44a061629fc40f48f21623c30a81f7
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_03 ae54e95fe7647920ea7d7545078788c2
    IN "\r"
    IN "4\r" # Top/Bottom margins are set to middle half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_04 76bac3175ffe96834bb366502f4077db
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_04 21bde13d638ac58664e7d7f902165c85
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_04 b4d276df63b6a8cf6a1edf16504dd3a7
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_04 d41000aa84285f4ea08308a35a5ed973
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_04 3bed021e02e7ffa4830839acd1c4d0fd
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_04 c0227f3aea2ca1d9317e58e980dcbdb6
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_04 c41886ebaffb2593805baf85cfd13882
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_04 6528a29ec10de921d43aa5e0efc0553c
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_04 5329ca8525fb86006a1b376a68fe81da
    IN "\r"
    IN "4\r" # Top/Bottom margins are reset
    IN "3\r" # Enable DECLRMM (left/right mode)
    IN "5\r" # Left/right margins are set to left half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_05 da57d0d4a995fc57e0e82f26890e558a
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_05 af722e4cce2d81bb27aa10c688dc8e29
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_05 94dab29595fc3d8d70d359fae4f5b76d
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_05 8e2bc2cad2120d9467818971e4ea0e91
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_05 9cbace667c9d5be1950205eb18711b61
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_05 d1bb1ef4f230c6736246251215fe3b26
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_05 5b9c63f652adb71b4466d1bbeebfe000
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_05 fa4b2a60c62b5f559633b328e00dc677
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_05 e515a977ad7f30cddba916cea8802764
    IN "\r"
    IN "5\r" # Left/right margins are set to right half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_06 57f6fc1342052b4f151e87d4bc90f9e7
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_06 9142b815b833a5cd05fe5b5e6f45e54b
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_06 8aa64c9033fd2e4ab29a87b5da0b261a
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_06 b25ab4e0458a7137dde7715ab3d6cff9
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_06 79339821a7dd4db3fa9f035f4244493f
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_06 637b9c630787ed2b36c1ddb2e0e04fa7
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_06 d0fd73b732ba36b01f235a934249535f
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_06 1fe58ede5c7b5ee6b99c3e8eff81bd49
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_06 06ec75a35ab7ad36f4666b80fbfe5ee6
    IN "\r"
    IN "5\r" # Left/right margins are set to middle half of screen
    IN "7\r" # Test Character-Position-Absolute (HPA)
    SNAP vt_11_04_02_07_07 6c62fdaf3664c0483d748b4593ab7c4d
    IN "\r"
    IN "8\r" # Test Cursor-Back-Tab (CBT)
    SNAP vt_11_04_02_08_07 caa1776d46320fcef28a15170df455e0
    IN "\r"
    IN "9\r" # Test Cursor-Character-Absolute (CHA)
    SNAP vt_11_04_02_09_07 75bed4faf3bf1c506f3fed606eb82a2b
    IN "\r"
    IN "10\r" # Test Cursor-Horizontal-Index (CHT)
    SNAP vt_11_04_02_10_07 6e0157d22c9d504a8e14f23be75f4cd5
    IN "\r"
    IN "11\r" # Test Horizontal-Position-Relative (HPR)
    SNAP vt_11_04_02_11_07 6d260bb190de2dea4f72e41b35695247
    IN "\r"
    IN "12\r" # Test Line-Position-Absolute (VPA)
    SNAP vt_11_04_02_12_07 12b7cc61450b86c8254ad068287b8e14
    IN "\r"
    IN "13\r" # Test Next-Line (CNL)
    SNAP vt_11_04_02_13_07 c61325eab90d37fef34f5a343198d144
    IN "\r"
    IN "14\r" # Test Previous-Line (CPL)
    SNAP vt_11_04_02_14_07 3917afeb2e3bebc2ce65a5427722130f
    IN "\r"
    IN "15\r" # Test Vertical-Position-Relative (VPR)
    SNAP vt_11_04_02_15_07 5b46dff0c40888bacc199415074f0fa0
    IN "\r"
    IN "0\r"
    IN "0\r"

    # At this point we restart vttest, to avoid bumping into
    # a bug that produces a flawed input sequence for vt_11_06_04
    # resulting in wrong output (regardless of terminal emulator).
    echo "Restarting vttest to work-around bug in 11.6.4 ..."
    IN "0\r"
    IN "0\r"
    IN "vttest\r"
    IN "11\r"

    IN "6\r" # ISO 6429 colors
    IN "2\r" # Display color test-pattern
    SNAP vt_11_06_02 85594a0e6c9227455e8854005befbcde
    IN "\r"
    IN "3\r" # Test SGR-0 color reset
    SNAP vt_11_06_03 5a99f807644c8cc00b0486aed5e870b1
    IN "\r"
    IN "4\r" # Test BCE-style clear line/display (ED, EL)
    SNAP vt_11_06_04_01 22fe2cbc8dd1eda36739922437856fe9
    IN "\r"
    SNAP vt_11_06_04_02 ac9e38462210e4728468df4ee7cdf7b8
    IN "\r"
    IN "5\r" # Test BCE-style clear line/display (ECH, Indexing)
    SNAP vt_11_06_05_01 22fe2cbc8dd1eda36739922437856fe9
    IN "\r"
    SNAP vt_11_06_05_02 ac9e38462210e4728468df4ee7cdf7b8
    IN "\r"
    IN "6\r" # Test VT102-style features with BCE
    IN "1\r" # Test of cursor movements
    SNAP vt_11_06_06_01_01 f77d06d60409e1205d88346cc86a24b3
    IN "\r"
    SNAP vt_11_06_06_01_02 e8a834b92c86b4353d350ec49de0ca15
    IN "\r"
    SNAP vt_11_06_06_01_03 1cd235a4760ff659fd0d2d3cbc4ed83d
    IN "\r"
    SNAP vt_11_06_06_01_04 1cd235a4760ff659fd0d2d3cbc4ed83d
    IN "\r"
    SNAP vt_11_06_06_01_05 5c4e9b105d39ce5d8bb6c176926bbfdf
    IN "\r"
    IN "2\r" # Test of screen features
    SNAP vt_11_06_06_02_01 eeae32bd506c31f532a6bef8f3067cb5
    IN "\r"
    SNAP vt_11_06_06_02_02 6687007defc04563d895ed65e54fa9f8
    IN "\r"
    SNAP vt_11_06_06_02_03 dc29e96f285c34b7ed71f5e4d9870cfb
    IN "\r"
    SNAP vt_11_06_06_02_04 20772c7d162284cd32bc20ee92d35714
    IN "\r"
    SNAP vt_11_06_06_02_05 dbb1b8e9da54130803e939541c814426
    IN "\r"
    SNAP vt_11_06_06_02_06 acbe8949539945595a4b47cdbc6de8ac
    IN "\r"
    SNAP vt_11_06_06_02_07 0c83792b11d0b0241159691cb366a028
    IN "\r"
    SNAP vt_11_06_06_02_08 826ed49f750b1f71a140eebb8e0f271d
    IN "\r"
    SNAP vt_11_06_06_02_09 f456d874dd1e7c661ed7985ef1246103
    IN "\r"
    SNAP vt_11_06_06_02_10 f665543ab609f0728e1e9b7246a76538
    IN "\r"
    SNAP vt_11_06_06_02_11 683802dedc9930ed882db4cc84586e36
    IN "\r"
    SNAP vt_11_06_06_02_12 812117018b72507433c390cfab02fef8
    IN "\r"
    SNAP vt_11_06_06_02_13 f8b963bf967baf3292bc01054fc8e661
    IN "\r"
    SNAP vt_11_06_06_02_14 62905438e5f2c97f6db25bd8c8e0e967
    IN "\r"
    IN "3\r" # Test Insert/Delete Char/Line
    SNAP vt_11_06_06_03_01 a720f1ca9686a8ca0bc602e5f5d442b4
    IN "\r"
    SNAP vt_11_06_06_03_02 66b84bff6848198560db7aaae24898a5
    IN "\r"
    SNAP vt_11_06_06_03_03 2eb806c510d2eb41ede0325aa395ca82
    IN "\r"
    SNAP vt_11_06_06_03_04 bf5cf13734be6771da6f97c85240a454
    IN "\r"
    SNAP vt_11_06_06_03_05 d3421c8efaa692b4fa4de707733dccec
    IN "\r"
    SNAP vt_11_06_06_03_06 1f299a2b8c776fac036044c6580d25e5
    IN "\r"
    SNAP vt_11_06_06_03_07 cca679be8fb1427c77342e76e6f1a4ce
    IN "\r"
    SNAP vt_11_06_06_03_08 8e1d6dd5e121e38339d44a80c9e5e630
    IN "\r"
    SNAP vt_11_06_06_03_09 3e148ed184c023c865814ccba8872603
    IN "\r"
    SNAP vt_11_06_06_03_10 bcf4c4f8fb5ebb94ca02432430ba168f
    IN "\r"
    SNAP vt_11_06_06_03_11 adcfdbba8c6eddbca0c6a43a8231d597
    IN "\r"
    SNAP vt_11_06_06_03_12 17cf05d4f95051b80420ca8d3270227f
    IN "\r"
    SNAP vt_11_06_06_03_13 936f0d5d8b42a50c771eee4d61541a3a
    IN "\r"
    SNAP vt_11_06_06_03_14 33277858b7d75ec99ee2a485e3910485
    IN "\r"
    SNAP vt_11_06_06_03_15 c8f2fe9a22b5757c901efa915f88f37c
    IN "\r"
    SNAP vt_11_06_06_03_16 3e148ed184c023c865814ccba8872603
    IN "\r"
    IN "0\r"
    IN "7\r" # Miscellaneous ISO-6429 (ECMA-48) Tests
    IN "2\r" # Test Repeat (REP)
    SNAP vt_11_06_07_02 e56b90739094f098d55e7f1b8493a65d
    IN "\r"
    IN "3\r" # Test Scroll-Down (SD)
    SNAP vt_11_06_07_03 5046d8de1f748cb15621a80250cd773d
    IN "\r"
    IN "4\r" # Test Scroll-Left (SL)
    SNAP vt_11_06_07_04 006bae0f2ce5c2ca6ff872a09f782c4a
    IN "\r"
    IN "5\r" # Test Scroll-Right (SR)
    SNAP vt_11_06_07_05 e5f136a489a9637322e63b6f90e667db
    IN "\r"
    IN "6\r" # Test Scroll-Up (SU)
    SNAP vt_11_06_07_06 081bb881dbe2832ea0aba9341c1a7833
    IN "\r"
    IN "0\r"
    IN "8\r" # Test screen features with BCE
    SNAP vt_11_06_08_01 0c83792b11d0b0241159691cb366a028
    IN "\r"
    SNAP vt_11_06_08_02 826ed49f750b1f71a140eebb8e0f271d
    IN "\r"
    SNAP vt_11_06_08_03 f456d874dd1e7c661ed7985ef1246103
    IN "\r"
    SNAP vt_11_06_08_04 f665543ab609f0728e1e9b7246a76538
    IN "\r"
    SNAP vt_11_06_08_05 ca444d80b2cf846cbe97860393f2241d
    IN "\r"
    SNAP vt_11_06_08_06 823e23a6bec24a28aac652c68452b114
    IN "\r"
    IN "9\r" # Test screen features with ISO 6429 SGR 22-27 codes
    SNAP vt_11_06_09_01 2dbaeaf36781a3d537d5c386a98ce691
    IN "\r"
    SNAP vt_11_06_09_02 59367d828a5eb71628fe7d87c0a67a3a
    IN "\r"
    SNAP vt_11_06_09_03 2dbaeaf36781a3d537d5c386a98ce691
    IN "\r"
    IN "0\r"

    IN "7\r" # Miscellaneous ISO-6429 (ECMA-48) Tests
    IN "2\r" # Test Repeat (REP)
    SNAP vt_11_07_02 b9e4018e9830bce69133fd2c62e46032
    IN "\r"
    IN "3\r" # Test Scroll-Down (SD)
    SNAP vt_11_07_03 e2d11e6b2704eb8e583436675408f33f
    IN "\r"
    IN "4\r" # Test Scroll-Left (SL)
    SNAP vt_11_07_04 0ef10be2a79f548bed58df62e8eac433
    IN "\r"
    IN "5\r" # Test Scroll-Right (SR)
    SNAP vt_11_07_05 ef5780a9c7c9c44c9db0037deb25c30d
    IN "\r"
    IN "6\r" # Test Scroll-Up (SU)
    SNAP vt_11_07_06 25ced4449004f338a3d6dd673eb8954e
    IN "\r"
    IN "0\r"

    IN "8\r" # XTERM special features
    IN "7\r" # Alternate-Screen features
    IN "1\r" # Switch to/from alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_08_07_01 c5b33c89c294737c60206ddac57c59d3
    IN "\r"

    IN "2\r" # Improved alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_08_07_02 b27206a789930441252ea9815d178699
    IN "\r"

    IN "3\r" # Better alternate screen
    IN "\r"
    IN "\r"
    SNAP vt_11_08_07_03 c76d332c63733d2419d18b83d4672c57
    IN "\r"

    IN "0\r"
    IN "0\r"
    IN "0\r"
}

setxkbmap us

IN "vttest\r"

VT_1
VT_2
VT_3
VT_5
VT_6
VT_7
VT_8
VT_9
VT_10
VT_11

IN "0\r"
