#!/bin/bash

set -e

# shellcheck disable=SC2065
if ! test -f WinQuake/build-wasm/Quake.js >/dev/null 2>&1; then
    echo "error: WinQuake/build-wasm/Quake.js doesn't exist; have you run build-wasm.sh?"
    exit 1
fi

# shellcheck disable=SC2065
if ! test -f WinQuake/id1/pak0.pak >/dev/null 2>&1; then
    echo "error: WinQuake/id1/pak0.pak doesn't exist; have you put your Quake resources in WinQuake/id1?"
    exit 1
fi

if ! command -v mkcert >/dev/null 2>&1; then
    echo "error: mkcert not found; have you installed it? (using homebrew or similar)"
    exit 1
fi

if ! command -v http-server >/dev/null 2>&1; then
    echo "error: http-server not found; have you installed? (using npm)"
    exit 1
fi

pushd WinQuake >/dev/null

pushd build-wasm >/dev/null

rm -fv ca.crt ca.key cert.crt cert.key >/dev/null 2>&1 || true
mkcert create-ca
mkcert create-cert

http-server -p 8443 -a 0.0.0.0 -g -b -c-1 -t0 -S -C cert.crt -K cert.key

popd >/dev/null
