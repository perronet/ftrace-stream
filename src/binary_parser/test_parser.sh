#!/bin/bash

# Check this:
# 1) Wakeups should be spaced by 2 seconds
# 2) Between a wakeup and switch with prev_state=S there should be 1 second

sudo echo
make

cd ../../../scripts
python3 evaluation/generate_workload.py --workload evaluation/workloads/uniprocessor_single/1_uni_single_slow.json &

sleep 1

PID=$(pgrep rtspin)
sudo ../src/events_generation/binary_parser/bin_parser.out "$PID"

# EXAMPLE WITH TRACE-CMD
# sudo trace-cmd stream -e "sched_switch" -e "sched_wakeup" -e "sched_process_exit" -P 3410 