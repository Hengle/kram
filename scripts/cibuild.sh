#!/bin/bash

# note: zsh works on TrivisCI osx, but not on Win git bash, so using bassh

# osx, windows, linux
buildType=$1

set -x

# create directories to install results
mkdir -p bin
mkdir -p build

pushd build

# need cmake 3.19 for universal apps, but vms have 3.18
# and 3.18 for Win, but machines only have 3.18 and 3.17 installed

# this is done in travis
#if [[ $buildType == osx ]]; then brew upgrade cmake; fi

if [[ $buildType == windows ]]; then choco upgrade cmake; fi

# TODO: do a curl to update cmake
#if [[ $buildType == linux ]]; then docurl...; fi



# can't just use cmake .. on osx, cmake gets confused about metal files since language not recognized
# but Xcode knows how to build these.  I don't to setup special rule for metal files right now.
if [[ $buildType == osx ]]; then
	cmake .. -G Xcode
elif [[ $buildType == windows ]]; then
	cmake .. -G "Visual Studio 15 2017" -A x64
elif [[ $buildType == linux ]]; then
	cmake .. 
fi

cmake --build . --config Release
cmake --install . --config Release

popd
