#!/bin/bash

mkdir -p /root/.trice/cache/
trice insert -src ./Project/ -src ./Tests -src ./Libs/trice/trice-config/Src/ -src ./Core/ -cache
