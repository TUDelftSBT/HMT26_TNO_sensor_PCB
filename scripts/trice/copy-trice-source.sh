#!/bin/bash

# The argument must be ${localWorkspaceFolderBasename}
mkdir -p /workspaces/$1/Libs/trice/trice/

# Prevent recompilation by using -n flag
cp -r -n /opt/trice/src/* /workspaces/$1/Libs/trice/trice/
