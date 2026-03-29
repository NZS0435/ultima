#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TRANSCRIPT_FILE="$SCRIPT_DIR/phase1output.txt"

make -s build >/dev/null
./Ultima --transcript-only --output-file "$TRANSCRIPT_FILE"

printf '\nPress Enter to close this Terminal window...'
read -r
