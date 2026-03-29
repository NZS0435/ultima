#!/bin/zsh
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

TRANSCRIPT_FILE="$SCRIPT_DIR/phase1output.txt"

if ! make -s build >/dev/null 2>&1; then
  printf 'Unable to rebuild Ultima before launch.\n'
  printf 'Run make in %s and try again.\n' "$SCRIPT_DIR"
  exit 1
fi
set +e
./Ultima --transcript-only --output-file "$TRANSCRIPT_FILE"
RUN_STATUS=$?
set -e

if [[ -f "$TRANSCRIPT_FILE" ]]; then
  printf '\nTranscript saved to:\n%s\n' "$TRANSCRIPT_FILE"
else
  printf '\nULTIMA finished with exit code %d, but no transcript file was written.\n' "$RUN_STATUS"
fi

printf '\nPress Enter to close this Terminal window...'
read -r
exit "$RUN_STATUS"
