#!/bin/bash

set -e

# shellcheck disable=SC2065
if ! test -f WinQuake/build-native/Quake >/dev/null 2>&1; then
    echo "error: WinQuake/build-native/Quake doesn't exist; have you run build-native.sh?"
    exit 1
fi

# shellcheck disable=SC2065
if ! test -f WinQuake/id1/pak0.pak >/dev/null 2>&1; then
    echo "error: WinQuake/id1/pak0.pak doesn't exist; have you put your Quake resources in WinQuake/id1?"
    exit 1
fi

pushd WinQuake >/dev/null

# shellcheck disable=SC2068
./build-native/Quake -width "${WIDTH:-1280}" -height "${HEIGHT:-800}" ${@}

popd >/dev/null
