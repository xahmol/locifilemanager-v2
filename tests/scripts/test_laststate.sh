#!/usr/bin/env bash
# tests/scripts/test_laststate.sh
#
# Scenario -- remembered pane state test (the `make test-laststate` target).
# See ROADMAP.md "Remember last path/pane state".
#
# Exercises the lastpath/lastdrive/lastactivepane fields of struct FmConfig
# (src/dir.h), persisted automatically by config_save() on every navigation
# (dir_get_next_drive/dir_get_prev_drive/dir_switch_pane/dir_gotoroot/
# dir_parentdir/ENTER-descent in main.c) and restored by config_load() at
# boot, into 0:/idi8b/locifm/locifm.cfg ($SANDBOX/idi8b/locifm/locifm.cfg on
# the host).
#
#   1. Boot -> 'o' (sort toggle, puts DEEP/ first) -> ENTER (descend into
#      DEEP/ in pane 0) -> pane 0 shows "0:/DEEP/" with its listing;
#      locifm.cfg's sort byte = 1 and lastpath[0] = "0:/DEEP/" (NUL-padded).
#      Relaunch from the SAME sandbox (no reset -- the config persists on the
#      host filesystem across emulator launches, which is the feature under
#      test) with no navigation keys -> pane 0 is restored to "0:/DEEP/" with
#      the same listing.
#   2. Boot -> 'o' (sort toggle) -> '/' (switch active pane to pane 1) ->
#      locifm.cfg's lastactivepane byte = 1 (lastpath/lastdrive both still
#      defaults). Relaunch from the SAME sandbox -> ENTER acts on the
#      restored activepane (pane 1, position 0 = DEEP/ since sort=1 was also
#      restored) -> pane 1 shows "0:/DEEP/", pane 0 remains "0:/".
#
# Boot/menu timing matches test_config.sh/test_favourites.sh: the app is
# interactive by 8,000,000 cycles; each \p1 typed-key gap costs roughly
# 1,000,000 cycles.
#
# Required env vars (set by `make test-laststate`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (working copy, reset before each sub-test)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
BOOT_CYCLES=8000000
CFGFILE="$SANDBOX/idi8b/locifm/locifm.cfg"

# struct FmConfig = magic + 4 settings bytes + 8x48 favourites bytes +
# 2x48 lastpath bytes + 2 lastdrive bytes + 1 lastactivepane byte (488
# bytes total). zero_hex N prints N zero bytes as hex (no separators).
zero_hex() {
    python3 -c "print('00' * $1, end='')"
}
# "0:/" + NUL padding to FMCONFIG_FAV_PATHLEN=48 bytes -- each pane's
# lastpath[] on a fresh boot (both panes default to 0:/).
PANE_PATH_HEX="303a2f$(zero_hex 45)"
# "0:/DEEP/" + NUL padding to 48 bytes -- pane 0's lastpath[] after
# descending into DEEP/.
DEEP_HEX="303a2f444545502f"

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

# Check that a specific screen row contains $needle -- used to distinguish
# which pane (top pane = row 4, bottom pane = row 16) shows a given path.
check_row_contains() {
    local label="$1" row="$2" needle="$3" dump="$4" line
    line=$(python3 "$SCREEN" "$dump" --row "$row")
    if [[ "$line" == *"$needle"* ]]; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label -- '$needle' not found in row $row: $line"
        fail=$((fail+1))
    fi
}

check_cfg_bytes() {
    local label="$1" expected="$2" actual
    if [ ! -f "$CFGFILE" ]; then
        echo "  [FAIL] $label -- $CFGFILE not found"
        fail=$((fail+1))
        return
    fi
    # -v: don't collapse repeated lines with '*' (config has long all-zero
    # favourites runs).
    actual=$(od -An -tx1 -v "$CFGFILE" | tr -d ' \n')
    if [ "$actual" = "$expected" ]; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label -- expected $expected, got $actual"
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
# the Makefile's sandbox-reset target -- run before each sub-test so prior
# sub-tests' 0:/idi8b/ state doesn't leak into the next one. NOT called
# between a sub-test's two launches: --loci-flash persists locifm.cfg on the
# host filesystem across emulator launches, and that persistence is exactly
# the feature under test.
reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    cp "build/$TAPFILE" "$SANDBOX/"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- remembered pane state test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# --- 1. cd into DEEP/ (pane 0) -> relaunch -> path restored ---------------
echo ""
echo "Boot -> sort toggle + ENTER into DEEP/ (pane 0) -> relaunch restores path"
reset_sandbox

EXPECTED_NAV="a500000001$(zero_hex 384)${DEEP_HEX}$(zero_hex 40)${PANE_PATH_HEX}$(zero_hex 3)"

nav_dump="$OUT/laststate_nav.bin"
run_emu "${BOOT_CYCLES}:o\\p1\\n\\p1" 10000000 "$nav_dump"
check_found "pane 0 shows 0:/DEEP/ after navigation" "0:/DEEP/" "$nav_dump"
check_found "DEEP/ listing visible (FILEA.TXT)" "FILEA.TXT" "$nav_dump"
check_cfg_bytes "locifm.cfg after nav into DEEP/ (sort=1, lastpath[0]=0:/DEEP/)" "$EXPECTED_NAV"

relaunch_dump="$OUT/laststate_relaunch_nav.bin"
run_emu "${BOOT_CYCLES}:\\p1" 8000000 "$relaunch_dump"
check_row_contains "pane 0 restored to 0:/DEEP/ after relaunch" 4 "0:/DEEP/" "$relaunch_dump"
check_found "DEEP/ listing still visible (FILEA.TXT) after relaunch" "FILEA.TXT" "$relaunch_dump"

# --- 2. switch active pane -> relaunch -> activepane restored -------------
echo ""
echo "Boot -> sort toggle + switch active pane -> relaunch restores activepane"
reset_sandbox

EXPECTED_SWITCH="a500000001$(zero_hex 384)${PANE_PATH_HEX}${PANE_PATH_HEX}$(zero_hex 2)01"

switch_dump="$OUT/laststate_switch.bin"
run_emu "${BOOT_CYCLES}:o\\p1/\\p1" 10000000 "$switch_dump"
check_cfg_bytes "locifm.cfg after sort toggle + pane switch (activepane=1)" "$EXPECTED_SWITCH"

relaunch_dump="$OUT/laststate_relaunch_switch.bin"
run_emu "${BOOT_CYCLES}:\\n\\p1" 10000000 "$relaunch_dump"
check_row_contains "pane 1 (restored activepane) descends into 0:/DEEP/" 16 "0:/DEEP/" "$relaunch_dump"
check_row_contains "pane 0 unaffected, still 0:/" 4 "0:/" "$relaunch_dump"

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
