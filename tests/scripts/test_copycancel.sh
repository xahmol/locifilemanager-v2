#!/usr/bin/env bash
# tests/scripts/test_copycancel.sh
#
# Scenario 8 -- mid-file copy cancellation test (the `make test-copycancel`
# target). See ROADMAP.md Phase 5.
#
# Verifies that pressing ESC while a copy is in progress (detected inside
# file_copy_progress(), include/loci.c) causes:
#   - the copy loop to abort (-2 sentinel),
#   - the partial destination file to be removed (loci_unlink(dst)),
#   - "Copy cancelled." to be shown in the progress popup (MSG_FILE_COPY_CANCELLED).
#
# And, as a regression check, that an uninterrupted copy of the same large
# file still completes correctly (matching source/dest sizes).
#
# tests/assets/BIGFILE.BIN is 2MB (1024 x 2KB XRAM chunks). This size is
# load-bearing: Phosphoric's --type-keys debounce model means an ESC typed
# via "\\p1\\e" after a prior keypress only registers ~1,079,872 cycles after
# that prior key was pressed (79872-cycle hold + 1,000,000-cycle \\p1 gap for
# release_count to reach 0). The 2MB copy takes ~1.1M cycles, comfortably
# straddling that window so ESC reliably lands mid-copy. Smaller fixtures
# (64KB/256KB/512KB) complete before ESC can register.
#
# BIGFILE.BIN lives in tests/assets/ (not tests/fixtures/) and is copied into
# the sandbox root by this script's reset_sandbox(), below -- NOT via the
# shared sandbox-reset. tests/fixtures/ is shared by every test target's
# sandbox-reset, and adding a 9th root-level entry there shifts the
# alphabetically-sorted root listing positions that test_fileops.sh and
# test_recurse.sh's --type-keys sequences are calibrated against.
#
# Navigation sequence (sort-on, calibrated against test_recurse.sh/
# test_namefilter.sh): with sort on, root listing is BIGFILE.BIN(0), DEEP/(1),
# DEMO.TAP(2), FIRM.ROM(3), GAME.DSK(4), libdemo.tap(5), locifm.tap(6),
# NOTES.TXT(7), SAVE.LCE(8), SUBDIR/(9) -- exactly 10 entries (PANE_HEIGHT).
#   o        -- sort on (resets both panes to pos 0)
#   s        -- select BIGFILE.BIN (pane 0, pos 0)
#   /        -- switch to pane 1
#   9x down  -- move to SUBDIR/ (pos 9)
#   ENTER    -- navigate pane 1 into SUBDIR/
#   /        -- switch back to pane 0
#   c        -- copy selected file(s) to pane 1's directory (SUBDIR/)
#
# Required env vars (set by `make test-copycancel`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
NAV="8000000:o\\p1s\\p1/\\p1\\d\\d\\d\\d\\d\\d\\d\\d\\d\\p1\\n\\p1/\\p1c"

pass=0
fail=0
skipped=0

check_found() {
    local label="$1" needle="$2" dump="$3"
    if python3 "$SCREEN" "$dump" --find "$needle" >/dev/null 2>&1; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label -- '$needle' not found"
        fail=$((fail+1))
    fi
}

check_not_found() {
    local label="$1" needle="$2" dump="$3"
    if python3 "$SCREEN" "$dump" --find "$needle" >/dev/null 2>&1; then
        echo "  [FAIL] $label -- '$needle' found (should not be present)"
        fail=$((fail+1))
    else
        echo "  [PASS] $label"
        pass=$((pass+1))
    fi
}

check_host() {
    local label="$1" cond="$2"
    if eval "$cond"; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label"
        fail=$((fail+1))
    fi
}

run_emu() {
    local typekeys="$1" cycles="$2" dump="$3"
    "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
        -t "$SANDBOX/$TAPFILE" -f \
        --headless -c "$cycles" \
        --type-keys "$typekeys" \
        --dump-ram-at "$cycles":"$dump" >/dev/null 2>&1
}

reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    find "$SANDBOX" -name '.gitkeep' -delete
    cp tests/assets/BIGFILE.BIN "$SANDBOX/"
    cp "build/$TAPFILE" "$SANDBOX/"
    cp "build/${TAPFILE/locifm/libdemo}" "$SANDBOX/"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- mid-file copy cancellation test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# -----------------------------------------------------------------------
# Sub-test 1: cancel mid-copy with ESC.
# "\\p1\\e" after 'c' gives the copy loop a full 1,000,000-cycle no-key gap
# (releasing 'c' and resetting the keyboard debounce) before ESC is pressed,
# so keyb_check() inside file_copy_progress() detects KEY_ESC while the 2MB
# copy is still in progress.
# -----------------------------------------------------------------------
echo ""
echo "Cancel copy mid-transfer with ESC"
reset_sandbox
cancel_dump="$OUT/copycancel_esc.bin"
run_emu "${NAV}\\p1\\e" 17000000 "$cancel_dump"
if [ ! -f "$cancel_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $cancel_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact"   "LOCI File Manager" "$cancel_dump"
    check_found     "copy cancelled message"  "Copy cancelled."   "$cancel_dump"
    check_not_found "no file error"           "File error"        "$cancel_dump"
fi
check_host "partial dest file removed" \
    "[ ! -e '$SANDBOX/SUBDIR/BIGFILE.BIN' ]"
check_host "source BIGFILE.BIN untouched" \
    "[ -f '$SANDBOX/BIGFILE.BIN' ] && [ \$(stat -c%s '$SANDBOX/BIGFILE.BIN') -eq 2097152 ]"

# -----------------------------------------------------------------------
# Sub-test 2: regression -- uninterrupted copy of the same 2MB file still
# completes correctly (no ESC sent).
# -----------------------------------------------------------------------
echo ""
echo "Uninterrupted copy completes correctly (regression)"
reset_sandbox
ok_dump="$OUT/copycancel_ok.bin"
run_emu "${NAV}\\p1\\n" 20000000 "$ok_dump"
if [ ! -f "$ok_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $ok_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact" "LOCI File Manager" "$ok_dump"
    check_not_found "no cancelled message"  "Copy cancelled."   "$ok_dump"
    check_not_found "no file error"         "File error"        "$ok_dump"
fi
check_host "dest file copied with matching size" \
    "[ -f '$SANDBOX/SUBDIR/BIGFILE.BIN' ] && \
     [ \$(stat -c%s '$SANDBOX/SUBDIR/BIGFILE.BIN') -eq \$(stat -c%s '$SANDBOX/BIGFILE.BIN') ]"

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
