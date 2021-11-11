#!/bin/bash

git clone git://git.kernel.org/pub/scm/libs/libtrace/libtraceevent.git
git clone git://git.kernel.org/pub/scm/libs/libtrace/libtracefs.git
git clone git://git.kernel.org/pub/scm/utils/trace-cmd/trace-cmd.git

./build_libs.sh
