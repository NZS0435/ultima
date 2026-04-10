#!/bin/sh
set -u

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR" || exit 1

TRANSCRIPT_FILE="$SCRIPT_DIR/phase1output.txt"
ULTIMA_BINARY=

if [ -x "$SCRIPT_DIR/ultima_os" ]; then
  ULTIMA_BINARY="$SCRIPT_DIR/ultima_os"
elif [ -x "$SCRIPT_DIR/Ultima" ]; then
  ULTIMA_BINARY="$SCRIPT_DIR/Ultima"
fi

if [ -f "$SCRIPT_DIR/Makefile" ]; then
  if ! make -s build >/dev/null 2>&1; then
    printf 'Unable to rebuild Ultima before launch.\n'
    printf 'Run make in %s and try again.\n' "$SCRIPT_DIR"
    exit 1
  fi

  if [ -x "$SCRIPT_DIR/ultima_os" ]; then
    ULTIMA_BINARY="$SCRIPT_DIR/ultima_os"
  elif [ -x "$SCRIPT_DIR/Ultima" ]; then
    ULTIMA_BINARY="$SCRIPT_DIR/Ultima"
  fi
fi

if [ -z "$ULTIMA_BINARY" ]; then
  printf 'Unable to locate an Ultima executable in %s.\n' "$SCRIPT_DIR"
  exit 1
fi

rm -f "$TRANSCRIPT_FILE"

set +e
"$ULTIMA_BINARY" --transcript-only --output-file "$TRANSCRIPT_FILE" "$@"
RUN_STATUS=$?
set -e

if [ "$RUN_STATUS" -eq 0 ] && [ -f "$TRANSCRIPT_FILE" ]; then
  printf '\nTranscript saved to:\n%s\n' "$TRANSCRIPT_FILE"
else
  printf '\nULTIMA finished with exit code %d, but no transcript file was written.\n' "$RUN_STATUS"
fi

exit "$RUN_STATUS"
