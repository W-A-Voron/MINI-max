#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
exec ./build/client/minimax-client -c "${1:-configs/client.toml}"
