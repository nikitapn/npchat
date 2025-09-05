#!/bin/env bash

set -e

BUILD_DIR=.build
ROOT_DIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

cd $ROOT_DIR
mkdir -p runtime/out

docker run --rm \
  -v $ROOT_DIR/${BUILD_DIR}/bin:/app \
  -v $ROOT_DIR/docker/scripts:/cmd \
  -v $ROOT_DIR/runtime/out:/runtime \
  -w /cmd \
  cpp-dev-env:latest \
  ./collect_boost.sh

set -ex

cp ${BUILD_DIR}/release/npchat runtime/out
cp ${BUILD_DIR}/external/npsystem/nplib/libnplib.so runtime/out
cp ${BUILD_DIR}/external/npsystem/nprpc/libnprpc.so runtime/out
cp docker/Dockerfile.runtime runtime/out

cd runtime
tar -czf npchat-server.tar.gz out/

[ ! -L "www" ] && ln -s ../client/public www
tar -czhf npchat-client.tar.gz www/

[ ! -L "data" ] && ln -s ../sample_data data
tar -czhf npchat-data.tar.gz data/