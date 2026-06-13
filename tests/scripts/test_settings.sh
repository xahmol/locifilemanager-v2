#!/usr/bin/env bash
# tests/scripts/test_settings.sh
#
# Scenario 6 -- persistent settings test (the `make test-settings` target).
# See ROADMAP.md Phase 3.
#
# Verifies confirm/filter/enterchoice/sort persist to 0:/LOCIFM.CFG
# (config_save(), called from confirm_toggle()/select_enter_choice()/
# select_filter()/dir_togglesort()) and reload on the next boot
# (config_load(), called from main() before the App pulldown labels are
# populated). A missing/absent config file falls back to the compiled-in
# defaults silently.
#
# Required env vars (set by `make test-settings`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
BOOT_CYCLES=8000000
OPEN_CYCLES=10000000
SAVE_CYCLES=14000000

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

run_emu() {
    local typekeys="$1" cycles="$2" dump="$3"
    "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
        -t "$SANDBOX/$TAPFILE" -f \
        --headless -c "$cycles" \
        --type-keys "$typekeys" \
        --dump-ram-at "$cycles":"$dump" >/dev/null 2>&1
}

echo "==========================================================="
echo "  locifilemanager-v2 -- persistent settings test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# -----------------------------------------------------------------------
# Save -- toggle sort on (o, direct key -> dir_togglesort() -> config_save())
# and filter to .DSK (f -> select_filter() popup -> DOWN to .DSK -> ENTER ->
# config_save()). Both writes target 0:/LOCIFM.CFG on the sandbox root
# (drive 0).
# -----------------------------------------------------------------------
echo ""
echo "Save settings (sort on, filter .DSK)"
check_host "no pre-existing LOCIFM.CFG" "[ ! -f '$SANDBOX/LOCIFM.CFG' ]"
save_dump="$OUT/settings_save.bin"
run_emu "${BOOT_CYCLES}:o\\p1f\\p1\\d\\p1\\n" $SAVE_CYCLES "$save_dump"
if [ ! -f "$save_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $save_dump"
    fail=$((fail+1))
else
    check_found "main interface intact" "LOCI File Manager" "$save_dump"
fi
check_host "LOCIFM.CFG created" "[ -f '$SANDBOX/LOCIFM.CFG' ]"

# -----------------------------------------------------------------------
# Load -- reboot the emulator (same sandbox, LOCIFM.CFG from the save step
# still present, no toggles this time) and open the App pulldown to check
# that config_load() restored sort=on and filter=.DSK.
# -----------------------------------------------------------------------
echo ""
echo "Reload settings on next boot"
load_dump="$OUT/settings_load.bin"
run_emu "${BOOT_CYCLES}:\\r\\p1\\n" $OPEN_CYCLES "$load_dump"
if [ ! -f "$load_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $load_dump"
    fail=$((fail+1))
else
    check_found "Sort On restored"     "S[O]rt: On"     "$load_dump"
    check_found "Filter .DSK restored" "[F]ilter: .DSK" "$load_dump"
fi

# -----------------------------------------------------------------------
# Fallback -- fresh sandbox (no LOCIFM.CFG): config_load() finds nothing and
# the compiled-in defaults from main() remain in effect.
# -----------------------------------------------------------------------
echo ""
echo "Fallback to defaults when LOCIFM.CFG is absent"
rm -rf "$SANDBOX"
mkdir -p "$SANDBOX"
cp -r tests/fixtures/. "$SANDBOX/"
find "$SANDBOX" -name '.gitkeep' -delete
cp "build/$TAPFILE" "$SANDBOX/"
cp "build/${TAPFILE/locifm/libdemo}" "$SANDBOX/"

fallback_dump="$OUT/settings_fallback.bin"
run_emu "${BOOT_CYCLES}:\\r\\p1\\n" $OPEN_CYCLES "$fallback_dump"
if [ ! -f "$fallback_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $fallback_dump"
    fail=$((fail+1))
else
    check_found "Sort Off default"     "S[O]rt: Off"    "$fallback_dump"
    check_found "Confirm Once default" "Confirm: Once"  "$fallback_dump"
    check_found "Filter None default"  "[F]ilter: None" "$fallback_dump"
fi

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
