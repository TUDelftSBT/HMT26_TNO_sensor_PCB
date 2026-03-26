#!/bin/bash

mkdir -p ./trice/logs/
trice log -p /dev/ttyACM0 -baud 115200 -lf ./trice/logs/auto
