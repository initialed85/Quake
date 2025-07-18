#!/bin/bash

set -e

if ! command -v docker >/dev/null 2>&1; then
    echo "error: docker not found; have you installed it?"
    exit 1
fi

if ! docker image inspect initialed85/quake:latest >/dev/null 2>&1; then
    echo "error: initialed85/quake:latest not found; have you run build-docker.sh?"
    exit 1
fi

# shellcheck disable=SC2065
if ! test -f WinQuake/id1/pak0.pak >/dev/null 2>&1; then
    echo "error: WinQuake/id1/pak0.pak doesn't exist; have you put your Quake resources in WinQuake/id1?"
    exit 1
fi

detected_arch="amd64"
if [[ "$(uname -p)" == "arm" ]]; then
    detected_arch="arm64"
fi

desired_arch="${ARCH:-${detected_arch}}"

actual_arch="$(docker image inspect initialed85/quake:latest | grep Architecture | cut -d '"' -f4 | xargs)"

if [[ "${actual_arch}" != "${desired_arch}" ]]; then
    echo "error: initialed85/quake:latest was found but it's for ${actual_arch} and not ${desired_arch}; did you mean ARCH=${desired_arch} build-docker.sh or ARCH=${actual_arch} run-docker.sh?"
    exit 1
fi

pushd WinQuake >/dev/null

if [[ "${1}" == "" ]] || [[ "${1}" == "down" ]]; then
    function cleanup() {
        docker stop -t0 quake >/dev/null 2>&1 || true
        docker rm -f quake >/dev/null 2>&1 || true
    }

    trap cleanup EXIT
fi

if [[ "${1}" == "" ]] || [[ "${1}" == "up" ]]; then
    if [[ "${DEBUG}" == "1" ]]; then
        docker run --stop-timeout=0 --platform="linux/${desired_arch}" --privileged -d -p 11111:11111 -v "$(pwd)/id1:/srv/WinQuake/id1" --name quake --entrypoint lldb-server initialed85/quake:latest platform --listen "*:11111" --server
    else
        # docker run --stop-timeout=0 --platform="linux/${desired_arch}" --privileged -d --net=host -v "$(pwd)/id1:/srv/WinQuake/id1" --name quake initialed85/quake:latest
        docker run --stop-timeout=0 --platform="linux/${desired_arch}" --privileged --rm -it --net=host -v "$(pwd)/id1:/srv/WinQuake/id1" --name quake initialed85/quake:latest
    fi
fi

if [[ "${1}" == "" ]]; then
    docker logs -f -t quake || true
fi

popd >/dev/null
