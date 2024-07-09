#!/bin/bash
# run in an infinite loop with highest priority, so that it restarts after a crash

while true
do
	nice -n -20 ./build/bb_imgacquisition
	date >> build/crashreport_bb_imgacquisition.txt
done
