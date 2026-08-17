# Quake

This repo contains my attempt to modernise the Quake 1 source code.

If it's working (mileage varies) you can play it right now at [quake.initialed85.cc](https://quake.initialed85.cc); I'm running
a few dedicated servers, but you can also run your own to play against others (standard old school Quake listen server style)
right from your browser.

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
      - [TODO] Confirm the single-library CMake organization is the right long-term structure
- [DONE] Use SDL2 as the system platform abstraction
  - [DONE] Work out idiomatic way to include SDL2
  - [DONE] Implement video w/ SDL2
  - [DONE] Implement keyboard w/ SDL2
  - [DONE] Implement mouse w/ SDL2
  - [DONE] Scale browser mouse input for display resolution and device pixel ratio
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
    - [DONE] Fix it without just spamming `ED_NewString` everywhere (causing leaks)
  - [DONE] Fix high-resolution software rendering and underwater warp pixelation
  - [DONE] Add optional lower internal render resolution with bilinear upscaling via `-internalwidth` / `-internalheight`
  - [DONE] Fix Wayland fullscreen cursor grabbing
  - [DONE] Avoid rebuilding the particle blend table on every screen flash
  - [DONE] Smooth WASM step-entity movement between WebSocket snapshots
- [DONE] Fix the NAT-related issues in the original UDP net code
  - You can control the port your client will use with the `-port` flag; e.g. if you wanted to run a server and 2 clients all on the same machine:
    - `./Quake -port 26000 +map start`
    - `./Quake -port 26001 +connect 127.0.0.1:26000`
    - `./Quake -port 26002  +connect 127.0.0.1:26000`
- [DONE] Implement WebSocket multiplayer for the WASM build (via a proxy / gateway server)
  - [DONE] Fix browser LAN/server-discovery broadcasts being delivered to only one matching endpoint
  - [DONE] Add regression tests for WebSocket broadcast fan-out
- [DONE] Fix Linux Docker builds and package the runtime SDL2 dependency
- [DONE] Fix server search not finding all virtual-LAN servers across matching ports
- [DONE] Allow a server/browser entrypoint to provision an initial number of FrikBots via the `botcount` cvar (using the engine's `maxclients` capacity)
- [TODO] Fix FrikBot occasionally entering a state where it will not attack (possibly related to runaway-loop protection)
- [DONE] Add a safe range check for the custom `botcount` cvar; excessive bot counts are clamped to available slots
- [DONE] Make `maxplayers 32` use the full supported scoreboard range
- [DONE] Keep long scoreboard names from overflowing the legacy text buffer
- [DONE] Fix botcount overriding manual bot impulses and guard botcount reconciliation against full servers
- [DONE] Fix bot slot collision handling, slot-zero handling, and occupancy-mask precedence
- [TODO] Fix crashes when too many players or bots try to connect / the QuakeC VM runs out of room

## Usage

There are similar build scripts for 3 different build variants:

Native builds require a system SDL2 development package discoverable by CMake (for example, `libsdl2-dev` on Debian/Ubuntu or `sdl2` on Arch).

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

# run in a 1280x800 window
./run-native.sh
```

### WASM

```shell
# build once
./build-wasm.sh

# build any time code changes
./watch-build-wasm.sh

# run at http://localhost:80 (play using a browser)
./run-wasm.sh
```

Browser mouse input is automatically adjusted using the current display
resolution, device pixel ratio, and effective Quake render resolution.

### Docker

```shell
# build once
./build-docker.sh

# build any time code changes
./watch-build-docker.sh

# run a dedicated Linux server in a Docker container
./run-docker.sh
```

## Original documentation

See [original README](./readme.txt) and [original LICENCE](./gnu.txt)

## Dev scratch notes

```shell
# for deploying Docker-built Linux Quake server to Kubernetes
ARCH=amd64 ./build-docker.sh && PUSH=1 ./package-docker.sh

# for deploying the WASM Quake server and its browser gateway to Kubernetes
ARCH=amd64 ./build-wasm.sh && ARCH=amd64 PUSH=1 ./package-wasm.sh
```
