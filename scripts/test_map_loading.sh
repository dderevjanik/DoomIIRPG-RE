#!/usr/bin/env bash
# Quick smoke test: launch the game with each level and verify it doesn't crash.
# Discovers levels from game.yaml or falls back to levels/maps/map*.bin.
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
BINARY="$BUILD_DIR/src/DRPGEngine"
TIMEOUT_SEC=3
PASSED=0
FAILED=0
ERRORS=""

if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Build the project first: cmake --build $BUILD_DIR"
    exit 1
fi

GAME_DIR="games/$GAME"
GAME_YAML="$GAME_DIR/game.yaml"

# Discover level directories from game.yaml
LEVELS=()
if [[ -f "$GAME_YAML" ]]; then
    # Parse level directories from game.yaml (lines like "  N: levels/dir_name")
    while IFS= read -r line; do
        # Match "  <number>: <path>" under the levels: section
        dir=$(echo "$line" | sed -n 's/^[[:space:]]*[0-9]\+:[[:space:]]*//p')
        if [[ -n "$dir" && -d "$GAME_DIR/$dir" ]]; then
            LEVELS+=("$dir")
        fi
    done < <(sed -n '/^[[:space:]]*levels:/,/^[[:space:]]*[a-z_]*:/{ /^[[:space:]]*[0-9]/p }' "$GAME_YAML")
fi

# Fall back to legacy map*.bin files
if [[ ${#LEVELS[@]} -eq 0 ]]; then
    MAP_DIR="$GAME_DIR/levels/maps"
    if [[ ! -d "$MAP_DIR" ]]; then
        echo "ERROR: No levels found in $GAME_YAML and no map directory: $MAP_DIR"
        exit 1
    fi
    for f in "$MAP_DIR"/map*.bin; do
        [[ -f "$f" ]] && LEVELS+=("levels/maps/$(basename "$f")")
    done
fi

if [[ ${#LEVELS[@]} -eq 0 ]]; then
    echo "ERROR: No levels found for game $GAME"
    exit 1
fi

echo "=== Map Loading Test ==="
echo "Binary:  $BINARY"
echo "Game:    $GAME"
echo "Levels:  ${#LEVELS[@]} found"
echo "Timeout: ${TIMEOUT_SEC}s per level"
echo ""

for LEVEL in "${LEVELS[@]}"; do
    LEVEL_NAME=$(basename "$LEVEL")
    printf "  %-25s ... " "$LEVEL_NAME"

    LOG=$(mktemp)
    set +e

    # Launch game in background, kill after timeout
    "$BINARY" --game "$GAME" $HEADLESS --map "$LEVEL" >"$LOG" 2>&1 &
    PID=$!

    # Wait up to TIMEOUT_SEC for the process
    ELAPSED=0
    while kill -0 "$PID" 2>/dev/null && [[ $ELAPSED -lt $TIMEOUT_SEC ]]; do
        sleep 1
        ELAPSED=$((ELAPSED + 1))
    done

    if kill -0 "$PID" 2>/dev/null; then
        # Still running after timeout = level loaded fine, kill it
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
            ERRORS="$ERRORS  $LEVEL_NAME (signal $SIG)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        else
            echo "FAIL (exit $EXIT_CODE)"
            FAILED=$((FAILED + 1))
            ERRORS="$ERRORS  $LEVEL_NAME (exit $EXIT_CODE)\n"
            echo "    Last output:"
            tail -5 "$LOG" | sed 's/^/    /'
        fi
    fi

    set -e
    rm -f "$LOG"
done

echo ""
echo "=== Results: $PASSED passed, $FAILED failed (${#LEVELS[@]} total) ==="

if [[ -n "$ERRORS" ]]; then
    echo ""
    echo "Failed levels:"
    printf "$ERRORS"
    exit 1
fi

exit 0
