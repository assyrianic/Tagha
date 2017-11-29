#!/bin/bash
cd "$(dirname "$0")"
./build_libtagha_static.sh
./build_libtagha_shared.sh
./build_hostapp.sh
