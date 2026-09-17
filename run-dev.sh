#!/bin/sh
# Dev launcher for OpenAuto (wired Android Auto, PC x86).
# Build first from this directory:
#   export AASDK_PATH=$HOME/dev/aa-headunit/aasdk
#   mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)
set -e
cd "$(dirname "$0")"
export AASDK_PATH="$HOME/dev/aa-headunit/aasdk"
export QT_QPA_PLATFORM=xcb
# aasdk has no system install: expose its libs at runtime.
export LD_LIBRARY_PATH="$AASDK_PATH/lib:${LD_LIBRARY_PATH:-}"
exec ./bin/autoapp "$@"
