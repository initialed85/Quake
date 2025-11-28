# Quake

This repo contains my attempt to modernise the Quake 1 source code.

## Goals

- Aim to keep the experience primarily vanilla with a few tasteful improvements
- Get the code building as a CMake project
- Use SDL2 as the system platform abstraction
- Provide build tooling for macOS, Linux and WASM (sorry about it Windows)
- Fix the NAT-related issues in the original UDP net code
- Implement WebSocket multiplayer for the WASM build (via a proxy / gateway server)

## Tasks

- [DONE] Get the code building as a CMake project
  - [DONE] Get a skeleton CMake project working w/ `sys_null.c` etc
    - [DONE] Fix various variable name clashes etc when throwing everything into a single `LibQuakeLib`
      - [TODO] Figure out if this is okay- the original build tooling clearly made it all work somehow
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
- [DONE] Fix various bugs stumbled across
  - [DONE] Fix pointer math segfault loading maps in `common.c::COM_FileBase`
  - [DONE] Fix pointer math segfault loading maps in `pr_exec.c::PR_ExecuteProgram::OP_EQ_S`
  - [DONE] Fix null pointer caused (I think) by late init of SDL2 audio under WASM in
    `snd_mix.c::Snd_WriteLinearBlastStereo16`
  - [DONE] Fix null pointer to do with server setup in `sv_main.c::SV_ModelIndex`
  - [DONE] Fix pointer math segfault to do with warping and odd screen sizes caused by `r_shared.h::MAXHEIGHT`
    and `r_shared.h::MAXWIDTH`
  - [DONE] Fix whatever the pointer hell was going on with `pr_strings`
- [DONE] Fix the NAT-related issues in the original UDP net code
  - You can control the port your client will use with the `-port` flag; e.g. if you wanted to run a server and 2 clients all on the same machine:
    - `./Quake -port 26000 +map start`
    - `./Quake -port 26001 +connect 127.0.0.1:26000`
    - `./Quake -port 26002  +connect 127.0.0.1:26000`
- [TODO] Implement WebSocket multiplayer for the WASM build (via a proxy / gateway server)
- [TODO] Fix crash encountered with `-width 2560 -height 1440 -fullscreen` and maybe higher resolutions

## Usage

There are similar build scripts for 3 different build variants:

- Native
  - Build for whatever your platform is (as long as it's macOS or Linux)
  - Build output is at `WinQuake/build-native`
- WASM
  - Build for running in a browser
  - Build output is at `WinQuake/build-wasm`
- Docker
  - Build for Linux, using Docker
  - Build output is at `WinQuake/build-docker`

NOTE: You'll need to place your demo / purchased Quake resources at `WinQuake/id1` to be able to run Quake.

### Native

```shell
# build once
./build-native.sh

# build any time code changes
./watch-build-native.sh

# run in a 1280x800 window (i.e. cd WinQuake && ./build-native/WinQuake -width 1280 -height 800)
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

# run in a Docker container (i.e. cd WinQuake && ./build-native/WinQuake -dedicated 16, in a Docker container)
./run-docker.sh
```

## Original documentation

See [original README](./readme.txt) and [original LICENCE](./gnu.txt)
