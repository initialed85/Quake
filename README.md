# Quake

This repo contains my attempt to modernise the Quake 1 source code.

## Goals

- Aim to keep the experience primarily vanilla with a few tasteful improvements
- Get the code building as a CMakeLists project
- Use SDL2 as the system platform abstraction
- Provide build tooling for macOS, Linux and WASM (sorry about it Windows)
- Fix the NAT-related issues in the original UDP net code
- Implement WebSocket multiplayer for the WASM build (via a proxy / gateway server)

## Tasks

- [DONE] Get the code building as a CMakeLists project
  - [DONE] Get a skeleton CMakeLists project working w/ `sys_null.c` etc
- [DONE] Use SDL2 as the system platform abstraction
  - [DONE] Work out idiomatic way to include SDL2
  - [DONE] Implement video w/ SDL2
  - [DONE] Implement keyboard w/ SDL2
  - [DONE] Implement mouse w/ SDL2
  - [DONE] Implement audio w/ SDL2
- [DONE] Provide build tooling for macOS, Linux and WASM (sorry about it Windows)
  - [DONE] Make Bash scripts for native (macOS / Linux) builds
  - [DONE] Make Bash scripts for WASM builds
    - [DONE] Various fixes to get WASM builds working
  - [DONE] Make Bash scripts for Docker (Linux) builds
- [TODO] Fix the NAT-related issues in the original UDP net code
- [TODO] Implement WebSocket multiplayer for the WASM build (via a proxy / gateway server)

## Usage

Env vars:

- `DEBUG=0` or `DEBUG=1`
  - Disable / enable symbols, sanitizers etc- slow but helpful
  - (Docker builds only) Disable / enable the LLDB server for remote debugging

- `ARCH=amd64` or `ARCH=amd64`
  - (Docker builds only) Specify target architecture

### Native

```shell
# build once
./build-native.sh

# build any time code changes
./watch-build-native.sh

# run in a 1280x800 window
./run-native.sh
```

### WASM

```shell
# build once
./build-wasm.sh

# build any time code changes
./watch-build-wasm.sh

# run at https://localhost:8443 (play using a browser)
./run-wasm.sh
```

### Docker

```shell
# build once
./build-docker.sh

# build any time code changes
./watch-build-docker.sh

# run in a Docker container (dedicated mode only)
./run-docker.sh
```

## Original documentation

See [original README](./readme.txt) and [original LICENCE](./gnu.txt)
