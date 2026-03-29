#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

export ULTIMA_OPENED_EXTERNAL_TERMINAL=1
TRANSCRIPT_FILE="$SCRIPT_DIR/phase1output.txt"

# Ask xterm-compatible terminals, including macOS Terminal, for a larger canvas.
printf '\033[8;62;140t'
sleep 0.2

make -s build >/dev/null
./Ultima --output-file "$TRANSCRIPT_FILE" "$@"
RUN_STATUS=$?

clear
if [[ -f "$TRANSCRIPT_FILE" ]]; then
  printf 'ULTIMA Phase 1 transcript saved to:\n%s\n\n' "$TRANSCRIPT_FILE"
  cat "$TRANSCRIPT_FILE"
  printf '\n'
else
  printf 'ULTIMA finished with exit code %d, but no transcript file was written.\n\n' "$RUN_STATUS"
fi

printf 'Press Enter to close this Terminal window...'
read -r
exit "$RUN_STATUS"
