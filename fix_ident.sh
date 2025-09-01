#!/bin/env bash
# Fix identation of files for the project

find . -type f \( \
  \( -path './*' -a ! -path './*/*' \) -o \
  -path './client/src/*' -o \( -path './client/*' -a ! -path './client/*/*' \) -o \
  -path './server/src/*' -o \
  -path './idl/*' \) \
  -type f \( \
    -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o \
    -name "*.ts" -o -name "*.js" -o -name "*.svelte" -o \
    -name "*.npidl" -o \
    -name "CMakeLists.txt" \
    \) \
  -exec echo "Fixing file: {}" \; \
  -exec sed -i 's/[[:space:]]\+$//' {} \; \
  -exec sed -i 's/\t/  /g' {} \;
