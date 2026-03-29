#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

export ULTIMA_OPENED_EXTERNAL_TERMINAL=1

# Ask xterm-compatible terminals, including macOS Terminal, for a larger canvas.
printf '\033[8;62;140t'
sleep 0.2

make build
exec ./Ultima "$@"
