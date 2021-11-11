#!/bin/bash

# Without LD_LIBRARY_PATH we won't be able to find trace-cmd libraries at runtime
sudo LD_LIBRARY_PATH=src/binary_parser/lib/build_output/usr/lib64 ./target/debug/ftrace-stream
