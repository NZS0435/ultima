#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export ULTIMA_OPENED_EXTERNAL_TERMINAL=1
exec /bin/sh "$SCRIPT_DIR/run_ultima_ui.sh" "$@"
