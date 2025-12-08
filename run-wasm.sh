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

docker build -t quake-wasm-nginx -f ./Dockerfile.nginx .
docker run --rm -it -p 80:80 -v "$(pwd)/WinQuake/build-wasm:/usr/share/nginx/html" quake-wasm-nginx
