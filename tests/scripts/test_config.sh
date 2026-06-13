#!/usr/bin/env bash
# tests/scripts/test_config.sh
#
# Scenario -- persistent settings regression test (the `make test-config`
# target).
#
# Exercises config_load()/config_save() (src/dir.c), which read/write
# FMCONFIG_PATH = "0:/idi8b/locifm/locifm.cfg" (struct FmConfig: magic,
# confirm, filter, enterchoice, sort). Phosphoric's --loci-flash sandbox maps
# "0:/..." directly onto $SANDBOX/..., so the saved file is checked as
# $SANDBOX/idi8b/locifm/locifm.cfg on the host filesystem.
#
# NOTE: Phosphoric's --loci-flash backs "0:" with a plain host directory, the
# same as drives "1:"-"9:". A previous flat-file persistent-settings feature
# (0:/LOCIFM.CFG) passed this harness but wedged real LOCI hardware solid
# (survives RESET, needs power-cycle) -- see
# project_locifm_persistent_settings_removed. Passing this test does NOT
# confirm the nested-directory 0:/idi8b/locifm/ path is safe on real
# hardware; that still needs manual verification on real LOCI.
#
#   1. Fresh sandbox (no 0:/idi8b/ dir) -> boot -> config_load() finds no
#      file, falls back to config_save() with compiled-in defaults ->
#      0:/idi8b/locifm/locifm.cfg created with magic=0xA5 and all-zero
#      settings bytes.
#   2. Pre-seed 0:/idi8b/locifm/locifm.cfg with valid magic + non-default
#      settings -> boot -> config_load() applies them (App pulldown menu
#      reflects the loaded values) and leaves the file unchanged.
#   3. Pre-seed 0:/idi8b/locifm/locifm.cfg with an invalid magic byte ->
#      boot -> config_load() rejects it and config_save() rewrites the file
#      with compiled-in defaults.
#   4. Fresh sandbox -> boot (creates default locifm.cfg, dirs now exist) ->
#      press 'o' (S[O]rt toggle, a global hotkey calling dir_togglesort())
#      -> config_save() is called immediately on the setting change (no
#      reboot) and locifm.cfg's sort byte reflects the new value right away.
#      Also exercises config_save()'s loci_mkdir() calls when 0:/idi8b/ and
#      0:/idi8b/locifm/ already exist (errors ignored intentionally).
#
# Boot/menu timing matches test_menus.sh: the app is interactive by
# 8,000,000 cycles; opening the App pulldown (1x RIGHT + ENTER, with the
# mandatory \p1 release-gap) is stable by 9,800,000 cycles.
#
# Required env vars (set by `make test-config`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (working copy, reset before each sub-test)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
BOOT_CYCLES=8000000
OPEN_CYCLES=9800000
CFGFILE="$SANDBOX/idi8b/locifm/locifm.cfg"
DEFAULT_BYTES="a500000000"

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

check_cfg_bytes() {
    local label="$1" expected="$2" actual
    if [ ! -f "$CFGFILE" ]; then
        echo "  [FAIL] $label -- $CFGFILE not found"
        fail=$((fail+1))
        return
    fi
    actual=$(od -An -tx1 "$CFGFILE" | tr -d ' \n')
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

echo "==========================================================="
echo "  locifilemanager-v2 -- persistent settings (locifm.cfg) test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# --- 1. no config file -> defaults written -------------------------------
echo ""
echo "no config file -> config_save() writes defaults"
reset_sandbox
dump="$OUT/config_defaults.bin"
run_emu "${BOOT_CYCLES}:\\p1" "$BOOT_CYCLES" "$dump"
check_found "main interface intact" "LOCI File Manager" "$dump"
check_host  "0:/idi8b/locifm/ created" "[ -d '$SANDBOX/idi8b/locifm' ]"
check_cfg_bytes "locifm.cfg has default bytes (magic+all-zero settings)" "$DEFAULT_BYTES"

# --- 2. existing valid config -> loaded, file left unchanged --------------
echo ""
echo "existing valid config -> config_load() applies settings, file unchanged"
reset_sandbox
mkdir -p "$SANDBOX/idi8b/locifm"
printf '\xa5\x01\x01\x01\x01' > "$CFGFILE"
dump="$OUT/config_loaded.bin"
run_emu "${BOOT_CYCLES}:\\r\\p1\\n" "$OPEN_CYCLES" "$dump"
check_found "Confirm: All (confirm=1 loaded)"   "Confirm: All"   "$dump"
check_found "Return: Enter (enterchoice=1 loaded)" "Return: Enter" "$dump"
check_found "[F] Type: .DSK (filter=1 loaded)"  "[F] Type: .DSK" "$dump"
check_found "S[O]rt: On (sort=1 loaded)"        "S[O]rt: On"     "$dump"
check_cfg_bytes "locifm.cfg unchanged after load" "a501010101"

# --- 3. existing config with bad magic -> rewritten with defaults ---------
echo ""
echo "existing config with bad magic -> config_save() rewrites defaults"
reset_sandbox
mkdir -p "$SANDBOX/idi8b/locifm"
printf '\x00\x01\x01\x01\x01' > "$CFGFILE"
dump="$OUT/config_badmagic.bin"
run_emu "${BOOT_CYCLES}:\\p1" "$BOOT_CYCLES" "$dump"
check_found "main interface intact" "LOCI File Manager" "$dump"
check_cfg_bytes "locifm.cfg rewritten with defaults (bad magic rejected)" "$DEFAULT_BYTES"

# --- 4. runtime sort toggle ('o') -> immediate config_save() --------------
echo ""
echo "runtime sort toggle ('o') -> config_save() persists immediately"
reset_sandbox
dump="$OUT/config_runtime_sort.bin"
run_emu "${BOOT_CYCLES}:o\\p1" 12000000 "$dump"
check_found "main interface intact" "LOCI File Manager" "$dump"
check_cfg_bytes "locifm.cfg updated immediately after sort toggle" "a500000001"

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
