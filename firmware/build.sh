#!/bin/bash

# if it doesn't already exist, create build folder
if [ ! -d build ]; then
  mkdir build
fi
cd build

# if Makefile does not already exist, configure the build folder
if [ ! -f Makefile ]; then
    cmake ..
fi

# now make the application
make
