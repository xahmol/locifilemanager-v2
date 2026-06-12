#!/usr/bin/env bash
# tests/scripts/test_libdemo.sh
#
# Scenario 4 -- libdemo full walkthrough (the `make test-libdemo` target).
#
# Drives the freshly built libdemo.tap under Atmos BASIC 1.1 via Phosphoric's
# --loci-flash sandbox (standing in for the LOCI's USB storage) and
# --type-keys auto-typer, decoding the $BB80 screen-text dump at calibrated
# checkpoints to walk through:
#   - the title/status screen and overlay RAM push/pop regression check
#     (project_locifm_overlay_ram_irq_crash)
#   - the colour test + cwin_textinput widget + key-echo loop
#   - every lettered section A-P of the demo (palette, ASTR attrs, fill-rect,
#     animated cursor walk, getat read-back, text editing, scroll benchmarks,
#     bouncing-ball animation, viewport scrolling, the full menu system + all
#     four popup helpers, the LOCI info screen, cursor move/put/get-chars,
#     get_rect/put_rect, printwrap, and scroll_left/scroll_right)
#   - the final "LIBRARY TEST COMPLETE. HALTED." screen
#
# Each checkpoint is its own emulator run: Phosphoric only honours the LAST
# --type-keys/--dump-ram-at flag, and the auto-typer schedule is cumulative
# from cycle 8,000,000, so every checkpoint replays the full key sequence
# typed so far and dumps RAM at a cycle count empirically calibrated to land
# on that checkpoint's screen.
#
# Required env vars (set by `make test-libdemo`):
#   PHOS      path to oric1-emu
#   ATMOSROM  path to roms/basic11b.rom
#   SANDBOX   tests/sandbox (fixtures + freshly built .tap files)
#   OUT       tests/out (scratch dir for RAM dumps)
#   TAPFILE   libdemo.tap or libdemo_fr.tap

set -u
cd "$(dirname "$0")/../.." || exit 1

SCREEN=tests/scripts/oric_screen.py

pass=0
fail=0
skipped=0

check_found() {
    local dump="$1" label="$2" needle="$3"
    if python3 "$SCREEN" "$dump" --find "$needle" >/dev/null 2>&1; then
        echo "  [PASS] $label"
        pass=$((pass+1))
    else
        echo "  [FAIL] $label -- '$needle' not found"
        fail=$((fail+1))
    fi
}

check_not_found() {
    local dump="$1" label="$2" needle="$3"
    if python3 "$SCREEN" "$dump" --find "$needle" >/dev/null 2>&1; then
        echo "  [FAIL] $label -- '$needle' found (should not be present)"
        fail=$((fail+1))
    else
        echo "  [PASS] $label"
        pass=$((pass+1))
    fi
}

# run_capture TK CYCLES NAME -- runs libdemo.tap with the given cumulative
# auto-typer sequence TK (scheduled from cycle 8,000,000), dumps RAM at
# CYCLES into $OUT/libdemo_NAME.bin, and echoes the dump path.
run_capture() {
    local tk="$1" cycles="$2" name="$3"
    local dump="$OUT/libdemo_${name}.bin"
    "$PHOS" -r "$ATMOSROM" --loci-flash "$SANDBOX" \
        -t "$SANDBOX/$TAPFILE" -f \
        --headless -c "$cycles" \
        --type-keys "8000000:${tk}" \
        --dump-ram-at "$cycles:$dump" >/dev/null 2>&1
    echo "$dump"
}

echo "==========================================================="
echo "  locifilemanager-v2 -- libdemo full walkthrough"
echo "==========================================================="

if [ ! -x "$PHOS" ]; then
    echo "  oric1-emu not found/executable at $PHOS -- skipping"
    exit 0
fi

# -----------------------------------------------------------------------
# Auto-typer building blocks. \pN = pause N seconds (N*1,000,000 cycles),
# \n = ENTER, \e = ESC, \u\d\l\r = arrows. The schedule is cumulative: each
# TK below is the full sequence typed so far, calibrated against its CYCLES
# checkpoint to land on the expected screen.
# -----------------------------------------------------------------------
S01='\p1\n'   # accept cwin_textinput
S02='\p1\n'   # advance past textinput's getch -> key-echo loop
S03='\p1X'    # press 'X' in the key-echo loop
S04='\p1\n'   # ENTER exits key-echo loop -> Section A
S05='\p1\n'   # Section A getch -> Section B
S06='\p1\n'   # Section B getch -> Section C
S07='\p1\n'   # Section C getch -> Section D (starts ~1M-cycle cursor-walk animation)
S08='\p2\n'   # Section D getch -> Section E (extra pause for animation to finish)
S09='\p1\n'   # Section E getch -> Section F (insert/delete demo)
S10='\p1\n'   # Section F insert/delete getch -> printline demo
S11='\p1\n'   # Section F printline getch -> Section G (150-iter scroll bench)
S12='\p3\n'   # Section G getch -> Section H (extra pause for scroll bench)
S13='\p2\n'   # Section H getch -> Section I (bouncing ball)
S14='\p1\n'   # ENTER interrupts Section I's bounce loop early
S15='\p1\n'   # Section I getch -> Section J (viewport)
S16='\p1\r'   # scroll viewport right
S17='\p1\d'   # scroll viewport down
S18='\p1\e'   # ESC exits viewport loop
S19='\p1\n'   # Section J getch -> Section K (menu_main)
S20='\p1\e'   # ESC at menu bar exits menu_main -> popup demos
S21='\p1\n'   # dismiss message popup (any key)
S22='\p1\n'   # ENTER -> default "Yes" in are-you-sure popup
S23='\p1\n'   # ENTER -> default "Select" in option-select popup
S24='\p1\n'   # dismiss file-error popup (any key)
S25='\p1\n'   # Section K final getch -> Section L
S26='\p2\n'   # Section L getch -> Section M (extra pause)
S27='\p1\n'   # Section M getch -> Section N (original)
S28='\p1\n'   # Section N getch#1 (original) -> N (saved/overwritten with ###)
S29='\p1\n'   # Section N getch#2 (overwritten) -> N (restored)
S30='\p1\n'   # Section N getch#3 (restored) -> Section O (printwrap)
S31='\p1\n'   # Section O getch -> Section P (filled)
S32='\p1\n'   # Section P getch#1 (filled) -> P (scroll-left done)
S33='\p1\n'   # Section P getch#2 (scroll-left) -> P (scroll-right done)
S34='\p1\n'   # Section P getch#3 (scroll-right) -> Section Q (lseek/file I/O)
S35='\p1\n'   # Section Q getch -> Done/halted screen

TK02="$S01$S02"
TK04="$TK02$S03$S04"
TK05="$TK04$S05"
TK06="$TK05$S06"
TK07="$TK06$S07"
TK08="$TK07$S08"
TK09="$TK08$S09"
TK10="$TK09$S10"
TK11="$TK10$S11"
TK12="$TK11$S12"
TK14="$TK12$S13$S14"
TK15="$TK14$S15"
TK16="$TK15$S16"
TK17="$TK16$S17"
TK18="$TK17$S18"
TK19="$TK18$S19"
TK25="$TK19$S20$S21$S22$S23$S24"
TKL="$TK25$S25"
TK26="$TKL$S26"
TK27="$TK26$S27"
TK28="$TK27$S28"
TK29="$TK28$S29"
TK30="$TK29$S30"
TK31="$TK30$S31"
TK32="$TK31$S32"
TK33="$TK32$S33"
TK34="$TK33$S34"
TK35="$TK34$S35"

echo ""
echo "Title screen"
DUMP=$(run_capture "" 8000000 title)
if [ ! -f "$DUMP" ]; then
    echo "  [FAIL] emulator did not produce a RAM dump at $DUMP"
    fail=$((fail+1))
else
    check_found "$DUMP" "title renders"       "LOCI FILE MANAGER V2 - LIBRARY TEST"
    check_found "$DUMP" "subtitle renders"    "Oscar64 bare-metal Oric Atmos build"
    check_found "$DUMP" "version line renders" "Version:"
    check_found "$DUMP" "first screen reached" "Press any key"

    echo ""
    echo "Build status checks"
    check_found "$DUMP" "CRT startup OK"      "[ OK ] oric-crt.c startup + regions"
    check_found "$DUMP" "charwin OK"          "[ OK ] charwin library initialized"
    check_found "$DUMP" "keyboard scanner OK" "[ OK ] keyboard scanner (VIA/AY)"
    check_found "$DUMP" "overlay RAM OK (LOCI present)" "[ OK ] overlay RAM (push/pop OK)"
    check_not_found "$DUMP" "no status check reports ERR" "[ERR]"
fi

echo ""
echo "Text input + key-echo loop"
DUMP=$(run_capture "$TK02" 11000000 textinput)
check_found "$DUMP" "textinput accepted (Entered:)" "Entered:"
check_found "$DUMP" "key-echo loop prompt shown" "Press any key for key-echo loop..."

echo ""
echo "Section A: colour palette"
DUMP=$(run_capture "$TK04" 13000000 section_a)
check_found "$DUMP" "Section A header" "A: Colour palette (8 ink x 8 paper)"
check_found "$DUMP" "Section A column header" "BLK RED GRN YEL BLU MAG CYN WHT"
check_found "$DUMP" "Section A row WHT" "WHT:BLKREDGRNYELBLUMAGCYNWHT"

echo ""
echo "Section B: inline ASTR attribute macros"
DUMP=$(run_capture "$TK05" 14000000 section_b)
check_found "$DUMP" "Section B header" "B: Inline attribute macros (ASTR)"
check_found "$DUMP" "Section B ink row" "Ink:   Red Grn Yel Cyn Mag Whi"
check_found "$DUMP" "Section B blink row" "Blink: blink norm"

echo ""
echo "Section C: fill rect concentric borders"
DUMP=$(run_capture "$TK06" 17000000 section_c)
check_found "$DUMP" "Section C header" "C: fill rect (concentric borders)"
check_found "$DUMP" "Section C border pattern" "*#+................................+#*"

echo ""
echo "Section D: animated cursor walk"
DUMP=$(run_capture "$TK07" 19000000 section_d)
check_found "$DUMP" "Section D header" "D: Animated cursor walk (perimeter)"
check_found "$DUMP" "Section D animation complete" "Press any key..."

echo ""
echo "Section E: getat char read-back"
DUMP=$(run_capture "$TK08" 20000000 section_e)
check_found "$DUMP" "Section E header" "E: getat char read-back test"
check_found "$DUMP" "Section E getat PASS" "GETAT: PASS"

echo ""
echo "Section F: text editing (insert/delete)"
DUMP=$(run_capture "$TK09" 22000000 section_f1)
check_found "$DUMP" "Section F header" "F: Text editing (insert/delete/line)"
check_found "$DUMP" "Section F insert/delete result" "Hello, World!"

echo ""
echo "Section F: printline auto-scroll"
DUMP=$(run_capture "$TK10" 22000000 section_f2)
check_found "$DUMP" "Section F printline label" "printline demo: 8 lines, auto-scroll"
check_found "$DUMP" "Section F printline reached row 8" "Printline row 8 of 8"

echo ""
echo "Section G: scroll speed bench"
DUMP=$(run_capture "$TK11" 24000000 section_g)
check_found "$DUMP" "Section G header" "G: Scroll speed bench (500 scrolls)"
check_found "$DUMP" "Section G bench completed" "Scroll bench: done. (fast!)"
check_found "$DUMP" "Section G final iteration" "S149 scroll done"

echo ""
echo "Section H: scroll-down demo"
DUMP=$(run_capture "$TK12" 27000000 section_h)
check_found "$DUMP" "Section H header" "H: Scroll-down demo (auto-scroll)"
check_found "$DUMP" "Section H completed" "60 down-scrolls: done."

echo ""
echo "Section I: bouncing ball (early-exit)"
DUMP=$(run_capture "$TK14" 28000000 section_i)
check_found "$DUMP" "Section I header" "I: Bounce ball (ESC/ENTER exits)"
check_found "$DUMP" "Section I early-exit frame count" "Done: 280 frames."

echo ""
echo "Section J: viewport map scrolling"
DUMP=$(run_capture "$TK15" 29000000 section_j)
check_found "$DUMP" "Section J header" "J: Viewport map (arrows, ESC=exit)"
check_found "$DUMP" "Section J initial position" "View x:0 y:0"
check_found "$DUMP" "Section J map row 0" "R00: 01234567890123456789012345678"

DUMP=$(run_capture "$TK16" 32000000 section_j_right)
check_found "$DUMP" "Section J scrolled right" "View x:1 y:0"

DUMP=$(run_capture "$TK17" 34000000 section_j_down)
check_found "$DUMP" "Section J scrolled down" "View x:1 y:1"

DUMP=$(run_capture "$TK18" 33000000 section_j_exit)
check_found "$DUMP" "Section J ESC exits loop" "Press any key..."
check_found "$DUMP" "Section J map scrolled (row offset)" "01: 012345678901234567890123456789"

echo ""
echo "Section K: menu system + popups"
DUMP=$(run_capture "$TK19" 35000000 section_k_menu)
check_found "$DUMP" "menu bar header renders" "LOCI File Manager"
check_found "$DUMP" "menu bar items render" "App File  Dir  Mounts  Info"
check_found "$DUMP" "Section K intro" "Navigate menu. ESC = exit."

DUMP=$(run_capture "$TK25" 40000000 section_k_popups)
check_found "$DUMP" "are-you-sure popup answer" "Answer: 1 (1=Yes 2=No)"
check_found "$DUMP" "option-select popup result" "Selection: 1 (0=cancel)"
check_found "$DUMP" "file-error popup simulated error" "Simulated error #42:"

echo ""
echo "Section L: LOCI library demo"
DUMP=$(run_capture "$TKL" 42000000 section_l)
check_found "$DUMP" "Section L header" "L: LOCI library demo"
check_found "$DUMP" "LOCI detected" "LOCI: Detected"
check_found "$DUMP" "firmware version shown" "Firmware: 1.1.254"
check_found "$DUMP" "root directory header" "Root directory:"
check_found "$DUMP" "IJK joystick status" "IJK joystick: not found"

echo ""
echo "Section M: cursor move/forward/back, chars"
DUMP=$(run_capture "$TK26" 42000000 section_m)
check_found "$DUMP" "Section M header" "M: Cursor move/forward/back, chars"
check_found "$DUMP" "Section M getat verify PASS" "getat verify:PASS"

echo ""
echo "Section N: get_rect/put_rect save+restore"
DUMP=$(run_capture "$TK27" 44000000 section_n1)
check_found "$DUMP" "Section N header" "N: getrect/putrect save+restore"
check_found "$DUMP" "Section N original content" "Original content drawn."
check_found "$DUMP" "Section N pattern row" "ABCDEFGHIJKLMNOPQRSTUVWXYZABCD"

DUMP=$(run_capture "$TK28" 46000000 section_n2)
check_found "$DUMP" "Section N rect saved+overwritten" "Rect saved. Overwriting with ### ..."
check_found "$DUMP" "Section N overwrite pattern visible" "CDEFG############TUVWXYZABCDEF"

DUMP=$(run_capture "$TK29" 48000000 section_n3)
check_found "$DUMP" "Section N rect restored" "Rect restored from buffer."
check_found "$DUMP" "Section N restored pattern visible" "CDEFGHIJKLMNOPQRSTUVWXYZABCDEF"

echo ""
echo "Section O: printwrap word-wrap demo"
DUMP=$(run_capture "$TK30" 50000000 section_o)
check_found "$DUMP" "Section O header" "O: printwrap word-wrap demo"
check_found "$DUMP" "Section O label" "Wrapping into 20-col window:"
check_found "$DUMP" "Section O wrapped text reaches last word" "function."

echo ""
echo "Section P: scroll_left/scroll_right"
DUMP=$(run_capture "$TK31" 50000000 section_p1)
check_found "$DUMP" "Section P header" "P: scroll left / scroll right"
check_found "$DUMP" "Section P rows filled" "Filled rows. Scroll left x3 by 3..."
check_found "$DUMP" "Section P filled pattern row" "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGH"

DUMP=$(run_capture "$TK32" 52000000 section_p2)
check_found "$DUMP" "Section P scroll-left done" "Left done."
check_found "$DUMP" "Section P scrolled-left pattern row" "JKLMNOPQRSTUVWXYZABCDEFGH"

DUMP=$(run_capture "$TK33" 54000000 section_p3)
check_found "$DUMP" "Section P scroll-right done" "Right done."

echo ""
echo "Section Q: LOCI lseek / file I/O smoke test"
DUMP=$(run_capture "$TK34" 56000000 section_q)
check_found "$DUMP" "Section Q header" "Q: LOCI lseek / file I/O test"
check_found "$DUMP" "Q1 create+write PASS" "Q1 CREATE+WRITE 32      PASS"
check_found "$DUMP" "Q2 seek_set/read PASS" "Q2 SEEK_SET=10/READ K   PASS"
check_found "$DUMP" "Q3 seek_cur+/read PASS" "Q3 SEEK_CUR+5=16/READ Q PASS"
check_found "$DUMP" "Q4 seek_cur-/read PASS" "Q4 SEEK_CUR-7=10/READ K PASS"
check_found "$DUMP" "Q5 seek_end-/read PASS" "Q5 SEEK_END-4=28/READ ] PASS"
check_found "$DUMP" "Q6 seek_end/eof PASS" "Q6 SEEK_END=32/EOF      PASS"
check_found "$DUMP" "Q7 close+unlink PASS" "Q7 CLOSE+UNLINK         PASS"
check_not_found "$DUMP" "no Section Q check reports FAIL" "FAIL"

echo ""
echo "Final screen"
DUMP=$(run_capture "$TK35" 58000000 done)
check_found "$DUMP" "library test complete + halted" "LIBRARY TEST COMPLETE. HALTED."

echo ""
echo "==========================================================="
echo "  Results: $pass passed, $fail failed, $skipped skipped"
echo "==========================================================="

if [ $fail -gt 0 ]; then
    exit 1
fi
exit 0
