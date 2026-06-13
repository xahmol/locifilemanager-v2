#!/usr/bin/env bash
# tests/scripts/test_favourites.sh
#
# Scenario -- favourite directories test (the `make test-favourites` target).
# See ROADMAP.md "Favourite directories".
#
# Exercises favourites_add/delete/goto/show() (src/dir.c) and the Tools ->
# Favourites popup (hotkey 'y', case 54 in mainmenuloop()), persisted via
# config_save()/config_load() in struct FmConfig.favourites[8][48]:
#
#   1. Fresh boot -> open Favourites ('y') -> all 8 slots show "(empty)".
#      ESC closes the popup, restoring the main interface. locifm.cfg
#      (written by config_load()'s default-creation, as in
#      test_config.sh scenario 1) has its favourites region all-zero.
#   2. cd into DEEP/ ('o' sort + ENTER) -> open Favourites -> [A]dd current
#      path to slot 1 -> popup redraws showing "1: 0:/DEEP/"; locifm.cfg's
#      slot-0 bytes match "0:/DEEP/" + NUL padding to 48 bytes.
#   3. Pre-seed locifm.cfg with slot 0 = "0:/DEEP/" -> boot -> open
#      Favourites -> shows "1: 0:/DEEP/" -> press '1' -> popup closes and
#      the active pane jumps to 0:/DEEP/ (listing shows DEEP/'s contents).
#   4. From the same pre-seeded config -> open Favourites -> [D]elete slot
#      1 -> shows "1: (empty)"; locifm.cfg's favourites region is all-zero
#      again (back to defaults).
#
# Boot/menu timing matches test_config.sh/test_namefilter.sh: the app is
# interactive by 8,000,000 cycles; each \p1 typed-key gap costs roughly
# 1,000,000 cycles.
#
# Required env vars (set by `make test-favourites`):
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

# struct FmConfig = magic + 4 settings bytes + 8x48 favourites bytes (389
# bytes total). zero_hex N prints N zero bytes as hex (no separators).
zero_hex() {
    python3 -c "print('00' * $1, end='')"
}
DEFAULT_BYTES="a5$(zero_hex 388)"

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
# sub-tests' 0:/idi8b/ state doesn't leak into the next one.
reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    cp "build/$TAPFILE" "$SANDBOX/"
}

# Write a 389-byte locifm.cfg with magic=0xA5, default settings, and slot 0
# (displayed as "1:") set to the given path (NUL-padded to 48 bytes).
seed_cfg_slot0() {
    local path="$1"
    mkdir -p "$SANDBOX/idi8b/locifm"
    python3 -c "
import sys
data = bytearray(389)
data[0] = 0xa5
slot = b'$path'
data[5:5 + len(slot)] = slot
sys.stdout.buffer.write(bytes(data))
" > "$CFGFILE"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- favourite directories test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# --- 1. Fresh boot -> Favourites shows 8 empty slots -----------------------
echo ""
echo "Fresh boot -> Favourites popup shows 8 empty slots"
reset_sandbox
open_dump="$OUT/favourites_empty_open.bin"
run_emu "${BOOT_CYCLES}:y\\p1" 10000000 "$open_dump"
check_found "Favourites title shown" "Favourites" "$open_dump"
for i in 1 2 3 4 5 6 7 8; do
    check_found "slot $i empty" "$i: (empty)" "$open_dump"
done

closed_dump="$OUT/favourites_empty_closed.bin"
run_emu "${BOOT_CYCLES}:y\\p1\\e" 11000000 "$closed_dump"
check_found "main interface restored after ESC" "LOCI File Manager" "$closed_dump"
check_cfg_bytes "locifm.cfg favourites region all-zero on fresh boot" "$DEFAULT_BYTES"

# --- 2. cd into DEEP/ -> [A]dd current path to slot 1 ----------------------
echo ""
echo "cd into DEEP/ -> [A]dd current path to slot 1"
reset_sandbox
add_dump="$OUT/favourites_add.bin"
run_emu "${BOOT_CYCLES}:o\\p1\\n\\p1y\\p1a\\p11\\p1" 15000000 "$add_dump"
check_found "slot 1 shows 0:/DEEP/" "1: 0:/DEEP/" "$add_dump"

DEEP_HEX="$(python3 -c "print('0:/DEEP/'.encode().hex(), end='')")"
EXPECTED_ADD="a500000001${DEEP_HEX}$(zero_hex 40)$(zero_hex $((48 * 7)))"
check_cfg_bytes "locifm.cfg slot 1 bytes = 0:/DEEP/ + padding" "$EXPECTED_ADD"

# --- 3. Pre-seeded slot 1 -> open Favourites -> jump -----------------------
echo ""
echo "Pre-seeded slot 1 (0:/DEEP/) -> open Favourites -> jump"
reset_sandbox
seed_cfg_slot0 "0:/DEEP/"

show_dump="$OUT/favourites_preseeded.bin"
run_emu "${BOOT_CYCLES}:y\\p1" 11000000 "$show_dump"
check_found "slot 1 shows 0:/DEEP/ (pre-seeded)" "1: 0:/DEEP/" "$show_dump"

jump_dump="$OUT/favourites_jump.bin"
run_emu "${BOOT_CYCLES}:y\\p1\\p11" 13000000 "$jump_dump"
check_found "active pane jumped to 0:/DEEP/" "0:/DEEP/" "$jump_dump"
check_found "DEEP/ listing visible (FILEA.TXT)" "FILEA.TXT" "$jump_dump"
check_found "DEEP/ listing visible (EMPTY/)" "EMPTY/" "$jump_dump"

# --- 4. Pre-seeded slot 1 -> [D]elete --------------------------------------
echo ""
echo "Pre-seeded slot 1 (0:/DEEP/) -> [D]elete -> slot 1 empty again"
reset_sandbox
seed_cfg_slot0 "0:/DEEP/"

delete_dump="$OUT/favourites_delete.bin"
run_emu "${BOOT_CYCLES}:y\\p1d\\p1\\p11" 14000000 "$delete_dump"
check_found "slot 1 empty after delete" "1: (empty)" "$delete_dump"
check_cfg_bytes "locifm.cfg favourites region all-zero after delete" "$DEFAULT_BYTES"

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
