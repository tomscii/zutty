#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

function title {
    inc=$1
    IN "source ${inc}\r"
    title=$(cat ${inc} | cut -c 15-)
    len=$(echo ${title} | wc -c)
    len=$((len - 7))
    exp_title=$(echo ${title} | cut -c -${len})
    read_title=$(wmctrl -lp | awk "/${WID}/ {\$1=\$2=\$3=\$4=\"\"; print \$0}" | cut -c 5-)
    if [[ "${read_title}" == "${exp_title}" ]] ; then
        COUNT_PASS
        echo "Title set OK: \"${exp_title}\""
    else
        COUNT_FAIL
        echo "Title set FAIL:"
        echo "   Read as: \"${read_title}\""
        echo "  Expected: \"${exp_title}\""
        EXIT_CODE=1
    fi
}

IN "source title_inc_setup.sh\r"
title title_inc_01.sh
title title_inc_02.sh
title title_inc_03.sh
