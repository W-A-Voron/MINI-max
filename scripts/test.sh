#!/usr/bin/env bash
# Run unit + integration tests (headless-safe).
set -euo pipefail
cd "$(dirname "$0")/.."
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
