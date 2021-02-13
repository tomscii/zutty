#!/usr/bin/env bash

cd $(dirname $0)
source testbase.sh

export VERIFY_SNAPS=no # Override profile setting

CHECK_DEPS dc
DICT=/usr/share/dict/british-english-huge
CHECK_FILES ${DICT}
TIMES=100
BYTES=$(stat -c%s ${DICT})

echo "Sending ${DICT} to the terminal." > ${TEST_LOG}
echo "Filesize: ${BYTES} bytes" >> ${TEST_LOG}
echo "Repeated: ${TIMES} times" >> ${TEST_LOG}
echo "Timings in seconds:" >> ${TEST_LOG}

if [ -z ${HMARGINS} ] ; then
    echo "Horizontal margins not set; run with HMARGINS=1 to enable."
else
    IN "source set_hmargins_inc.sh\r"
fi
IN "{ time -p for i in \$(seq 1 ${TIMES}); do cat ${DICT}; done } 2>>${TEST_LOG} && touch .complete\r"

WAIT_FOR_DOT_COMPLETE

real_secs=$(grep "^real" ${TEST_LOG} | awk '{print $2}')
bytes_per_sec=$(dc -e "${BYTES} ${TIMES} * ${real_secs} / p")
echo "Calculated throughput: ${bytes_per_sec} bytes/sec" >> ${TEST_LOG}

cat ${TEST_LOG}
