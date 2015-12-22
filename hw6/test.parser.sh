#!/bin/sh

ERRDIR=..
REFERRLOG=${ERRDIR}/err_ref.log
CMPERRLOG=${ERRDIR}/err_cmp.log

cd tests.err
echo "q -f" | ../ref/cirTest-ref -f do.err > $REFERRLOG 2>&1
echo "q -f" | ../cirTest -f do.err > $CMPERRLOG 2>&1
vimdiff $CMPERRLOG $REFERRLOG
cd - > /dev/null

