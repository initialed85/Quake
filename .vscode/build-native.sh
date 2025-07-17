#!/bin/bash

set -e

if ! DEBUG=1 ./build-native.sh; then
    exit 1
fi

exit 0
