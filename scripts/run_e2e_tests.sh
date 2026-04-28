#!/usr/bin/env bash
# Run all E2E scenarios in Testing/scenarios/.
#
# Each scenario is a .script file. Engine flags are embedded as `# args:` lines:
#   # args: --map 10_test_map --skip-intro
# Multiple `# args:` lines are concatenated.
#
# Pass criteria: engine exits 0 with no failed expectations.
# Fail codes:
#   2     -> assertion failure (one or more `expect` directives failed)
#   >128  -> crash (signal $((code-128)))
#   other -> error (e.g. arg parse, asset load)
#
# Usage:
#   ./scripts/run_e2e_tests.sh [--headless] [build_dir] [game]
#     --headless: pass --headless to engine (opens no window). Faster + needed for CI,
#                 but headless mode has known instability in some flows; default is
#                 windowed for reliability on dev machines.
#     build_dir:  path to build dir (default: build)
#     game:       game name          (default: doom2rpg)

set -euo pipefail

HEADLESS_FLAG=""
POSITIONAL=()
for arg in "$@"; do
    case "$arg" in
        --headless) HEADLESS_FLAG="--headless" ;;
        *) POSITIONAL+=("$arg") ;;
    esac
done

BUILD_DIR="${POSITIONAL[0]:-build}"
GAME="${POSITIONAL[1]:-doom2rpg}"
BINARY="$BUILD_DIR/src/DRPGEngine"
# On macOS the engine is bundled into a .app — fall back to the bundle binary.
if [[ ! -x "$BINARY" && -x "$BUILD_DIR/src/DRPGEngine.app/Contents/MacOS/DRPGEngine" ]]; then
    BINARY="$BUILD_DIR/src/DRPGEngine.app/Contents/MacOS/DRPGEngine"
fi

SCENARIO_DIR="Testing/scenarios"
PASSED=0
FAILED=0
FAILURES=""

if [[ ! -x "$BINARY" ]]; then
    echo "ERROR: Binary not found at $BINARY"
    echo "Build the project first: cmake --build $BUILD_DIR"
    exit 1
fi

if [[ ! -d "$SCENARIO_DIR" ]]; then
    echo "ERROR: Scenario directory not found: $SCENARIO_DIR"
    exit 1
fi

shopt -s nullglob
SCENARIOS=("$SCENARIO_DIR"/*.script)
shopt -u nullglob

if [[ ${#SCENARIOS[@]} -eq 0 ]]; then
    echo "ERROR: No .script files in $SCENARIO_DIR"
    exit 1
fi

echo "=== E2E Scenario Tests ==="
echo "Binary:    $BINARY"
echo "Game:      $GAME"
echo "Mode:      ${HEADLESS_FLAG:-windowed}"
echo "Scenarios: ${#SCENARIOS[@]}"
echo ""

for SCENARIO in "${SCENARIOS[@]}"; do
    NAME=$(basename "$SCENARIO" .script)
    printf "  %-30s ... " "$NAME"

    # Extract `# args:` lines; concatenate.
    ARGS=$(grep '^# args:' "$SCENARIO" | sed 's/^# args://' | tr '\n' ' ')

    LOG=$(mktemp)
    set +e
    # shellcheck disable=SC2086 # ARGS is intentionally word-split
    "$BINARY" --game "$GAME" $HEADLESS_FLAG --script "$SCENARIO" $ARGS >"$LOG" 2>&1
    EXIT_CODE=$?
    set -e

    case "$EXIT_CODE" in
        0)
            echo "PASS"
            PASSED=$((PASSED + 1))
            ;;
        2)
            echo "FAIL (assertion)"
            FAILED=$((FAILED + 1))
            FAIL_LINES=$(grep '^EXPECT_FAIL:' "$LOG" || true)
            if [[ -n "$FAIL_LINES" ]]; then
                echo "$FAIL_LINES" | sed 's/^/      /'
            fi
            FAILURES="$FAILURES\n  $NAME (assertion)"
            ;;
        *)
            if [[ "$EXIT_CODE" -gt 128 ]]; then
                SIG=$((EXIT_CODE - 128))
                echo "CRASH (signal $SIG)"
                FAILURES="$FAILURES\n  $NAME (signal $SIG)"
            else
                echo "ERROR (exit $EXIT_CODE)"
                FAILURES="$FAILURES\n  $NAME (exit $EXIT_CODE)"
            fi
            FAILED=$((FAILED + 1))
            echo "      Last output:"
            tail -5 "$LOG" | sed 's/^/      /'
            ;;
    esac

    rm -f "$LOG"
done

TOTAL=$((PASSED + FAILED))
echo ""
echo "=== Results: $PASSED passed, $FAILED failed ($TOTAL total) ==="

if [[ $FAILED -gt 0 ]]; then
    echo ""
    echo "Failures:"
    printf "%b\n" "$FAILURES"
    exit 1
fi

exit 0
