#!/usr/bin/env bash
# tests/scripts/test_viewer.sh
#
# Scenario 9 -- text file viewer test (the `make test-viewer` target).
# See ROADMAP.md Phase 6.
#
# Verifies the 'j' shortcut (and Tools->"View text file" pulldown item,
# case 63) opens viewer_show_text() (src/viewer.c), which:
#   - reads the selected file in chunks, splits on '\n' and word-wraps each
#     line into a full-screen content window via cwin_printwrap();
#   - shows "Press a key to continue" at EOF;
#   - paginates forward-only: once the content window's last row has been
#     used, shows "SPACE: next page  ESC: exit" and waits for a keypress
#     before clearing for the next page;
#   - ESC at any pause exits back to the main interface.
#
# Navigation (sort-on, calibrated against test_recurse.sh/test_namefilter.sh):
# root listing with sort on is DEEP/(0), DEMO.TAP(1), FIRM.ROM(2), GAME.DSK(3),
# libdemo.tap(4), locifm.tap(5), NOTES.TXT(6), SAVE.LCE(7), SUBDIR/(8).
#
# tests/fixtures/SUBDIR/LONGTEXT.TXT (30 short lines, "LINE NN OF LONGTEXT
# FIXTURE FILE") is the only entry in SUBDIR/ -- placed there (not at fixtures
# root) so it doesn't shift the root listing positions calibrated by other
# scenarios (see tests/scripts/test_copycancel.sh's fixture-placement note).
# Page 1 of the 27-row content window shows LINE 01..27; page 2 (after SPACE)
# shows LINE 28..30.
#
# Required env vars (set by `make test-viewer`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap)
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

run_emu() {
    local typekeys="$1" cycles="$2" dump="$3"
    "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
        -t "$SANDBOX/$TAPFILE" -f \
        --headless -c "$cycles" \
        --type-keys "$typekeys" \
        --dump-ram-at "$cycles":"$dump" >/dev/null 2>&1
}

# Every sub-test below presses 'o' (sort toggle), which now calls
# config_save() and writes 0:/LOCIFM.CFG to the sandbox root (Phase 3).
# That file doesn't change the listing *within* the run that creates it
# (dir listing is read once at boot, not rescanned), but it WOULD add a
# 10th root-level entry -- shifting the sorted positions of NOTES.TXT/
# SUBDIR/ -- for any later run in this script. So each sub-test gets its
# own fresh sandbox, same as tests/scripts/test_copycancel.sh.
reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    find "$SANDBOX" -name '.gitkeep' -delete
    cp "build/$TAPFILE" "$SANDBOX/"
    cp "build/${TAPFILE/locifm/libdemo}" "$SANDBOX/"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- text file viewer test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# -----------------------------------------------------------------------
# Sub-test 1: open NOTES.TXT (sorted pos 6) -- content visible, EOF prompt.
# -----------------------------------------------------------------------
echo ""
echo "View NOTES.TXT"
notes_dump="$OUT/viewer_notes.bin"
reset_sandbox
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\p1j" 14000000 "$notes_dump"
if [ ! -f "$notes_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $notes_dump"
    fail=$((fail+1))
else
    check_found "NOTES.TXT content visible" "PHOSPHORIC TEST FIXTURE"  "$notes_dump"
    check_found "EOF prompt shown"          "Press a key to continue" "$notes_dump"
fi

# -----------------------------------------------------------------------
# Sub-test 2: ESC at EOF prompt exits back to the main interface.
# -----------------------------------------------------------------------
echo ""
echo "ESC exits viewer, restores main interface"
exit_dump="$OUT/viewer_notes_exit.bin"
reset_sandbox
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\p1j\\p1\\e" 16000000 "$exit_dump"
if [ ! -f "$exit_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $exit_dump"
    fail=$((fail+1))
else
    check_found "main interface intact" "LOCI File Manager" "$exit_dump"
    check_found "listing restored"      "NOTES.TXT"         "$exit_dump"
fi

# -----------------------------------------------------------------------
# Sub-test 3: view LONGTEXT.TXT (SUBDIR/, only entry) -- page 1 (LINE 01-27).
# Navigation: 8x down to SUBDIR/ (pos 8), ENTER to navigate in (resets cursor
# to pos 0 on LONGTEXT.TXT), then 'j'.
# -----------------------------------------------------------------------
echo ""
echo "View LONGTEXT.TXT -- page 1"
page1_dump="$OUT/viewer_longtext_p1.bin"
reset_sandbox
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\d\\d\\p1\\n\\p1j" 16000000 "$page1_dump"
if [ ! -f "$page1_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $page1_dump"
    fail=$((fail+1))
else
    check_found     "LINE 01 visible on page 1" "LINE 01 OF LONGTEXT" "$page1_dump"
    check_found     "LINE 27 visible on page 1" "LINE 27 OF LONGTEXT" "$page1_dump"
    check_not_found "LINE 28 not yet visible"   "LINE 28 OF LONGTEXT" "$page1_dump"
    check_found     "pagination prompt shown"   "next page"           "$page1_dump"
fi

# -----------------------------------------------------------------------
# Sub-test 4: SPACE advances to page 2 (LINE 28-30), then EOF prompt.
# -----------------------------------------------------------------------
echo ""
echo "View LONGTEXT.TXT -- page 2 (SPACE)"
page2_dump="$OUT/viewer_longtext_p2.bin"
reset_sandbox
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\d\\d\\p1\\n\\p1j\\p1 " 18000000 "$page2_dump"
if [ ! -f "$page2_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $page2_dump"
    fail=$((fail+1))
else
    check_not_found "LINE 01 no longer visible" "LINE 01 OF LONGTEXT"     "$page2_dump"
    check_found     "LINE 28 visible on page 2" "LINE 28 OF LONGTEXT"     "$page2_dump"
    check_found     "LINE 30 visible on page 2" "LINE 30 OF LONGTEXT"     "$page2_dump"
    check_found     "EOF prompt shown"          "Press a key to continue" "$page2_dump"
fi

# -----------------------------------------------------------------------
# Sub-test 5: ESC from page 2 exits back to the main interface.
# -----------------------------------------------------------------------
echo ""
echo "ESC from page 2 restores main interface"
page2exit_dump="$OUT/viewer_longtext_exit.bin"
reset_sandbox
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\d\\d\\p1\\n\\p1j\\p1 \\p1\\e" 20000000 "$page2exit_dump"
if [ ! -f "$page2exit_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $page2exit_dump"
    fail=$((fail+1))
else
    check_found "main interface intact" "LOCI File Manager" "$page2exit_dump"
    check_found "listing restored"      "LONGTEXT.TXT"      "$page2exit_dump"
fi

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
