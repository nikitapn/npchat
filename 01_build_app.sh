#!/bin/env bash

set -e

BUILD_DIR=.build_docker
ROOT_DIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

cd $ROOT_DIR

docker run --rm \
  -v $ROOT_DIR:/app \
  -v $(readlink -f external/npsystem):/app/external/npsystem \
  -w /app cpp-dev-env:latest \
  cmake -G "Ninja" -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Release \
    -DBOOST_LIB_PREFIX=/usr/local/lib \
    -DOPT_NPRPC_SKIP_TESTS=ON

docker run --rm \
  -v $ROOT_DIR:/app \
  -v $(readlink -f external/npsystem):/app/external/npsystem \
  -w /app cpp-dev-env:latest \
  cmake --build $BUILD_DIR -j$(nproc)
