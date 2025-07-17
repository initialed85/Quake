#!/bin/bash

set -e

if ! command -v entr >/dev/null 2>&1; then
    echo "error: entr not found; have you installed it?"
    exit 1
fi

bash -c "find WinQuake/CMakeLists.txt Dockerfile ; find WinQuake/ -type f -name '*.c'; find WinQuake/ -type f -name '*.h'; find WinQuake/ -type f -name '*.html'" | grep -v 'build-' | entr -n -r -cc -s "DEBUG=${DEBUG} ./build-native.sh"
