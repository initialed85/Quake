#!/bin/bash

set -e

if [[ "${SKIP_BUILD}" != "1" ]]; then
	DEBUG=1 ./build-wasm.sh
	echo ''
fi

SERVER_8080_PID=""
SERVER_8081_PID=""

cleanup() {
	kill "${SERVER_8080_PID}" "${SERVER_8081_PID}" >/dev/null 2>&1 || true
	wait "${SERVER_8080_PID}" "${SERVER_8081_PID}" >/dev/null 2>&1 || true
}

trap cleanup EXIT

(cd ws-server && go build -o ../.vscode/ws-server-8080 && ./ws-server-8080 &)
SERVER_8080_PID=$!

(cd ws-server && go build -o ../.vscode/ws-server-8081 && PORT=8081 ./ws-server-8081 &)
SERVER_8081_PID=$!

sleep 2

node --inspect-brk WinQuake/build-wasm/tests.js
