#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "${ROOT}" -B "${ROOT}/build/host" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${ROOT}/build/host" --parallel
