#!/bin/bash
set -e

if [ $# -ne 1 ]; then
  echo "Usage: $0 <path-to-server.cpp>"
  exit 1
fi

SRC="$1"
OUT="out/server"

echo "Compiling $SRC..."
g++ -Wall -Wextra -O2 -o "$OUT" "$SRC"

echo "Running $OUT..."
./"$OUT"