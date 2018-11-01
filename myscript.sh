#!/bin/bash

make

./daemon start
sleep 10
./daemon stop
sleep 60

rngtest < ~/random/data

tail /var/log/syslog

