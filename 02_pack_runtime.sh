#!/bin/env bash

set -e

BUILD_DIR=.build_docker
ROOT_DIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

cd $ROOT_DIR
mkdir -p artifacts/out

docker run --rm \
  -v $ROOT_DIR/${BUILD_DIR}/bin:/app \
  -v $ROOT_DIR/docker/scripts:/cmd \
  -v $ROOT_DIR/artifacts/out:/out \
  -w /cmd \
  cpp-dev-env:latest \
  ./collect_boost.sh

set -ex

cp ${BUILD_DIR}/bin/npchat                            artifacts/out
cp ${BUILD_DIR}/external/npsystem/nplib/libnplib.so   artifacts/out
cp ${BUILD_DIR}/external/npsystem/nprpc/libnprpc.so   artifacts/out
cp docker/Dockerfile.runtime                          artifacts/out

cd artifacts
tar -czf npchat-server.tar.gz out/

cp -r ../client/dist www
tar -czhf npchat-client.tar.gz www/

cp -r ../sample_data data
tar -czhf npchat-data.tar.gz data/