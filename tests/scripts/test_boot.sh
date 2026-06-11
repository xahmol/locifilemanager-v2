#!/usr/bin/env bash
# tests/scripts/test_boot.sh
#
# Scenario 1 -- boot smoke test (the `make test-quick` / `make test` target).
#
# Fast-loads the freshly built locifm.tap under Atmos BASIC 1.1, with
# Phosphoric's --loci-flash sandbox standing in for the LOCI's USB storage,
# then decodes the $BB80 screen-text dump and asserts:
#   - the app boots and renders its main interface
#   - the LOCI device is detected (no "No LOCI device detected" message)
#   - the menu bar renders
#   - each tests/fixtures/ entry appears in the directory listing
#
# Required env vars (set by `make test-quick`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

CYCLES=8000000
SCREEN=tests/scripts/oric_screen.py
DUMP="$OUT/capture.bin"

pass=0
fail=0
skipped=0

check_found() {
    local label="$1" needle="$2"
    if python3 "$SCREEN" "$DUMP" --find "$needle" >/dev/null 2>&1; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label -- '$needle' not found"
        fail=$((fail+1))
    fi
}

check_not_found() {
    local label="$1" needle="$2"
    if python3 "$SCREEN" "$DUMP" --find "$needle" >/dev/null 2>&1; then
        echo "  [FAIL] $label -- '$needle' found (should not be present)"
        fail=$((fail+1))
    else
        echo "  [PASS] $label"
        pass=$((pass+1))
    fi
}

echo "==========================================================="
echo "  locifilemanager-v2 -- boot smoke test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

"$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
    -t "$SANDBOX/$TAPFILE" -f \
    --headless -c $CYCLES \
    --dump-ram-at $CYCLES:"$DUMP" >/dev/null 2>&1

if [ ! -f "$DUMP" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $DUMP"
    fail=$((fail+1))
else
    echo ""
    echo "Boot + main interface"
    check_found     "main interface header renders" "LOCI File Manager"
    check_not_found "LOCI device detected"          "No LOCI device detected"
    check_found     "menu bar renders"              "Mounts"
    check_found     "root path indicator"           "0:/"

    echo ""
    echo "Directory listing -- tests/fixtures/ entries"
    check_found "locifm.tap"  "locifm.tap"
    check_found "SAVE.LCE"    "SAVE.LCE"
    check_found "FIRM.ROM"    "FIRM.ROM"
    check_found "GAME.DSK"    "GAME.DSK"
    check_found "DEMO.TAP"    "DEMO.TAP"
    check_found "NOTES.TXT"   "NOTES.TXT"
    check_found "SUBDIR/"     "SUBDIR/"
fi

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
