#!/bin/bash
echo "Started can_monitor script"
while ! ip link show can1 | grep "UP" > /dev/null; do 
	:
done
echo "CAN1 is up"

n=0;
max_attempts=50;
d=$(date +%b%d_%Y_%k_%M_%S)
f_name="${d}.log"
echo $d
while [ $n -lt $max_attempts ]; do
	echo "Attempting candump for the ${n}th time"
	candump can1 -t a > /home/debian/can_monitor_logs/$f_name
	sleep 0.25;
	let n=$n+1
done
