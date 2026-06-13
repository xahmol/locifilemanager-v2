#!/usr/bin/env bash
# tests/scripts/test_namefilter.sh
#
# Scenario 7 -- filename wildcard filter test (the `make test-namefilter`
# target). See ROADMAP.md Phase 4.
#
# Verifies the 'l' shortcut (and Tools->"Filter files by name" pulldown item,
# case 62) opens a popup (select_namefilter() in main.c) that:
#  - sets namefilter[] from a typed pattern, applied case-insensitively to
#    non-directory entries only (directories are always shown) via
#    dir_wildcard_match() in dir_read();
#  - shows "Current: <pattern>" / "Current: None" on reopen;
#  - clears the filter when ENTER is pressed on an empty input, restoring
#    the full listing.
#
# namefilter is NOT persisted (separate from LOCIFM.CFG / Phase 3), so both
# the "set" and "set then clear" cases are exercised within a single boot.
#
# Required env vars (set by `make test-namefilter`):
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

echo "==========================================================="
echo "  locifilemanager-v2 -- filename wildcard filter test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# -----------------------------------------------------------------------
# Set filter -- 'l' opens select_namefilter(), type "*.TXT", ENTER.
# Only NOTES.TXT (non-directory match) should remain visible; DEMO.TAP,
# GAME.DSK, FIRM.ROM, SAVE.LCE, locifm.tap and libdemo.tap should be hidden.
# Directories (DEEP/, SUBDIR/) are always shown regardless of name filter.
# -----------------------------------------------------------------------
echo ""
echo "Set filter to *.TXT"
set_dump="$OUT/namefilter_set.bin"
run_emu "${BOOT_CYCLES}:l\\p1*\\p1.\\p1T\\p1X\\p1T\\p1\\n" 17000000 "$set_dump"
if [ ! -f "$set_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $set_dump"
    fail=$((fail+1))
else
    check_found     "NOTES.TXT visible"     "NOTES.TXT"   "$set_dump"
    check_found     "DEEP/ still visible"   "DEEP/"       "$set_dump"
    check_found     "SUBDIR/ still visible" "SUBDIR/"     "$set_dump"
    check_not_found "DEMO.TAP hidden"       "DEMO.TAP"    "$set_dump"
    check_not_found "GAME.DSK hidden"       "GAME.DSK"    "$set_dump"
    check_not_found "FIRM.ROM hidden"       "FIRM.ROM"    "$set_dump"
    check_not_found "SAVE.LCE hidden"       "SAVE.LCE"    "$set_dump"
    check_not_found "locifm.tap hidden"     "locifm.tap"  "$set_dump"
    check_not_found "libdemo.tap hidden"    "libdemo.tap" "$set_dump"
fi

# -----------------------------------------------------------------------
# Clear filter -- reopen with 'l' (shows "Current: *.txt"), then ENTER on
# the empty input clears namefilter[]. Full listing should be restored.
# -----------------------------------------------------------------------
echo ""
echo "Clear filter (ENTER on empty input)"
clear_dump="$OUT/namefilter_clear.bin"
run_emu "${BOOT_CYCLES}:l\\p1*\\p1.\\p1T\\p1X\\p1T\\p1\\n\\p1l\\p1\\n" 21000000 "$clear_dump"
if [ ! -f "$clear_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $clear_dump"
    fail=$((fail+1))
else
    check_found "main interface intact" "LOCI File Manager" "$clear_dump"
    check_found "DEMO.TAP reappears"    "DEMO.TAP"          "$clear_dump"
    check_found "GAME.DSK reappears"    "GAME.DSK"          "$clear_dump"
    check_found "FIRM.ROM reappears"    "FIRM.ROM"          "$clear_dump"
    check_found "SAVE.LCE reappears"    "SAVE.LCE"          "$clear_dump"
    check_found "NOTES.TXT still there" "NOTES.TXT"         "$clear_dump"
fi

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
