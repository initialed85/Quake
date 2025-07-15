#!/bin/bash

set -e

rm -fr ./dist >/dev/null 2>&1

mkdir -p ./dist/quake

cp -frv ./WinQuake/build-docker/libQuakeLib.a ./dist/quake/
cp -frv ./WinQuake/build-docker/libSDL2-2.0.so.0 ./dist/quake/
cp -frv ./WinQuake/build-docker/libSDL2-2.0.so.0.3200.4 ./dist/quake/
cp -frv ./WinQuake/build-docker/Quake ./dist/quake/
cp -frv ./WinQuake/id1 ./dist/quake/

pushd dist
tar -czvf quake.tar.gz quake
