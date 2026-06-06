# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

v2 rebuild of [locifilemanager](https://github.com/xahmol/locifilemanager) — a file manager for the LOCI mass storage device on Oric Atmos — rewritten using Oscar64 instead of CC65. The v1 CC65 implementation at `/home/xahmol/git/locifilemanager/` is the functional reference for all features, screen layouts, and UX behavior.

**Phase status:**
- Phase 1 — Oric platform research: ✅ complete
- Phase 2 — Buildchain + charwin library + keyboard: ✅ complete
- Phase 3 — LOCI library + localisation infrastructure + libdemo expansion: ✅ complete
- Phase 4 — Application rebuild (main, menu, dir, file, drive modules): 🔲 next
- Phase 5 — Localisation (EN/FR compile-time switch): merged into Phase 3 ✅

## Compiler Toolchain

This is an **Oscar64** project. `oscar64manual.md` is in the project root — consult it for compiler flags, pragmas, language extensions, and gotchas. The Oscar64 binary is at `/home/xahmol/oscar64/bin/oscar64`.

Target: 6502A (Oric Atmos), bare-metal. No VIC-II, SID, or C64 Kernal.

**Critical Oscar64 gotchas (all documented in oscar64manual.md):**
- `va_arg` is broken in native mode (`-n`) — do not use `<stdarg.h>` / `va_list` / `va_arg`.
- `#if MACRO` fails when macro is defined via `-d` flag (no value) — use `#ifdef MACRO`.
- `(type)struct.member` — Oscar64 applies cast before member access; use a temp variable.
- Macro that expands to volatile read in braces-free for-loop body causes parse error; use braced body with explicit temp.
- Ternary `? ptr : 0` in pointer-returning function — use `if`/`return` instead.

## Build and Run

```
make              # build build/locifm.tap  (Phase 3+ app stub)
make run          # build + launch in Oricutron
make libdemo      # build build/libdemo.tap (Phase 2 charwin/keyboard demo)
make libdemo-run  # build + launch libdemo in Oricutron
make clean        # remove build artifacts
```

Compiler flags: `-n -tf=bin -rt=include/oric_crt.c -i=include -O2 -dNOFLOAT`

Emulator: `/home/xahmol/oricutron/oricutron -ma --serial none --vsynchack off --turbotape on`

## Source Layout

```
src/
  main.c          Phase 3+ app entry point (hello-world stub for now)
  libdemo.c       Phase 2 demo — tests all charwin/keyboard functions
include/
  oric.h          Hardware constants (VIA, AY, screen, overlay RAM, ASTR_*/CH_*/A_* attrs)
  oric_crt.c      Custom Oscar64 runtime: startup, regions, math stubs
  keyboard.h/c    Direct VIA/AY keyboard scanner — no ROM calls
  charwin.h/c     Character window library (see below)
  strings.h       (Phase 5) Conditionally includes strings_en.h or strings_fr.h
build/
  locifm.tap      Main app tape image
  libdemo.tap     Demo tape image
tools/
  mktap.py        Wraps .bin in Oric tape header
```

## charwin Library (`include/charwin.h/c`)

Complete bare-metal Oric character window library. All functions are implemented and tested (except `cwin_push`/`cwin_pop` which require LOCI overlay RAM).

**Window model:** `OricCharWin` struct holds `sx,sy` (top-left, sx≥2), `wx,wy` (size), `cx,cy` (cursor), `ink`, `paper`. `cwin_clear` writes INK attr at screen col 0 and PAPER attr at screen col 1 for every managed row; content occupies cols `sx` through `sx+wx-1`.

**Key functions:**

| Function | Purpose |
|---|---|
| `charwin_init()` | Build row-address lookup table — call once before any cwin_* |
| `cwin_init(w, sx,sy,wx,wy,ink,paper)` | Initialise window struct |
| `cwin_clear(w)` | Fill with spaces, write INK+PAPER attrs, reset cursor |
| `cwin_putat_char/string/printf(w,x,y,...)` | Positional write (no cursor update) |
| `cwin_put_char/string/attr/printf(w,...)` | Cursor-advancing write |
| `cwin_putat_dblhi_string(w,x,y,s)` | Double-height: writes A_STD2H+s+A_STD on row y AND y+1 from one call |
| `cwin_console_put_char/string/printf(w,...)` | Console mode: handles `\n`, wraps, scrolls |
| `cwin_fill_rect(w,x,y,bw,bh,ch)` | Fill rectangle with character |
| `cwin_scroll_up(w)` | Scroll window content up 1 row |
| `cwin_cursor_show(w,on)` | Toggle cursor via inverse-video |
| `cwin_cursor_left/right/up/down(w)` | Move cursor; returns false at edge |
| `cwin_getat_char(w,x,y)` | Read character from screen RAM |
| `cwin_getch()` | Blocking key read |
| `cwin_textinput(w,x,y,vwidth,str,maxlen,validation)` | Text-input widget with viewport scroll |
| `cwin_push(w)` / `cwin_pop(w)` | Save/restore rows to overlay RAM — **LOCI required** |

**Printf format support:** `%d` (int16), `%u` (uint16), `%x` (uint16 hex), `%s`, `%c`, `%%`, width+zero-fill (e.g. `%02u`). No floats (`-dNOFLOAT`).

**Inline attribute macros** (`oric.h`):
- `ASTR_INK_RED` … `ASTR_INK_WHITE` — embed ink-change attrs in string literals (positions 0x01–0x07)
- `ASTR_PAPER_BLACK` … `ASTR_PAPER_WHITE` — embed paper-change attrs (0x10–0x17)
- `ASTR_CHARSET_STD/ALT/STD2H/BLKSTD` etc. — charset mode attrs (0x08–0x0F)
- `A_FWBLACK` (= 0x00 = NUL) cannot be embedded in C string literals — use `cwin_put_attr(&w, A_FWBLACK)` instead
- `ASTR_CHARSET_STD2H` (= `\x0A`) conflicts with `\n` in console-mode output — use only with `cwin_putat_string` (positional), not `cwin_console_put_string`

**Character constants:**
- `CH_SPACE` (0x20) — blank, safe for clearing
- `CH_INVSPACE` (0xA0) — solid ink-colored block (used for cursor and animation heads)
- `CH_POUND` (0x5F) — £ sign (Oric ROM maps underscore position — avoid `_` in display strings)
- `CH_COPYRIGHT` (0x60) — © sign (avoid backtick in display strings)

## Keyboard Scanner (`include/keyboard.h/c`)

Direct VIA/AY scan — no ROM calls. Key functions:
- `keyb_scan()` — populate `keyb_matrix[8]`; ~370 cycles at 1 MHz
- `keyb_poll()` — scan + decode + repeat/debounce; returns key code or `KEY_NONE`
- `keyb_getch()` — blocking; loops `keyb_poll()` until non-zero
- `keyb_check()` — non-blocking equivalent of `keyb_poll()`

Repeat: `REP_DELAY=3000` polls (~1.1 s), `REP_RATE=300` polls (~110 ms). Release debounce: `RELEASE_DEBOUNCE=20` no-key polls required after each accepted keypress.

Key codes: `KEY_ESC`, `KEY_ENTER`, `KEY_DEL`, `KEY_UP/DOWN/LEFT/RIGHT`, `KEY_F1`–`KEY_F10`, `KEY_NONE=0`. Modifiers: `MOD_SHIFT`, `MOD_CTRL`, `MOD_FUNCT`, `MOD_CAPSLOCK` in `keyb_modifiers`.

Animation delay without `sleep`: `for (uint8_t d = 0; d < N; d++) keyb_scan();` — each call is ~370 cycles (hardware register accesses prevent optimization).

## Oric Screen Model

- **40×28 character grid**, screen RAM at `$BB80`
- **Serial attributes:** byte with `(byte & 0x60) == 0` is an attribute (values 0x00–0x1F only). `0x20` (space) is a character, not an attribute. ULA resets to white-ink/black-paper at start of each raster line, then applies attrs left-to-right.
- **Attr bytes occupy one screen column** (display as paper-colour box) — when embedding `ASTR_*` in strings, each attr shifts subsequent text one column right.
- **No per-character colour:** ink attr at column X affects all characters to its right on that row. Changing colour per character requires writing an attr byte before each char (2 columns per coloured char).
- **Inverse video:** `ch | 0x80` inverts pixel rendering; `CH_INVSPACE` (0xA0) = solid ink-coloured block.
- **Double-height:** `ASTR_CHARSET_STD2H` on row N shows top halves; same attr+content on row N+1 shows bottom halves. Use `cwin_putat_dblhi_string()` to write both rows in one call.

## LOCI Hardware API (Phase 3 target)

The LOCI device exposes two memory-mapped register blocks:

### MIA — Mass Interface Adapter at `$03A0`

```c
struct __LOCI_MIA {
    const unsigned char ready;   // +0: TX/RX ready bits
    unsigned char tx;            // +1: transmit byte
    const unsigned char rx;      // +2: receive byte
    const unsigned char vsync;   // +3: vsync flag
    unsigned char rw0;           // +4: DMA channel 0 R/W
    unsigned char step0;         // +5: DMA step
    unsigned int  addr0;         // +6: DMA address (16-bit)
    unsigned char rw1;           // +8: DMA channel 1
    unsigned char step1;         // +9
    unsigned int  addr1;         // +A
    unsigned char xstack;        // +C: extended stack
    unsigned char errno_lo;      // +D
    unsigned char errno_hi;      // +E
    unsigned char op;            // +F: operation code
    unsigned char irq;           // +10
    const unsigned char spin;    // +11
    const unsigned char busy;    // +12: busy flag ($80)
    const unsigned char lda;     // +13: LDA opcode in ROM
    unsigned char a;             // +14: accumulator
    const unsigned char ldx;     // +15: LDX opcode in ROM
    unsigned char x;             // +16: X register
    const unsigned char rts;     // +17: RTS opcode in ROM
    unsigned int  sreg;          // +18: status register
};
#define MIA (*(volatile struct __LOCI_MIA *)0x03A0)
```

### TAP — Tape controller at `$0315`

```c
struct __LOCI_TAP {
    unsigned char cmd;    // +0: command (PLAY=1, REC=2, REW=3, BIT=4, FFW=5)
    unsigned char status; // +1
    unsigned char data;   // +2
};
#define TAP (*(volatile struct __LOCI_TAP *)0x0315)
```

### XRAM and Overlay RAM

- **XRAM** — accessed via MIA DMA channels; for large file copy buffers. Functions needed: `xram_memcpy_to`, `xram_memcpy_from`, `xram_poke`, `xram_peek`.
- **Overlay RAM** (`$C000–$FFFF`) — enabled via `MICRODISCCFG` ($0314) = `$FD`; requires LOCI. `cwin_push`/`cwin_pop` already use this. Functions needed for Phase 3: `enable_overlay_ram`, `disable_overlay_ram`.
- **Not testable in Oricutron** — all MIA/TAP/overlay features require real LOCI hardware.

### High-Level LOCI Routines (to port from v1 `libsrc/`)

- `get_locicfg()` — populate `locicfg` (device count, firmware version, `uname`)
- `loci_check_fw(major, minor, patch)` — version gate; exits if firmware too old
- `get_loci_devname(devid, maxlength)` — returns drive label string
- File ops: `file_copy`, `file_save`, `file_load`, `file_exists`
- Directory ops: `opendir`, `readdir`, `closedir`, `seekdir`, `rewinddir`
- IJK joystick: `ijk_detect`, `ijk_read`, `ijk_present`, `ijk_ljoy`, `ijk_rjoy`

### LOCI Filesystem Types

```c
#define DIR_ATTR_RDO 0x01
#define DIR_ATTR_SYS 0x04
#define DIR_ATTR_DIR 0x10
```

Recognized extensions: `.DSK`, `.TAP`, `.ROM`, `.LCE`.

## Phase 4 — Application Architecture

Port these modules from v1 (`/home/xahmol/git/locifilemanager/src/`):

| Module | Purpose |
|---|---|
| `main.c` | Main loop, event dispatch, configuration state |
| `menu.c/h` | Pulldown menu system; returns `menubarchoice * 10 + menuoptionchoice` |
| `dir.c/h` | Directory pane rendering, navigation, selection |
| `file.c/h` | File ops: copy, move, delete, rename, tape browse |
| `drive.c/h` | LOCI drive enumeration, mount/unmount, boot |

Menu system: follows `~/.claude/menu_conventions.md` (Oric CC65 section). Key conventions: LIFO window stack (`cwin_push`/`cwin_pop`), `[X]` keyboard hints, four standard popup helpers.

**On exit:** boot from active mounts (disk > tape > ROM). This boot preference is the primary output of the application.

**IJK joystick:** Raxiss IJK at fixed I/O. Directions → cursor keys, fire → RETURN.
```c
#define IJK_JOY_LEFT  0x02
#define IJK_JOY_RIGHT 0x01
#define IJK_JOY_UP    0x10
#define IJK_JOY_DOWN  0x08
#define IJK_JOY_FIRE  0x04
```

## Phase 5 — Localisation (EN/FR)

**Compile-time switch, two binaries.** `make` → EN default; `make LANG=FR` → FR; `make all-langs` → both. Oscar64 receives `-dLANG_FR` flag.

Architecture:
```
include/strings.h      — #if LANG_FR → strings_fr.h, else strings_en.h
include/strings_en.h   — all MSG_* defines in English
include/strings_fr.h   — all MSG_* defines in French (unaccented — Oric ROM has no é è à ç)
```

**All user-visible strings must be `MSG_*` macros from Phase 3 day one.** No raw string literals in logic files. Retroactive extraction is painful.

French without accents: "Deplacer", "Effacer", "Renommer", "Oui", "Non" — consistent with historic Oric French software.
