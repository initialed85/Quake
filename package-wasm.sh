#!/bin/bash

set -e

detected_arch="amd64"
if ! file ./dist/quake/Quake | grep 'x86' >/dev/null; then
	detected_arch="arm64"
fi

desired_arch="${ARCH:-${detected_arch}}"

image_1="${IMAGE_1:-initialed85/quake-wasm-nginx:latest}"
image_2="${IMAGE_2:-initialed85/quake-ws-server:latest}"
image_3="${IMAGE_2:-initialed85/quake-wasm-node:latest}"

docker build --platform="linux/${desired_arch}" -t "${image_1}" -f ./Dockerfile.nginx .
docker build --platform="linux/${desired_arch}" -t "${image_2}" -f ./Dockerfile.ws-server .
docker build --platform="linux/${desired_arch}" -t "${image_3}" -f ./Dockerfile.node .

if [[ "${PUSH}" == "1" ]]; then
	docker image push "${image_1}"
	docker image push "${image_2}"
	docker image push "${image_3}"
fi

echo -e "\ndone"
