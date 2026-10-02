#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
exec ./build/server/minimax-server -c "${1:-configs/server.toml}"
