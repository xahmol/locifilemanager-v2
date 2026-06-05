# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

v2 rebuild of [locifilemanager](https://github.com/xahmol/locifilemanager) — a file manager for the LOCI mass storage device on Oric Atmos — rewritten using Oscar64 instead of CC65. The v1 CC65 implementation at `/home/xahmol/git/locifilemanager/` is the functional reference for all features, screen layouts, and UX behavior.

## Compiler Toolchain

This is an **Oscar64** project. Per global instructions, `oscar64manual.md` is in the project root — consult it for compiler flags, pragmas, library APIs, and gotchas. The Oscar64 binary is at `/home/xahmol/oscar64/bin/oscar64`.

The target CPU is a 6502A (Oric Atmos), so Oscar64 must be configured for bare-metal Oric output — **not** the default C64 target. This means custom startup/linker configuration will be required; no VIC-II, SID, or C64 Kernal should be assumed.

## Build and Run

Build system is not yet defined. When creating the Makefile, follow the conventions in `~/.claude/makefile_conventions.md`. Key paths:

- **Emulator:** `/home/xahmol/oricutron/oricutron` — run with `-ma` (Atmos mode), `--serial none`, `--vsynchack off`, `--turbotape on`
- **Output:** Tape image `.tap` in `build/`
- **make run** — should launch Oricutron with the built tape image

```makefile
# Expected targets (to be filled in):
make          # build locifm.tap
make run      # build and launch in Oricutron
make clean    # remove build artifacts
```

## Architecture

Planned source layout mirrors the v1 structure. Port these modules from v1 (`/home/xahmol/git/locifilemanager/src/`):

| Module | Purpose |
|---|---|
| `main.c` | Main loop, event dispatch, configuration state |
| `generic.c/h` | Screen output, input (`getkey`), windowing (save/restore), wait helpers |
| `menu.c/h` | Pulldown menu system; returns `menubarchoice * 10 + menuoptionchoice` |
| `dir.c/h` | Directory pane rendering, navigation, selection |
| `file.c/h` | File ops: copy, move, delete, rename, tape browse |
| `drive.c/h` | LOCI drive enumeration, mount/unmount, boot |

Menu system design follows `~/.claude/menu_conventions.md` (Oric CC65 section is the closest reference). Key conventions: LIFO window stack, `menu_main()` encoded return, `[X]` keyboard hint notation in menu strings, four standard popup helpers.

## LOCI Hardware API

The LOCI device exposes two memory-mapped register blocks that must be re-implemented for Oscar64 (v1 version in `libsrc/` used CC65 `__fastcall__` and `.s` assembly):

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

- **XRAM** (Extended RAM) — accessed via MIA DMA channels; used for large file copy buffers (v1 used `$8000`/`0x0800` bytes). Functions needed: `xram_memcpy_to`, `xram_memcpy_from`, `xram_poke`, `xram_peek`.
- **Overlay RAM** (`$C000–$FFFF`) — RAM normally shadowed by Atmos ROM; accessed via bank-switch. v1 uses it for the window save stack. Functions needed: `enable_overlay_ram`, `disable_overlay_ram`.

### High-Level LOCI Routines

Port these from `libsrc/` and `include/loci.h` in v1:
- `get_locicfg()` — populate `locicfg` (device count, firmware version, `uname`)
- `loci_check_fw(major, minor, patch)` — version gate; exits if firmware too old
- `get_loci_devname(devid, maxlength)` — returns drive label string
- File ops: `file_copy`, `file_save`, `file_load`, `file_exists`
- Directory ops: `opendir`, `readdir`, `closedir`, `seekdir`, `rewinddir`
- IJK joystick: `ijk_detect`, `ijk_read`, `ijk_present`, `ijk_ljoy`, `ijk_rjoy`

### LOCI Filesystem Types (for directory filtering)

```c
// d_attrib flags from readdir()
#define DIR_ATTR_RDO 0x01
#define DIR_ATTR_SYS 0x04
#define DIR_ATTR_DIR 0x10
```

Recognized file extensions: `.DSK` (disk image), `.TAP` (tape image), `.ROM` (ROM image), `.LCE` (LOCI executable — not yet implemented in v1).

## Oric Atmos Screen Model

- **40×28 character grid** (40 columns, 28 rows)
- **Color via serial attributes** — each attribute byte in screen RAM changes fg/bg color for the rest of the line; no per-character color. This affects window rendering: clearing a window must also clear/reset attribute bytes.
- No box-drawing characters with consistent rendering — use inverse-space fill for borders (as v1 does).
- Screen RAM layout: interleaved characters and attributes in a fixed pattern.

## Configuration Preserved at Exit

On exit the program boots from the active mounts (disk image > tape image > ROM image). This boot preference is the primary output of the application, not a file on disk.

## IJK Joystick Interface

The Raxiss IJK interface connects at a fixed I/O address; directions translate 1:1 to cursor keys, fire button = RETURN. `getkey(ijk_present)` in `generic.c` handles unified input. IJK flags:

```c
#define IJK_JOY_LEFT  0x02
#define IJK_JOY_RIGHT 0x01
#define IJK_JOY_UP    0x10
#define IJK_JOY_DOWN  0x08
#define IJK_JOY_FIRE  0x04
```
