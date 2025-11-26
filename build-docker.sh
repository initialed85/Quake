#!/bin/bash

set -e

if ! command -v docker >/dev/null 2>&1; then
	echo "error: docker not found; have you installed it?"
	exit 1
fi

function cleanup() {
	docker stop -t0 quake >/dev/null 2>&1 || true
	docker rm -f quake >/dev/null 2>&1 || true
}

trap cleanup EXIT

detected_arch="amd64"
if [[ "$(uname -p)" == "arm" ]]; then
	detected_arch="arm64"
fi

desired_arch="${ARCH:-${detected_arch}}"

if [[ "${desired_arch}" != "amd64" ]]; then
	echo "error: only ARCH=amd64 is supported for now (detected_arch=${detected_arch}, desired_arch=${desired_arch})"
	exit 1
fi

docker build --platform="linux/${desired_arch}" --progress=plain --build-arg "DEBUG=${DEBUG:-0}" -t "initialed85/quake-build:latest" -f ./Dockerfile .

docker run --platform="linux/${desired_arch}" -d --rm --entrypoint bash --name quake "initialed85/quake-build:latest" -c 'tail -F /dev/null'

rm -fr ./WinQuake/build-docker >/dev/null 2>&1 || true

docker cp quake:/srv/WinQuake/build-linux ./WinQuake/build-docker
