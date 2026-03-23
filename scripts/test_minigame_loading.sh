#!/usr/bin/env bash
# Quick smoke test: launch each minigame and verify it doesn't crash.
# Usage: ./scripts/test_minigame_loading.sh [build_dir] [game]
#   build_dir: path to build directory (default: build)
#   game:      game name (default: doom2rpg)

set -euo pipefail

BUILD_DIR="${1:-build}"
GAME="${2:-doom2rpg}"
BINARY="$BUILD_DIR/src/DoomIIRPG"
TIMEOUT_SEC=3
PASSED=0
FAILED=0
ERRORS=""

MINIGAMES=("hacking" "sentrybot" "vending")

if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Build the project first: cmake --build $BUILD_DIR"
    exit 1
fi

echo "=== Minigame Loading Test ==="
echo "Binary:    $BINARY"
echo "Game:      $GAME"
echo "Minigames: ${#MINIGAMES[@]}"
echo "Timeout:   ${TIMEOUT_SEC}s each"
echo ""

for MG in "${MINIGAMES[@]}"; do
    printf "  %-15s ... " "$MG"

    LOG=$(mktemp)
    set +e

    # Launch game in background, kill after timeout
    "$BINARY" --game "$GAME" --minigame "$MG" >"$LOG" 2>&1 &
    PID=$!

    # Wait up to TIMEOUT_SEC for the process
    ELAPSED=0
    while kill -0 "$PID" 2>/dev/null && [[ $ELAPSED -lt $TIMEOUT_SEC ]]; do
        sleep 1
        ELAPSED=$((ELAPSED + 1))
    done

    if kill -0 "$PID" 2>/dev/null; then
        # Still running after timeout = minigame loaded fine, kill it
        kill "$PID" 2>/dev/null
        wait "$PID" 2>/dev/null
        echo "OK"
        PASSED=$((PASSED + 1))
    else
        # Process ended on its own — check exit code
        wait "$PID" 2>/dev/null
        EXIT_CODE=$?
        if [[ $EXIT_CODE -eq 0 ]]; then
            echo "OK (exited)"
            PASSED=$((PASSED + 1))
        elif [[ $EXIT_CODE -gt 128 ]]; then
            SIG=$((EXIT_CODE - 128))
            echo "CRASH (signal $SIG)"
            FAILED=$((FAILED + 1))
            ERRORS="$ERRORS  $MG (signal $SIG)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        else
            echo "FAIL (exit $EXIT_CODE)"
            FAILED=$((FAILED + 1))
            ERRORS="$ERRORS  $MG (exit $EXIT_CODE)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        fi
    fi

    set -e
    rm -f "$LOG"
done

echo ""
echo "=== Results: $PASSED passed, $FAILED failed (${#MINIGAMES[@]} total) ==="

if [[ -n "$ERRORS" ]]; then
    echo ""
    echo "Failed minigames:"
    printf "$ERRORS"
    exit 1
fi

exit 0
