#!/bin/env bash

source common.shi
cmake -B $BUILD_DIR -S . \
  -DOPT_NPRPC_SKIP_TESTS=ON \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE