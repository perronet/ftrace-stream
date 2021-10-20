#!/bin/bash

# To test the script use INSTALL_PATH=/tmp/install

git clone git://git.kernel.org/pub/scm/libs/libtrace/libtraceevent.git
git clone git://git.kernel.org/pub/scm/libs/libtrace/libtracefs.git
git clone git://git.kernel.org/pub/scm/utils/trace-cmd/trace-cmd.git

make -C libtraceevent clean
make -C libtracefs clean
make -C trace-cmd clean

cd libtraceevent
PREFIX=/usr/local INSTALL_PATH=/ ../trace-cmd/make-trace-cmd.sh install

cd ../libtracefs
PREFIX=/usr/local INSTALL_PATH=/ ../trace-cmd/make-trace-cmd.sh install

cd ../trace-cmd
PREFIX=/usr/local INSTALL_PATH=/ ./make-trace-cmd.sh install_libs

# Replace the trace-cmd.h header with a custom header.
# The reason for this is that the standard header does not expose
# some of the functions that we need in the binary parser.
cd ..
cp trace-cmd-private.h /usr/local/include/trace-cmd/trace-cmd.h

make -C libtraceevent clean
make -C libtracefs clean
make -C trace-cmd clean

sudo ldconfig
