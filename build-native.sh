#!/bin/bash

set -e

if ! command -v git >/dev/null 2>&1; then
    echo "error: git not found; have you installed it?"
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "error: cmake not found; have you installed it?"
    exit 1
fi

if ! command -v make >/dev/null 2>&1; then
    echo "error: make not found; have you installed it?"
    exit 1
fi

pushd WinQuake >/dev/null

if [[ "${CLEAN}" == "1" ]]; then
    rm -fr build-native 2>&1 || true
fi

mkdir -p build-native
cd build-native

if command -v clang-20 >/dev/null 2>&1; then
    export CC="$(which clang-20)"
else
    export CC="$(which clang)"
end

if command -v clang++-20 >/dev/null 2>&1; then
    export CXX="$(which clang++-20)"
else
    export CXX="$(which clang++)"
end

if [[ "${DEBUG}" == "1" ]]; then
    ASAN_OPTIONS=detect_leaks=1 cmake -Wno-dev -DCMAKE_BUILD_TYPE=Debug -B . -S ../
else
    cmake -Wno-dev -DCMAKE_BUILD_TYPE=Release -B . -S ../
fi

make

echo ""

find ./ -maxdepth 1 -type f -name 'Quake' || true
find ./ -maxdepth 1 -type f -name '*.dylib' || true
find ./ -maxdepth 1 -type f -name '*.so' || true
find ./ -maxdepth 1 -type f -name '*.js' || true
find ./ -maxdepth 1 -type f -name '*.html' || true

popd >/dev/null
