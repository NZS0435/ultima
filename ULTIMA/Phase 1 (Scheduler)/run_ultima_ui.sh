#!/bin/sh
set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR" || exit 1

TRANSCRIPT_FILE="$SCRIPT_DIR/phase1output.txt"

if [ -t 1 ]; then
  printf '\033[8;62;140t'
  stty rows 62 cols 140 2>/dev/null || true
fi

if ! make -s build >/dev/null 2>&1; then
  printf 'Unable to rebuild Ultima before launch.\n'
  printf 'Run make in %s and try again.\n' "$SCRIPT_DIR"
  exit 1
fi

set +e
./Ultima --output-file "$TRANSCRIPT_FILE" "$@"
RUN_STATUS=$?
set -e

if [ -f "$TRANSCRIPT_FILE" ]; then
  printf '\nTranscript saved to:\n%s\n' "$TRANSCRIPT_FILE"
else
  printf '\nULTIMA finished with exit code %d, but no transcript file was written.\n' "$RUN_STATUS"
fi

exit "$RUN_STATUS"
