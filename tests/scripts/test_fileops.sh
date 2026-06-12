#!/usr/bin/env bash
# tests/scripts/test_fileops.sh
#
# Scenario 3 -- file operation regression test (the `make test-fileops` target).
#
# This is the end goal of the Phosphoric test harness initiative: exercises
# the LOCI-dependent copy/move/delete/rename/mkdir code paths that were
# previously "real LOCI hardware only" (see CLAUDE.md history).
#
# Each operation gets its own fresh sandbox (reset from tests/fixtures/ +
# the freshly built .tap), so cursor positions are deterministic:
#   - 'o' toggles sort on, which re-sorts both panes case-insensitively and
#     resets the cursor to position 0 -- always DEMO.TAP for the fixture set
#     (DEMO.TAP, FIRM.ROM, GAME.DSK, locifm.tap, NOTES.TXT, SAVE.LCE, SUBDIR/).
#   - Result is verified primarily via tests/sandbox/ host-filesystem state
#     (ground truth, no screen-scraping needed for the actual outcome), plus
#     a screen-dump sanity check that the main interface is intact and no
#     "File error!" popup appeared.
#
# --type-keys timing: the boot is interactive at 8,000,000 cycles (matches
# test_boot.sh/test_menus.sh). \p1 (the minimum 1-second pause, which calls
# oric_keyboard_release_all() and holds for 1,000,000 cycles) MUST be
# inserted between every pair of consecutive DIFFERENT keys -- this applies
# to arrows/ENTER/ESC AND regular characters alike, since keyb_poll()'s
# RELEASE_DEBOUNCE=20 requires ~20 consecutive "no key" polls before any new
# key-accept, and Phosphoric only auto-inserts a release gap for REPEATED
# same-key presses (1 frame = 19968 cycles, > the ~7400 cycles needed).
#
# Required env vars (set by `make test-fileops`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (working copy, reset before each sub-test)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
BOOT_CYCLES=8000000

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

# Reset tests/sandbox/ from checked-in fixtures + freshly built tap, mirroring
# the Makefile's sandbox-reset target -- run before each sub-test so cursor
# positions (and host-fs assertions) are independent of prior sub-tests.
reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    cp "build/$TAPFILE" "$SANDBOX/"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- file operation regression test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# --- mkdir: 'o' (sort) + 'e' (new dir) + name "A" + ENTER -----------------
echo ""
echo "mkdir (new directory)"
reset_sandbox
dump="$OUT/fileops_mkdir.bin"
run_emu "${BOOT_CYCLES}:o\\p1e\\p1A\\p1\\n" 12000000 "$dump"
check_found     "main interface intact"  "LOCI File Manager" "$dump"
check_not_found "no file error popup"    "File error"        "$dump"
check_host "directory 'a' created" "[ -d '$SANDBOX/a' ]"

# --- rename: 'o' (sort) + 'r' (rename DEMO.TAP) + "1" + ENTER -------------
echo ""
echo "rename (DEMO.TAP -> DEMO.TAP1)"
reset_sandbox
dump="$OUT/fileops_rename.bin"
run_emu "${BOOT_CYCLES}:o\\p1r\\p11\\p1\\n" 12000000 "$dump"
check_found     "main interface intact"  "LOCI File Manager" "$dump"
check_not_found "no file error popup"    "File error"        "$dump"
check_host "DEMO.TAP renamed to DEMO.TAP1" \
    "[ -f '$SANDBOX/DEMO.TAP1' ] && [ ! -e '$SANDBOX/DEMO.TAP' ]"

# --- delete: 'o' (sort) + File menu > [DEL]ete + confirm Yes + ack --------
echo ""
echo "delete (DEMO.TAP via File > [DEL]ete)"
reset_sandbox
dump="$OUT/fileops_delete.bin"
run_emu "${BOOT_CYCLES}:o\\p1\\r\\r\\p1\\n\\p1\\d\\d\\d\\d\\p1\\n\\p1\\n\\p1\\n" 16000000 "$dump"
check_found     "main interface intact"  "LOCI File Manager" "$dump"
check_not_found "no file error popup"    "File error"        "$dump"
check_host "DEMO.TAP deleted" "[ ! -e '$SANDBOX/DEMO.TAP' ]"

# --- copy: 'o' (sort) + switch pane + cd into SUBDIR/ + switch back + 'c' -
echo ""
echo "copy (DEMO.TAP -> SUBDIR/)"
reset_sandbox
dump="$OUT/fileops_copy.bin"
run_emu "${BOOT_CYCLES}:o\\p1/\\p1\\d\\d\\d\\d\\d\\d\\p1\\n\\p1/\\p1c\\p1\\n" 17000000 "$dump"
check_found     "main interface intact"  "LOCI File Manager" "$dump"
check_not_found "no file error popup"    "File error"        "$dump"
check_host "DEMO.TAP copied to SUBDIR/, original retained" \
    "[ -f '$SANDBOX/SUBDIR/DEMO.TAP' ] && [ -f '$SANDBOX/DEMO.TAP' ]"

# --- move: 'o' (sort) + switch pane + cd into SUBDIR/ + switch back + 'v' -
echo ""
echo "move (DEMO.TAP -> SUBDIR/)"
reset_sandbox
dump="$OUT/fileops_move.bin"
run_emu "${BOOT_CYCLES}:o\\p1/\\p1\\d\\d\\d\\d\\d\\d\\p1\\n\\p1/\\p1v\\p1\\n" 17000000 "$dump"
check_found     "main interface intact"  "LOCI File Manager" "$dump"
check_not_found "no file error popup"    "File error"        "$dump"
check_host "DEMO.TAP moved to SUBDIR/" \
    "[ -f '$SANDBOX/SUBDIR/DEMO.TAP' ] && [ ! -e '$SANDBOX/DEMO.TAP' ]"

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
