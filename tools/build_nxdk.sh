#!/usr/bin/env bash
set -euo pipefail

NXDK_DIR="${NXDK_DIR:-/c/nxdk}"
LLVM_BIN="${LLVM_BIN:-/c/Program Files/LLVM/bin}"

export NXDK_DIR
export PATH="${LLVM_BIN}:${NXDK_DIR}/bin:/usr/bin:/mingw64/bin:${PATH}"

exec make "$@"
