#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

export ULTIMA_OPENED_EXTERNAL_TERMINAL=1

make build
exec ./Ultima "$@"
