#!/bin/bash

set -e

ARCH=amd64 DEBUG=1 ./build-docker.sh

ARCH=amd64 ./run-docker.sh down || true

for _ in {0..10}; do
    if ! ARCH=amd64 DEBUG=1 ./run-docker.sh up; then
        sleep 1
        continue
    fi

    break
done

for _ in {0..10}; do
    if ! nmap -p 11111 --open localhost | grep open >/dev/null 2>&1; then
        sleep 1
        continue
    fi

    break
done
