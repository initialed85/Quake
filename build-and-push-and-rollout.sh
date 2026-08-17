#!/bin/bash

set -e

ARCH=amd64 ./build-wasm.sh

ARCH=amd64 PUSH=1 ./package-wasm.sh

kubectl --context home-prod -n quake rollout restart deployment

kubectl --context home-prod -n quake rollout restart statefulset/quake-ws-server

kubectl --context home-prod -n quake rollout restart statefulset.apps/quake-wasm-node-coop-easy statefulset.apps/quake-wasm-node-coop-hard statefulset.apps/quake-wasm-node-coop-medium statefulset.apps/quake-wasm-node-coop-nightmare statefulset.apps/quake-wasm-node-deathmatch-dm1 statefulset.apps/quake-wasm-node-deathmatch-dm2 statefulset.apps/quake-wasm-node-deathmatch-dm3 statefulset.apps/quake-wasm-node-deathmatch-dm4 statefulset.apps/quake-wasm-node-deathmatch-dm5 statefulset.apps/quake-wasm-node-deathmatch-dm6
