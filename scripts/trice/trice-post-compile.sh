#!/bin/bash

# Only do trice for the project, otherwise it is way too long.
mkdir -p /root/.trice/cache/
trice clean -src ./Project/ -src ./Tests -src ./Libs/trice/trice-config/Src/ -src ./Core/ -cache
