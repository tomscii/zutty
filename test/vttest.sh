#!/bin/bash

cd $(dirname $0)
source testbase.sh

function VT_1 {
    IN "1\r"
    SNAP vt_01_01 7d5405859f5e3c7e4048c4d91630a120
    IN "\r"
    SNAP vt_01_02 20e218fbe8f40cea4a7b8b8790a772f4
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

IN "vttest\r"
VT_1
VT_2
VT_3
VT_5
VT_6
VT_8
IN "0\r"
