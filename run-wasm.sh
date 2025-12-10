#!/bin/bash

set -e

# shellcheck disable=SC2065
if ! test -f WinQuake/build-wasm/Quake.js >/dev/null 2>&1; then
	echo "error: WinQuake/build-wasm/Quake.js doesn't exist; have you run build-wasm.sh?"
	exit 1
fi

docker build -t quake-wasm-nginx -f ./Dockerfile.nginx .
docker run --rm -it -p 80:80 -v "$(pwd)/WinQuake/build-wasm:/usr/share/nginx/html" -v "$(pwd)/WinQuake:/usr/share/nginx/html/WinQuake" quake-wasm-nginx
