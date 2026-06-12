#!/usr/bin/env bash
# tests/scripts/test_menus.sh
#
# Scenario 2 -- pulldown menu regression test (the `make test-menus` target).
#
# For each of the 5 top-level pulldown menus (App, File, Dir, Mounts, Info):
#   - boot to the main interface, navigate the menu bar with RIGHT * (N+1)
#     presses and ENTER to open the pulldown, dump the screen and assert
#     every menu item's text renders uncorrupted (regression coverage for
#     the pulldown menu corruption bug, see project_locifm_pulldown_menu_bug)
#   - close the pulldown and the menu bar with ESC, ESC and assert the main
#     interface is restored cleanly with no pulldown residue (regression
#     coverage for menu_winsave/menu_winrestore window save/restore)
#
# Boot timing matches test_boot.sh: the app is interactive by 8,000,000
# cycles. --type-keys requires a >=1 frame gap (--p1, the minimum pause)
# between the RIGHT-arrow sequence and ENTER/ESC, otherwise the keyboard
# scanner's release debounce (RELEASE_DEBOUNCE=20 polls) swallows the next
# key -- consecutive different keys are otherwise typed back-to-back with
# no released-key gap.
#
# Required env vars (set by `make test-menus`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   locifm.tap or locifm_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py
BOOT_CYCLES=8000000
OPEN_CYCLES=9800000
CLOSE_CYCLES=11000000

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
echo "  locifilemanager-v2 -- pulldown menu regression test"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# rightcount:name:item|item|... (items are the exact MSG_MENU_* texts,
# without the leading "-" selection marker drawn on the first item)
menus=(
    "1:app:Confirm: Once|Return: Select|[F]ilter: None|S[O]rt: Off|[ESC] Exit"
    "2:file:[S]elect toggle|Select [A]ll|Select [N]one|[I]nverse select|[DEL]ete|[R]ename|[C]opy|Mo[v]e|Bro[W]se tape"
    "3:dir:[\\] Go to root|[<] Back|Page [D]own|Page U[P]|[T]op|[B]ottom|N[e]w dir"
    "4:mounts:[/] Switch pane|[.] Next drive|[,] Prev drive|[M]ount|[U]nmount|Tar[G]et: A|Show mounts"
    "5:info:Version/credits|Help"
)

for entry in "${menus[@]}"; do
    rights="${entry%%:*}"
    rest="${entry#*:}"
    name="${rest%%:*}"
    items="${rest#*:}"

    rs=""
    for ((j = 0; j < rights; j++)); do rs="${rs}\\r"; done

    echo ""
    echo "$name menu"

    open_dump="$OUT/menu_${name}_open.bin"
    run_emu "${BOOT_CYCLES}:${rs}\\p1\\n" "$OPEN_CYCLES" "$open_dump"
    if [ ! -f "$open_dump" ]; then
        echo "  [FAIL] emulator did not produce a RAM dump at $open_dump"
        fail=$((fail+1))
    else
        IFS='|' read -ra item_arr <<< "$items"
        for item in "${item_arr[@]}"; do
            check_found "item renders: $item" "$item" "$open_dump"
        done
    fi

    close_dump="$OUT/menu_${name}_closed.bin"
    run_emu "${BOOT_CYCLES}:${rs}\\p1\\n\\p1\\e\\p1\\e" "$CLOSE_CYCLES" "$close_dump"
    if [ ! -f "$close_dump" ]; then
        echo "  [FAIL] emulator did not produce a RAM dump at $close_dump"
        fail=$((fail+1))
    else
        check_found     "main interface restored after ESC" "LOCI File Manager" "$close_dump"
        check_found     "directory listing restored"        "0:/"               "$close_dump"
        check_not_found "no leftover pulldown residue"       "${item_arr[0]}"    "$close_dump"
    fi
done

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
