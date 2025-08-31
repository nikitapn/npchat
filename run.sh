#!/bin/bash

set -e

USE_SSL=0
BUILD_TYPE=Debug

cmake -B .build_local -S . -DOPT_NPRPC_SKIP_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build .build_local

CMD=(
    .build_local/${BUILD_TYPE}/npchat
    --hostname archvm.lan
    --http-dir ./client/dist
    --data-dir ./sample_data
    --trace
)

if [ $USE_SSL -eq 1 ]; then
    CMD+=(
        --use-ssl
        --port 8443
        --public-cert certs/archvm.lan.crt
        --private-key certs/archvm.lan.key
        --dh-params certs/dhparam.pem
    )
else
    CMD+=(
        --port 8080
    )
fi

CMD=${CMD[@]}

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
    kill -SIGKILL $PID
    exit 0
}

wait $PID
echo "Server have stopped."
