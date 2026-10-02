#!/usr/bin/env bash
# Apply SQL migrations:  scripts/migrate.sh postgresql://user:pass@host/db
set -euo pipefail
cd "$(dirname "$0")/.."
: "${1:?usage: migrate.sh <postgres-url>}"
for f in server/migrations/*.sql; do echo "applying $f"; psql "$1" -v ON_ERROR_STOP=1 -f "$f"; done
