#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec /bin/sh "$SCRIPT_DIR/run_ultima_transcript.sh"
