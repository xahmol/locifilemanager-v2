#!/usr/bin/env bash
# tests/scripts/test_recurse.sh
#
# Scenario 5 -- recursive operations foundation test (the `make test-recurse`
# target). See ROADMAP.md Phase 0.
#
# Phase 0 only adds scaffolding: recurse_walk() (src/recurse.c, not yet
# exercised by any UI action), the new 6th "Tools" menu-bar pulldown with
# Properties/Filter-by-name/View-text stubs (hotkeys k/l/j), and the DEEP/
# fixture tree used by later phases (recursive copy/move/delete, recursive
# directory size). This script asserts:
#   - the build produced a .tap
#   - the app still boots to its main interface
#   - the Tools pulldown opens and shows its 3 stub items
#   - the DEEP/ fixture tree (used by Phases 1, 2 and 7) is present with the
#     expected file sizes
#
# Later phases extend this script with behavioural tests.
#
# Tools is the 6th menu-bar item (App=1 .. Info=5, Tools=6). Per
# test_menus.sh, opening bar item N requires N RIGHT-arrow presses (the
# first RIGHT enters menu mode landing on App; each further RIGHT moves one
# item right) followed by \p1\n to open the pulldown.
#
# Required env vars (set by `make test-recurse`):
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
    if [ -n "$typekeys" ]; then
        "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
            -t "$SANDBOX/$TAPFILE" -f \
            --headless -c "$cycles" \
            --type-keys "$typekeys" \
            --dump-ram-at "$cycles":"$dump" >/dev/null 2>&1
    else
        "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
            -t "$SANDBOX/$TAPFILE" -f \
            --headless -c "$cycles" \
            --dump-ram-at "$cycles":"$dump" >/dev/null 2>&1
    fi
}

# Reset the sandbox from fixtures + freshly built .taps (mirrors the
# Makefile's sandbox-reset target). Needed mid-script for sub-tests that
# mutate tests/sandbox/ (Phase 1's copy and move sub-tests each need a
# pristine DEEP/ tree).
#
# .gitkeep placeholders (tracking otherwise-empty fixture dirs, e.g.
# DEEP/EMPTY/) are stripped so such directories are genuinely empty on the
# host filesystem -- needed for Phase 2's plain empty-dir delete sub-test.
reset_sandbox() {
    rm -rf "$SANDBOX"
    mkdir -p "$SANDBOX"
    cp -r tests/fixtures/. "$SANDBOX/"
    find "$SANDBOX" -name '.gitkeep' -delete
    cp "build/$TAPFILE" "$SANDBOX/"
    cp "build/${TAPFILE/locifm/libdemo}" "$SANDBOX/"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- recursive operations foundation test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

echo ""
echo "Build artifacts"
check_host "build succeeded" "[ -f 'build/$TAPFILE' ]"

echo ""
echo "Boot smoke"
boot_dump="$OUT/recurse_boot.bin"
run_emu "" $BOOT_CYCLES "$boot_dump"
if [ ! -f "$boot_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $boot_dump"
    fail=$((fail+1))
else
    check_found "main interface intact" "LOCI File Manager" "$boot_dump"
fi

echo ""
echo "Tools pulldown menu"
tools_dump="$OUT/recurse_tools_open.bin"
run_emu "${BOOT_CYCLES}:\\r\\r\\r\\r\\r\\p1\\n" $OPEN_CYCLES "$tools_dump"
if [ ! -f "$tools_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $tools_dump"
    fail=$((fail+1))
else
    check_found "Properties item visible"     "[K] Properties" "$tools_dump"
    check_found "Filter by name item visible" "[L] Text: Off"  "$tools_dump"
    check_found "View text item visible"      "[J] View text"  "$tools_dump"
fi

echo ""
echo "DEEP/ fixture tree"
check_host "DEEP fixture tree present" \
    "[ -d '$SANDBOX/DEEP/SUB1/SUB2' ] && [ -f '$SANDBOX/DEEP/FILEA.TXT' ]"
check_host "DEEP fixture file sizes correct (100/200/300 bytes)" \
    "[ \$(stat -c%s '$SANDBOX/DEEP/FILEA.TXT') -eq 100 ] && \
     [ \$(stat -c%s '$SANDBOX/DEEP/SUB1/FILEB.TXT') -eq 200 ] && \
     [ \$(stat -c%s '$SANDBOX/DEEP/SUB1/SUB2/FILEC.TXT') -eq 300 ]"
check_host "DEEP/EMPTY/ present" "[ -d '$SANDBOX/DEEP/EMPTY' ]"

# -----------------------------------------------------------------------
# Phase 1 -- Recursive directory copy/move
#
# Sort order with DEEP/ present (case-insensitive alpha, sort toggled on):
#   DEEP/(0) DEMO.TAP(1) FIRM.ROM(2) GAME.DSK(3) libdemo.tap(4) locifm.tap(5)
#   NOTES.TXT(6) SAVE.LCE(7) SUBDIR/(8)
#
# Sequence: o (sort on, resets both panes to pos 0) -> s (select DEEP/ in
# pane 0) -> / (switch active to pane 1) -> 8x down (pane 1 cursor to
# SUBDIR/, pos 8) -> ENTER (navigate pane 1 into SUBDIR/) -> / (switch back
# to pane 0, active again) -> c/v (copy/move the 1 selected item from pane 0
# into pane 1's path 0:/SUBDIR/) -> ENTER (dismiss final "Press a key" popup).
# -----------------------------------------------------------------------
COPYMOVE_CYCLES=30000000
COPYMOVE_KEYS_TMPL="${BOOT_CYCLES}:o\\p1s\\p1/\\p1\\d\\d\\d\\d\\d\\d\\d\\d\\p1\\n\\p1/\\p1__ACTION__\\p1\\n"

echo ""
echo "Phase 1 -- Recursive directory copy"
copy_dump="$OUT/recurse_copy.bin"
run_emu "${COPYMOVE_KEYS_TMPL/__ACTION__/c}" $COPYMOVE_CYCLES "$copy_dump"
if [ ! -f "$copy_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $copy_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact" "LOCI File Manager" "$copy_dump"
    check_not_found "no file error"         "File error"        "$copy_dump"
fi
check_host "SUBDIR/DEEP/FILEA.TXT copied"          "[ -f '$SANDBOX/SUBDIR/DEEP/FILEA.TXT' ]"
check_host "SUBDIR/DEEP/SUB1/FILEB.TXT copied"     "[ -f '$SANDBOX/SUBDIR/DEEP/SUB1/FILEB.TXT' ]"
check_host "SUBDIR/DEEP/SUB1/SUB2/FILEC.TXT copied" "[ -f '$SANDBOX/SUBDIR/DEEP/SUB1/SUB2/FILEC.TXT' ]"
check_host "SUBDIR/DEEP/EMPTY/ copied"             "[ -d '$SANDBOX/SUBDIR/DEEP/EMPTY' ]"
check_host "original DEEP/FILEA.TXT still present" "[ -f '$SANDBOX/DEEP/FILEA.TXT' ]"
check_host "original DEEP/ tree still present"     "[ -d '$SANDBOX/DEEP/SUB1/SUB2' ]"

echo ""
echo "Phase 1 -- Recursive directory move"
reset_sandbox
move_dump="$OUT/recurse_move.bin"
run_emu "${COPYMOVE_KEYS_TMPL/__ACTION__/v}" $COPYMOVE_CYCLES "$move_dump"
if [ ! -f "$move_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $move_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact" "LOCI File Manager" "$move_dump"
    check_not_found "no file error"         "File error"        "$move_dump"
fi
check_host "SUBDIR/DEEP/FILEA.TXT moved"          "[ -f '$SANDBOX/SUBDIR/DEEP/FILEA.TXT' ]"
check_host "SUBDIR/DEEP/SUB1/FILEB.TXT moved"     "[ -f '$SANDBOX/SUBDIR/DEEP/SUB1/FILEB.TXT' ]"
check_host "SUBDIR/DEEP/SUB1/SUB2/FILEC.TXT moved" "[ -f '$SANDBOX/SUBDIR/DEEP/SUB1/SUB2/FILEC.TXT' ]"
check_host "SUBDIR/DEEP/EMPTY/ moved"             "[ -d '$SANDBOX/SUBDIR/DEEP/EMPTY' ]"
check_host "original DEEP/ fully removed"        "[ ! -e '$SANDBOX/DEEP' ]"

# -----------------------------------------------------------------------
# Phase 2 -- Recursive directory delete
#
# The physical DEL key (0x7F) can't be sent via Phosphoric's --type-keys
# (unmapped in the ORIC keyboard matrix -- see test_fileops.sh's delete
# sub-test), so both sub-tests use File > [DEL]ete via the menu instead
# (menu code 25 -> dir_deletedir() when nothing is selected and the cursor
# is on a directory). Opening the File pulldown: \r\r (first RIGHT enters
# menu mode landing on App, second RIGHT moves to File) -> \n opens it;
# \d\d\d\d moves down to the 5th item ("[DEL]ete").
#
# The sandbox root is drive 0 (--loci-flash), so dir_deletedir()'s
# opendir+readdir pre-check (drive != 0 only) is skipped; a non-empty
# directory is instead detected when loci_unlink() fails with
# loci_errno == LOCI_EACCES (3, ENOTEMPTY -- see Phase 1's Phosphoric
# op_unlink fix), which triggers the "Delete ALL?" recursive-delete confirm.
# -----------------------------------------------------------------------
DELETE_CYCLES=30000000
DELETE_MENU_KEYS="\\r\\r\\p1\\n\\p1\\d\\d\\d\\d\\p1\\n"

echo ""
echo "Phase 2 -- Recursive directory delete (non-empty DEEP/)"
reset_sandbox
delete_dump="$OUT/recurse_delete.bin"
# o (sort on, cursor at DEEP/ pos 0) -> open File pulldown -> [DEL]ete ->
# "Delete dir?" Yes -> unlink fails (not empty) -> "Delete ALL?" Yes ->
# recursive delete runs -> "Press a key" dismiss.
run_emu "${BOOT_CYCLES}:o\\p1${DELETE_MENU_KEYS}\\p1\\n\\p1\\n\\p1\\n" $DELETE_CYCLES "$delete_dump"
if [ ! -f "$delete_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $delete_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact" "LOCI File Manager" "$delete_dump"
    check_not_found "no file error"         "File error"        "$delete_dump"
fi
check_host "DEEP/ fully removed" "[ ! -e '$SANDBOX/DEEP' ]"

echo ""
echo "Phase 2 -- Plain empty directory delete (DEEP/EMPTY/, regression)"
reset_sandbox
emptydel_dump="$OUT/recurse_delete_empty.bin"
# o (sort on, cursor at DEEP/ pos 0) -> ENTER (navigate into DEEP/, cursor
# resets to pos 0 = EMPTY/, sorted before FILEA.TXT and SUB1/) -> open File
# pulldown -> [DEL]ete -> "Delete dir?" Yes -> empty dir, unlink succeeds.
run_emu "${BOOT_CYCLES}:o\\p1\\n\\p1${DELETE_MENU_KEYS}\\p1\\n" $DELETE_CYCLES "$emptydel_dump"
if [ ! -f "$emptydel_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $emptydel_dump"
    fail=$((fail+1))
else
    check_found     "main interface intact" "LOCI File Manager" "$emptydel_dump"
    check_not_found "no file error"         "File error"        "$emptydel_dump"
fi
check_host "DEEP/EMPTY/ removed"        "[ ! -e '$SANDBOX/DEEP/EMPTY' ]"
check_host "DEEP/ parent still present" "[ -d '$SANDBOX/DEEP' ]"
check_host "DEEP/FILEA.TXT unaffected"  "[ -f '$SANDBOX/DEEP/FILEA.TXT' ]"

# Restore a pristine sandbox for any later phases appended to this script.
reset_sandbox

# -----------------------------------------------------------------------
# Phase 7 -- Properties popup + recursive directory size
#
# 'k' (direct hotkey) / Tools->[K] Properties (case 61) -> dir_show_properties().
# Both sub-tests press 'o' (sort on) and start from a fresh sandbox.
# -----------------------------------------------------------------------
PROPS_CYCLES=14000000

echo ""
echo "Phase 7 -- Properties popup, file (NOTES.TXT, sorted pos 6)"
reset_sandbox
propsfile_dump="$OUT/recurse_props_file.bin"
run_emu "${BOOT_CYCLES}:o\\p1\\d\\d\\d\\d\\d\\d\\p1k" $PROPS_CYCLES "$propsfile_dump"
if [ ! -f "$propsfile_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $propsfile_dump"
    fail=$((fail+1))
else
    check_found "popup title shown"   "Properties"   "$propsfile_dump"
    check_found "file name shown"     "NOTES.TXT"    "$propsfile_dump"
    check_found "file type shown"     "Type: .TXT"   "$propsfile_dump"
    check_found "byte count shown"    "78 bytes"     "$propsfile_dump"
    check_found "path shown"          "Path: 0:/"    "$propsfile_dump"
fi

echo ""
echo "Phase 7 -- Properties popup, directory recursive size (DEEP/, sorted pos 0)"
reset_sandbox
propsdir_dump="$OUT/recurse_props_dir.bin"
run_emu "${BOOT_CYCLES}:o\\p1k" $PROPS_CYCLES "$propsdir_dump"
if [ ! -f "$propsdir_dump" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $propsdir_dump"
    fail=$((fail+1))
else
    check_found "popup title shown"        "Properties" "$propsdir_dump"
    check_found "directory name shown"     "DEEP/"      "$propsdir_dump"
    check_found "directory type shown"     "Type: Directory"  "$propsdir_dump"
    check_found "recursive total shown"    "600 bytes"  "$propsdir_dump"
fi

# Restore a pristine sandbox for any later phases appended to this script.
reset_sandbox

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
