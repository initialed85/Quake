#!/bin/bash

set -e

rm -fr ./dist >/dev/null 2>&1

mkdir -p ./dist/quake

cp -frv ./WinQuake/build-docker/libQuakeLib.a ./dist/quake/
cp -frv ./WinQuake/build-docker/libSDL2-2.0* ./dist/quake/
cp -frv ./WinQuake/build-docker/Quake ./dist/quake/

mkdir -p ./dist/quake/id1
cp -frv ./WinQuake/id1/*.pak ./dist/quake/id1/
cp -frv ./WinQuake/id1/*.cfg ./dist/quake/id1/
cp -frv ./WinQuake/id1/*.dat ./dist/quake/id1/

pushd dist
tar -czvf quake.tar.gz quake
popd

detected_arch="amd64"
if ! file ./dist/quake/Quake | grep 'x86' >/dev/null; then
	detected_arch="arm64"
fi

desired_arch="${ARCH:-${detected_arch}}"

image="${IMAGE:-initialed85/quake:latest}"

docker build --platform="linux/${desired_arch}" -t "${image}" -f ./Dockerfile.dist .

if [[ "${PUSH}" == "1" ]]; then
	docker image push "${image}"
fi
