#!/bin/bash
echo "Creating testbench"
BINARY=~/ros2_ws/install/rclcpp_executor_testbench/lib/rclcpp_executor_testbench/rclcpp_executor_testbench
RATE=1
MSG_SIZE=2
TOPOLOGY=F
PROCESS_TYPE=all
NUM_TOPICS=1
NUM_SUBS=10


echo script $BINARY $NUM_TOPICS $TOPOLOGY
python3 ./configure_test.py $BINARY $RATE $MSG_SIZE $TOPOLOGY $PROCESS_TYPE $NUM_TOPICS $NUM_SUBS

