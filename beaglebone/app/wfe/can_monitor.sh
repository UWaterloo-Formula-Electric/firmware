#!/bin/bash
echo "Started can_monitor script"
while ! ip link show can1 | grep "UP" > /dev/null; do
        :
done
echo "CAN1 is up"

n=0;
max_attempts=50;

i=1
while true
do
        logname="/home/debian/can_monitor_logs/$i.log"
        if [[ ! -f $logname ]]
        then
                break
        fi
        i=$((i+1))

done

echo $d
while [ $n -lt $max_attempts ]; do
        echo "Attempting candump for the ${n}th time"
        candump can1 -t a > $logname
        sleep 0.25;
        let n=$n+1
done