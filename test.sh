#!/bin/bash

set -e

DEBUG=1 ./build-wasm.sh

echo ''

node --inspect-brk WinQuake/build-wasm/tests.js
