#!/bin/env bash

ldd /app/npchat | grep boost | awk '{ print $3 }' | xargs tar --absolute-names -czf /out/boost_runtime_libs.tar.gz
chown 1000:1000 /out/boost_runtime_libs.tar.gz