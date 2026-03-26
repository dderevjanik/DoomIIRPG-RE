#!/usr/bin/env bash
# Quick smoke test: launch the game with each map and verify it doesn't crash.
# Usage: ./scripts/test_map_loading.sh [build_dir] [game]
#   build_dir: path to build directory (default: build)
#   game:      game name (default: doom2rpg)

set -euo pipefail

HEADLESS=""
POSITIONAL=()
for arg in "$@"; do
    case "$arg" in
        --headless) HEADLESS="--headless" ;;
        *) POSITIONAL+=("$arg") ;;
    esac
done

BUILD_DIR="${POSITIONAL[0]:-build}"
GAME="${POSITIONAL[1]:-doom2rpg}"
BINARY="$BUILD_DIR/src/DoomIIRPG"
TIMEOUT_SEC=3
PASSED=0
FAILED=0
ERRORS=""

if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Build the project first: cmake --build $BUILD_DIR"
    exit 1
fi

# Find map files
MAP_DIR="games/$GAME/levels/maps"
if [[ ! -d "$MAP_DIR" ]]; then
    echo "ERROR: Map directory not found: $MAP_DIR"
    exit 1
fi

MAPS=($(ls "$MAP_DIR"/map*.bin 2>/dev/null | sort))
if [[ ${#MAPS[@]} -eq 0 ]]; then
    echo "ERROR: No map files found in $MAP_DIR"
    exit 1
fi

echo "=== Map Loading Test ==="
echo "Binary:  $BINARY"
echo "Game:    $GAME"
echo "Maps:    ${#MAPS[@]} found"
echo "Timeout: ${TIMEOUT_SEC}s per map"
echo ""

for MAP_PATH in "${MAPS[@]}"; do
    MAP_NAME=$(basename "$MAP_PATH")
    printf "  %-15s ... " "$MAP_NAME"

    LOG=$(mktemp)
    set +e

    # Launch game in background, kill after timeout
    "$BINARY" --game "$GAME" --skip-travel-map $HEADLESS --map "$MAP_NAME" >"$LOG" 2>&1 &
    PID=$!

    # Wait up to TIMEOUT_SEC for the process
    ELAPSED=0
    while kill -0 "$PID" 2>/dev/null && [[ $ELAPSED -lt $TIMEOUT_SEC ]]; do
        sleep 1
        ELAPSED=$((ELAPSED + 1))
    done

    if kill -0 "$PID" 2>/dev/null; then
        # Still running after timeout = map loaded fine, kill it
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
            ERRORS="$ERRORS  $MAP_NAME (signal $SIG)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        else
            echo "FAIL (exit $EXIT_CODE)"
            FAILED=$((FAILED + 1))
            ERRORS="$ERRORS  $MAP_NAME (exit $EXIT_CODE)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        fi
    fi

    set -e
    rm -f "$LOG"
done

echo ""
echo "=== Results: $PASSED passed, $FAILED failed (${#MAPS[@]} total) ==="

if [[ -n "$ERRORS" ]]; then
    echo ""
    echo "Failed maps:"
    printf "$ERRORS"
    exit 1
fi

exit 0
