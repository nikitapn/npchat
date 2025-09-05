#/bin/env bash

set -e

ROOT_DIR=$(dirname $(readlink -e ${BASH_SOURCE[0]}))

# Build docker image for runtime, this also copies all necessary C++ libs and excutables from artifacts/out
docker build -t cpp-runtime-env -f docker/Dockerfile.runtime artifacts/out

# Remove old container if exists
docker rm -f npchat &> /dev/null

mkdir -p artifacts/certs
cp -n certs/* artifacts/certs/ || true

# Run npchat server
docker run -d \
  --name npchat \
  -v ${ROOT_DIR}/artifacts:/app \
  -w /app \
  -p 8443:8443 \
  cpp-runtime-env \
  npchat \
    --hostname archvm.lan \
    --http-dir www \
    --data-dir data \
    --port 8443 \
    --public-cert certs/archvm.lan.crt \
    --private-key certs/archvm.lan.key \
    --dh-params certs/dhparam.pem

