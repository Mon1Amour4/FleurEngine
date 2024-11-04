#!/bin/bash

rootFolder="$(pwd)"
buildDir="$rootFolder/build/macos"

if [ ! -d "$buildDir" ]; then
    mkdir -p "$buildDir"
fi

cd "$buildDir" || exit

cmake -S "$rootFolder" -B . -G "Xcode"
cmake --build .

read -rp "Press Enter to close the window"
