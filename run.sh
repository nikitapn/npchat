#!/bin/bash

set -e

BUILD_TYPE=Debug

cmake -B .build_local -S . -DOPT_NPRPC_SKIP_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build .build_local

CMD=".build_local/${BUILD_TYPE}/npchat \
    --hostname archvm.lan \
    --port 8443 \
    --http-dir ./client/dist \
    --data-dir ./sample_data \
    --public-cert certs/archvm.lan.crt \
    --private-key certs/archvm.lan.key \
    --dh-params certs/dhparam.pem \
    --trace"

if [ "$1" == "debug" ]; then
    CMD="gdb --args $CMD"
fi

$CMD &
PID=$!
echo "Server is running with PID $PID"

# ./.build_local/debug/proxy_client &
# PROXY_PID=$!
# echo "Proxy client is running with PID $PROXY_PID"

trap ctrl_c INT
ctrl_c() {
    echo "Stopping Server with PID $PID..."
    kill $PID
    exit 0
}

wait $PID
echo "Server have stopped."
