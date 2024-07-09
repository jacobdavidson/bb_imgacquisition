#!/bin/bash
# run in an infinite loop with highest priority, so that it restarts after a crash

cd build

while true
do
	nice -n -20 ./bb_imgacquisition
	date >> crashreport_bb_imgacquisition.txt
done
