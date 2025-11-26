#!/bin/bash

set -e

rm -fr ./dist >/dev/null 2>&1

mkdir -p ./dist/quake

cp -frv ./WinQuake/build-native/libQuakeLib.a ./dist/quake/
cp -frv ./WinQuake/build-native/libSDL2-2.0* ./dist/quake/
cp -frv ./WinQuake/build-native/Quake ./dist/quake/

mkdir -p ./dist/quake/id1
cp -frv ./WinQuake/id1/*.pak ./dist/quake/id1/
cp -frv ./WinQuake/id1/*.cfg ./dist/quake/id1/
cp -frv ./WinQuake/id1/*.dat ./dist/quake/id1/

pushd dist
tar -czvf quake.tar.gz quake
popd
