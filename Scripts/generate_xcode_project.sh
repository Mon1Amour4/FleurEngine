#!/bin/bash

current_dir=$(pwd)
cd "$(dirname "$0")" || exit
platform="macos"
enable_tests="true"
build_dll="false"
python3 ./generate_project.py $platform $enable_tests $build_dll
cd "$current_dir"
