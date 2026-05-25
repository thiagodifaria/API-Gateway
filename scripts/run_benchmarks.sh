#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$ROOT_DIR/scripts/build.sh" Release

cmake --build "$ROOT_DIR/build" --config Release --target benchmark_aes benchmark_sha256 benchmark_p256

run_binary() {
  local binary="$1"
  if [[ -x "$binary" ]]; then
    "$binary"
  elif [[ -x "$binary.exe" ]]; then
    "$binary.exe"
  else
    echo "Binary not found: $binary" >&2
    exit 1
  fi
}

run_binary "$ROOT_DIR/build/benchmark_aes"
run_binary "$ROOT_DIR/build/benchmark_sha256"
run_binary "$ROOT_DIR/build/benchmark_p256"
