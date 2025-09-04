#!/bin/bash

set -e

source common.shi

if [[ -z "$1" || "$1" == "run" ]]; then
    cmake --build $BUILD_DIR -j$(nproc)
elif [ "$1" == "server" ]; then
    cmake --build $BUILD_DIR --target npchat -j$(nproc)
elif [ "$1" == "client" ]; then
    cmake --build $BUILD_DIR --target npchat_js -j$(nproc)
fi

[ ! "$1" == "run" ] && exit 0

CMD=(
    ${BUILD_DIR}/bin/npchat
    --hostname archvm.lan
    --http-dir ./client/dist
    --data-dir ./sample_data
    --trace
)

if [ $USE_SSL -eq 1 ]; then
    CMD+=(
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
