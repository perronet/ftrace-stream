#!/bin/bash

# To test the script use INSTALL_PATH=/tmp/install

mkdir -p build_output

# Clean previous output
make -C libtraceevent clean
make -C libtracefs clean
make -C trace-cmd clean
rm -rf build_output/*

CUSTOM_PATH=$(pwd)/build_output

cd libtraceevent
INSTALL_PATH="$CUSTOM_PATH" ../trace-cmd/make-trace-cmd.sh install

cd ../libtracefs
INSTALL_PATH="$CUSTOM_PATH" ../trace-cmd/make-trace-cmd.sh install

cd ../trace-cmd
INSTALL_PATH="$CUSTOM_PATH" ./make-trace-cmd.sh install_libs

cd ..
make -C libtraceevent clean
make -C libtracefs clean
make -C trace-cmd clean
