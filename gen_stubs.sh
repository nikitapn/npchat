#!/bin/env bash
# Script to generate RPC stubs and copy them to the client source directory

source common.shi

cmake --build .build_local/${BUILD_TYPE} --target gen_stubs