# ARCHITECTURE.md

Technical architecture reference for **locifilemanager-v2** — a bare-metal
file manager for the LOCI mass-storage device on the Oric Atmos, built with
the Oscar64 C compiler. This document describes the memory map, the main
data structures, the module layout, and the application's runtime control
flow. For compiler gotchas and day-to-day build/test commands, see
`CLAUDE.md` and `oscar64manual.md`.

---

## 1. Technical Setup

### 1.1 Target platform

- **CPU:** 6502A @ 1 MHz (Oric Atmos)
- **No ROM calls** — bare-metal. No VIC-II/SID/Kernal (those are C64 concepts
  and do not apply here). Screen, keyboard, sound (AY-3-8912) and storage
  (LOCI) are all driven directly via memory-mapped registers.
- **No interrupts.** Startup executes `SEI` and never `CLI`s. The keyboard
  scanner is pure-poll, so no IRQ handler is installed. Any code that must
  briefly enable interrupts (overlay-RAM access, IJK VIA Port A access) uses
  `PHP`/`SEI`/`PLP`, never `SEI`/`CLI` — see §2.6.

### 1.2 Toolchain

- **Compiler:** [Oscar64](https://github.com/drmortalwombat/oscar64),
  native mode (`-n`), binary target (`-tf=bin`).
- **Custom runtime:** `include/oric_crt.c` (passed via `-rt=`) — defines
  memory regions, the startup routine, and pulls in `crt_math.c` for
  integer/float runtime helpers.
- **Flags:** `-n -tf=bin -rt=include/oric_crt.c -i=include -O2 -dNOFLOAT`,
  plus `-dVERSION_MAJOR/MINOR/PATCH/TIMESTAMP` and `-dLANG_FR` (French build
  only).
- **Tape wrapping:** `tools/mktap.py` wraps the Oscar64 `.bin` output in an
  Oric tape header to produce `.tap` files.
- **Emulator:** Oricutron (`make run`/`make libdemo-run`) — LOCI features
  (file I/O, mount, overlay RAM) are **not** emulated by Oricutron and
  require real hardware.
- **Headless test emulator:** [Phosphoric](https://github.com/benedictemarty/Phosphoric)
  (`oric1-emu`) — emulates a LOCI device backed by a host-filesystem sandbox
  (`--loci-flash`), with `--type-keys` auto-typing and `--dump-ram-at`
  screen capture. This is what `make test*` runs against (§7).

### 1.3 Build artifacts

| Target | Output |
|---|---|
| `make` | `build/locifm.tap` (EN) |
| `make LANG=FR` | `build/locifm_fr.tap` (FR) |
| `make libdemo` | `build/libdemo.tap` — exercises charwin/keyboard/LOCI/IJK library |
| `make all-langs` | all four `.tap` images (app + libdemo, EN + FR) |
| `make usb` | copies all `.tap` images to a USB stick path from `.env` |

### 1.4 Localisation (EN/FR)

Compile-time switch, two binaries — no runtime language switching.

```
src/strings.h     -- #ifdef LANG_FR -> strings_fr.h, else strings_en.h
src/strings_en.h  -- all MSG_* defines, English
src/strings_fr.h  -- all MSG_* defines, French (unaccented: Oric ROM charset
                     has no é/è/à/ç — "Deplacer", "Effacer", "Oui", "Non")
```

All user-visible strings are `MSG_*` macros — no raw string literals in
logic files. `src/strings_demo*.h` is the equivalent pair for `libdemo.c`.

---

## 2. Memory Map

The Oric Atmos has a 16-bit (64 KB) address space. locifilemanager-v2 uses a
custom Oscar64 runtime (`include/oric_crt.c`) that carves it up as follows:

### 2.1 Main CPU address space ($0000–$FFFF)

| Range | Size | Contents |
|---|---|---|
| `$0000–$00FF` | 256 B | Zero page — Oscar64 internal register file (`__ip`, `__sp`, `__fp`, `__accu`, `__addr`, `__tmp`, `__tmpy`, etc.) |
| `$0100–$01FF` | 256 B | 6502 hardware stack (`txs #$FF` at startup) |
| `$0200–$030F` | — | Oric ROM system variables / VIA 6522 (see §2.2) — not used directly except where noted |
| `$0245–$0246` | 2 B | IRQ vector low/high (ROM chains through here — unused; IRQs stay disabled) |
| `$0300–$030F` | 16 B | **VIA 6522** registers (see §2.2) |
| `$0314` | 1 B | `MICRODISCCFG` — overlay-RAM enable (Microdisc-compatible register) |
| `$0315–$0317` | 3 B | **LOCI TAP** tape controller (see §2.3) |
| `$0319` | 1 B | LOCI presence signature — ROM/LOCI places `'L'` here when LOCI is active |
| `$03A0–$03B9` | 26 B | **LOCI MIA** (Mass Interface Adapter) register block (see §2.3) |
| `$0500–$057F` | 128 B | **Startup region** — tape entry point jumps directly into `oric_startup` here |
| `$0580–$B1FF` | ~42.4 KB | **Main program region** — code, data, BSS, heap |
| `$B200–$B3FF` | 512 B | **Software stack** (`#pragma stacksize(0x0200)`) — shared by the entire call chain |
| `$B400–$B7FF` | 1 KB | Character set RAM — **standard** charset (left untouched by code/data/stack so the version-splash glyphs survive) |
| `$B800–$BB7F` | ~896 B | Character set RAM — **alternate** charset |
| `$BB80–$BFFF` | 1120 B | **Text screen RAM** — 40 cols × 28 rows |
| `$C000–$FFFF` | 16 KB | Oric ROM (normally); **overlay RAM** when LOCI enables it (see §2.5) |

> **Hard ceiling:** `$B400` is OSDK's documented start of charset RAM and the
> hard ceiling for the program/data/stack region. Code, data, BSS, heap and
> the software stack must all stay below `$B400`.

### 2.2 VIA 6522 ($0300–$030F)

| Offset | Register | Notes |
|---|---|---|
| `$0300` | `prb` | Port B: bits 0–2 = keyboard row select, bit 3 = tape/kbd sense (input), bit 4 = printer strobe (IJK select), bit 5 = cassette motor, bit 6 = AY BC1, bit 7 = AY BDIR |
| `$0301` | `pra` | Port A (with handshake) — AY data bus / printer data |
| `$0302` | `ddrb` | DDR B (ROM sets `$F7`) |
| `$0303` | `ddra` | DDR A |
| `$0304–$0305` | `t1lo/t1hi` | Timer 1 counter |
| `$0306–$0307` | `t1llo/t1lhi` | Timer 1 latch (100 Hz latch ≈ 9984, **unused** — IRQs disabled) |
| `$0308–$0309` | `t2lo/t2hi` | Timer 2 |
| `$030A` | `sr` | Shift register |
| `$030B` | `acr` | Auxiliary control register |
| `$030C` | `pcr` | Peripheral control register — CB2 drives AY keyboard column strobe |
| `$030D` | `ifr` | Interrupt flag register (bit 6 = Timer 1) — cleared before `mia_call_boot` jump |
| `$030E` | `ier` | Interrupt enable register |
| `$030F` | `pra2` | Port A without handshake — AY data bus writes, **and IJK joystick read** |

AY-3-8912 register access is via the VIA Port B BC1/BDIR latch sequence
(`oric.h` comment block). Relevant AY registers: `AY_REG_MIXER` (7),
`AY_REG_IOA` (14, keyboard column drive), `AY_REG_IOB` (15, unused).

### 2.3 LOCI device registers

**MIA — Mass Interface Adapter at `$03A0`** (26 bytes, struct `__LOCI_MIA`):

| Offset | Field | Purpose |
|---|---|---|
| `+00 $03A0` | `ready` (RO) | TX/RX ready bits |
| `+01 $03A1` | `tx` | Transmit byte to MIA firmware |
| `+02 $03A2` | `rx` (RO) | Receive byte |
| `+03 $03A3` | `vsync` (RO) | Vsync flag |
| `+04 $03A4` | `rw0` | DMA channel 0 R/W data |
| `+05 $03A5` | `step0` | DMA channel 0 address step |
| `+06 $03A6` | `addr0` (u16) | DMA channel 0 XRAM address |
| `+08 $03A8` | `rw1` | DMA channel 1 R/W data |
| `+09 $03A9` | `step1` | DMA channel 1 address step |
| `+0A $03AA` | `addr1` (u16) | DMA channel 1 XRAM address |
| `+0C $03AC` | `xstack` | XSTACK — parameter/result exchange byte |
| `+0D $03AD` | `errno_lo` | Error code low byte |
| `+0E $03AE` | `errno_hi` | Error code high byte |
| `+0F $03AF` | `op` | Operation code — write to invoke (see §2.4) |
| `+10 $03B0` | `irq` | Interrupt control |
| `+11 $03B1` | `spin` (RO) | `JSR` here to execute the firmware's wait/result routine |
| `+12 $03B2` | `busy` (RO) | Bit 7 set while operation runs |
| `+13 $03B3` | `lda_op` (RO) | `$A9` (LDA #imm, part of result routine) |
| `+14 $03B4` | `areg` | A-register argument/result low byte |
| `+15 $03B5` | `ldx_op` (RO) | `$A2` (LDX #imm) |
| `+16 $03B6` | `xreg` | X-register argument/result high byte |
| `+17 $03B7` | `rts_op` (RO) | `$60` (RTS) |
| `+18 $03B8` | `sreg` (u16) | Upper 16 bits of a 32-bit result/argument |

**TAP — Tape controller at `$0315`** (3 bytes, struct `__LOCI_TAP`):
`cmd` (`$0315`), `status` (`$0316`), `data` (`$0317`).
Commands: `PLAY=1`, `REC=2`, `REW=3`, `BIT=4`, `FFW=5`.

**Overlay-RAM enable — `MICRODISCCFG` at `$0314`:** write `$FD` (`OVERLAY_ON`)
to map 16 KB of RAM under the ROM at `$C000–$FFFF`; write `$FF`
(`OVERLAY_OFF`) to restore ROM. Requires LOCI; not available in Oricutron.

**Presence signature — `$0319`:** `loci_present()` checks for `'L'` here.

### 2.4 MIA operation codes

Selected groups (full list in `include/loci.h`):

| Group | Codes |
|---|---|
| System | `ZXSTACK 0x00`, `XREG 0x01`, `PHI2 0x02`, `CODEPAGE 0x03`, `LRAND 0x04`, `STDIN_OPT 0x05` |
| Clock | `CLOCK_GETRES 0x10`, `CLOCK_GETTIME 0x11`, `CLOCK_SETTIME 0x12`, `CLOCK_GETTIMEZONE 0x13` |
| File I/O | `OPEN 0x14`, `CLOSE 0x15`, `READ_XSTACK 0x16`, `READ_XRAM 0x17`, `WRITE_XSTACK 0x18`, `WRITE_XRAM 0x19`, `LSEEK 0x1A`, `UNLINK 0x1B`, `RENAME 0x1C` |
| Directory | `OPENDIR 0x80`, `CLOSEDIR 0x81`, `READDIR 0x82`, `MKDIR 0x83`, `GETCWD 0x88` |
| Mount/tape | `MOUNT 0x90`, `UMOUNT 0x91`, `TAP_SEEK 0x92`, `TAP_TELL 0x93`, `TAP_HDR 0x94`, `UNAME 0x98` |
| Boot/tune | `BOOT 0xA0`, `TUNE_TMAP 0xA1`…`TUNE_SCAN 0xA6` |
| Exit | `EXIT 0xFF` |

**Calling convention** (matches CC65 v1 `mia.s` byte ordering):
- Push int16: high byte first to `XSTACK`, then low byte. Pop: low byte then
  `hi<<8 | lo`.
- Push int32: byte3 (MSB) → byte0 (LSB). Pop also reads `sreg` for the upper
  16 bits.
- `mia_call_int`/`mia_call_long`: write opcode to `MIA.op`, `CLV`, then
  **execute** `JSR MIA.spin` ($03B1) — `$03B1` is itself the firmware's
  `BVC` opcode of a `CLV;BVC -2` busy-loop that the firmware overwrites with
  `CLV;BVC+0;LDA #lo;LDX #hi;RTS` once done. `CLV` before the `JSR` is
  required because the loop's correctness depends on V being clear on entry.
- `mia_call_boot` (op `BOOT` only) is special: the "done" sequence is
  `CLV;BVC+0;JMP ($FFFC)` (jumps to the reset vector, never returns on
  success). It polls `MIA.busy` as data (not via `JSR spin`), clears
  `VIA.ifr`/`VIA.ier` to avoid a stale Timer-1 IRQ firing the instant the
  booted ROM enables interrupts, then `JSR MIA.spin` to take the jump. On
  failure it returns a result like `mia_call_int`.
- Errors: result `< 0` → `loci_errno = MIA.errno_lo`, function returns `-1`.

### 2.5 XRAM (LOCI extended RAM) and Overlay RAM

**XRAM** is a separate address space (not the 6502's), accessed only via MIA
DMA channels (`xram_peek`/`xram_poke`/`xram_memcpy_to`/`xram_memcpy_from`).
Used for large buffers that don't fit in the ~42 KB main RAM region:

| XRAM range | Size | Purpose |
|---|---|---|
| `$8000–$87FF` | 2 KB (`COPYBUF_XRAM_SIZE`) | `COPYBUF_XRAM_ADDR` — file copy/move transfer buffer |
| `$8800–$93FF` | 0xC00 (3 KB, `DIRSIZE`) | `DIR1BASE` — pane 0 directory linked list (`struct DirElement` entries) |
| `$9400–$9FFF` | 0xC00 (3 KB) | `DIR2BASE` — pane 1 directory linked list |

**Overlay RAM** (`$C000–$FFFF`, 16 KB, enabled via `MICRODISCCFG=$FD`) is
partitioned by software convention between two consumers:

| Overlay range | Size | Owner |
|---|---|---|
| `$C000–$E2FF` | 0x2300 (8960 B) | `cwin_push`/`cwin_pop` — worst case 8 depths × 28 rows × 40 bytes |
| `$E300–...` | grows upward | `MENU_OVERLAY_BASE` — menu/popup window-save stack (`menu_winsave`/`menu_winrestore`), up to `MENU_WIN_DEPTH=9` nested saves, each `height × 40` bytes |

Both regions require real LOCI hardware — Oricutron does not map overlay
RAM, so `menu_winsave`/`cwin_push` increment their stack-depth bookkeeping
but skip the actual copy when `!loci_present()`.

### 2.6 Interrupt policy

Startup (`oric_startup` in `oric_crt.c`) executes `SEI` and never `CLI`s —
the keyboard scanner is pure-poll (`keyb_scan`/`keyb_poll`), so no IRQ
handler is installed and the ROM's IRQ chain at `$0245/$0246` is never
entered. **Convention:** any code that must transiently enable interrupts
(overlay-RAM access via `MICRODISCCFG`, or VIA Port A access in `ijk.c`)
brackets it with `PHP`/`SEI` ... `PLP`, never `SEI`/`CLI` — an unconditional
`CLI` would permanently re-enable IRQs (since no handler exists) and let the
stock ROM IRQ handler corrupt zero page / screen RAM on every frame
thereafter.

### 2.7 Screen model

- **40×28 character grid**, screen RAM at `$BB80` (1120 bytes, `$BB80–$BFFF`).
- A screen byte with `(byte & 0x60) == 0` (values `$00–$1F`) is a **serial
  attribute** (changes ink/paper/charset for the rest of the raster line,
  occupies one screen column as a paper-coloured box); `$20` (space) is a
  normal character. The ULA resets to white-ink/black-paper at the start of
  each raster line.
- Ink attrs `$00–$07` (`A_FW*`), paper attrs `$10–$17` (`A_BG*`), charset
  mode attrs `$08–$0F` (`A_STD`, `A_ALT`, `A_STD2H`, …).
- Inverse video: `ch | 0x80`. `CH_INVSPACE` (`0xA0`) = solid ink-coloured
  block (cursor / progress-bar segments).

---

## 3. Source Layout

```
src/
  main.c          Application entry point: main loop, event dispatch, config state
  menu.c/h        Pulldown menu system, popups, LIFO window stack
  dir.c/h         Two-pane directory browser: rendering, navigation, selection,
                  name filter, recursive delete, properties popup
  file.c/h        File ops: copy, move (recursive), delete, rename, tape browse
  drive.c/h       LOCI drive enumeration, mount/unmount, boot-mode flags
  recurse.c/h     Iterative depth-first directory walker (recurse_walk())
  viewer.c/h      Full-screen paged, word-wrapped text file viewer
  input.c/h       Shared keyboard/joystick input helper (fm_getkey)
  strings.h       #ifdef LANG_FR -> strings_fr.h, else strings_en.h
  strings_en.h    All MSG_* string defines (English)
  strings_fr.h    All MSG_* string defines (French)
  splash_data.h   Boot/version-screen logo + GitHub QR code bitmap data
  libdemo.c       Library demo -- exercises every charwin/keyboard/loci/ijk function

include/
  oric.h          Hardware constants: VIA, AY sequences, screen, overlay RAM,
                  ASTR_*/CH_*/A_* attribute and character constants
  oric_crt.c      Custom Oscar64 runtime: regions, startup, IRQ policy
  crt_math.c      Integer/float runtime helpers required by Oscar64
  keyboard.h/c    Direct VIA/AY keyboard scanner (no ROM calls)
  charwin.h/c     Character window library (windows, viewports, text input)
  loci.h/c        LOCI mass-storage API (MIA/TAP/XRAM/file/dir/mount/tape)
  ijk.h/c         Raxiss IJK joystick driver (VIA Port A; independent of LOCI)
  strings_demo*.h Demo-only string defines (libdemo.c)

build/
  locifm.tap, locifm_fr.tap     Main app tape images
  libdemo.tap, libdemo_fr.tap   Library demo tape images

tools/
  mktap.py        Wraps Oscar64 .bin output in an Oric tape header
  gen_qr.js       Generates the GitHub QR code bitmap for splash_data.h

tests/
  scripts/        Phosphoric-driven regression test scripts (see §7)
  fixtures/       Sandbox seed filesystem for tests
  sandbox/        Working copy of fixtures + built .tap (regenerated per run)
  out/            RAM dumps / screenshots from test runs
```

---

## 4. Main Data Structures

### 4.1 `OricCharWin` and `OricViewport` (`include/charwin.h`)

```c
typedef struct {
    uint8_t sx, sy;   // start col (>=2), start row
    uint8_t wx, wy;   // width, height in characters
    uint8_t cx, cy;   // cursor position within window (0-based)
    uint8_t ink;      // A_FW* ink colour
    uint8_t paper;    // A_BG* paper colour
} OricCharWin;
```

Window convention: screen columns 0–1 of every managed row hold the
INK/PAPER attribute bytes; content occupies columns `sx … sx+wx-1`.
`charwin_init()` precomputes a row-address lookup table used by all `cwin_*`
functions.

```c
typedef struct {
    uint8_t     *sourcebase;   // flat source character map (row-major)
    uint16_t     sourcewidth;  // bytes per row in source (>= win->wx)
    uint16_t     sourceheight; // total rows in source
    uint16_t     viewx, viewy; // current scroll offset
    OricCharWin *win;          // target display window
} OricViewport;
```

Used by the text viewer (§6.8) to scroll a large in-memory page buffer
through a fixed-size window.

### 4.2 Menu system (`src/menu.h`)

```c
typedef struct {
    uint16_t overlay_addr;  // overlay-RAM address where rows were saved
    uint8_t  ypos;          // first screen row saved
    uint8_t  height;        // number of rows saved (each = 40 bytes)
} MenuWinRecord;

typedef struct {
    char    titles[MENUBAR_MAXOPTIONS][MENUBAR_MAXLENGTH]; // "App","File",...
    uint8_t xstart[MENUBAR_MAXOPTIONS]; // screen col of each bar item's highlight
    uint8_t ypos;                        // screen row the bar is drawn on
} MenuBar;
```

Global tables (defined in `menu.c`, mutated by `main.c` for dynamic entries):

```c
extern MenuBar menubar;
extern char    pulldown_options[PULLDOWN_NUMBER];                       // option count per pulldown
extern char    pulldown_titles[PULLDOWN_NUMBER][PULLDOWN_MAXOPTIONS][PULLDOWN_MAXLENGTH];
```

`PULLDOWN_NUMBER = 11`: indices 0–5 are the six menu-bar pulldowns
(App/File/Dir/Mounts/Tools/Info); indices 6–10 are popup sub-menus
(Enter-action, Filter, Target-drive, Mount-source, Yes/No) opened via
`menu_option_select()`/`menu_areyousure()`/`menu_confirm_file()`.

A private stack `static MenuWinRecord menu_win_stack[MENU_WIN_DEPTH]`
(`MENU_WIN_DEPTH=9`) plus a bump-allocator pointer `menu_win_ptr` (starting
at `MENU_OVERLAY_BASE = $E300`) implement LIFO window save/restore —
see §6.3.

### 4.3 Directory engine (`src/dir.h`)

```c
struct DirMeta {
    uint16_t next, prev;  // XRAM pointers (linked list; 0 = none)
    uint8_t  type;        // 1=dir, 2=DSK, 3=TAP, 4=ROM, 5=LCE
    uint8_t  select;      // 0=not selected, 1=selected
    uint8_t  length;       // length of name
};

struct DirElement {
    char           name[64];
    struct DirMeta meta;
};
extern struct DirElement presentdirelement;   // currently-active element (loaded from XRAM)

struct Directory {
    uint16_t firstelement; // XRAM ptr to first element of this pane's list
    uint16_t firstprint;   // XRAM ptr to first element shown on screen
    uint16_t lastprint;    // XRAM ptr to last element shown on screen
    uint16_t present;      // XRAM ptr to the currently-active element
    uint8_t  drive;        // drive number (0=A...)
    uint8_t  position;     // cursor row within the visible page (0..PANE_HEIGHT-1)
    char     path[256];    // current path, e.g. "0:/SUBDIR/"
    uint16_t address;      // next free XRAM write address for this pane's list
};
extern struct Directory presentdir[2];   // [0]=top pane, [1]=bottom pane
```

User-configurable settings are grouped in one struct, `settings`:

```c
struct AppSettings
{
    uint8_t confirm;     // 0=confirm once, 1=confirm all (skip subsequent prompts)
    uint8_t filter;      // Type filter: 0=None, 1=DSK, 2=TAP, 3=ROM, 4=LCE
    uint8_t enterchoice; // ENTER action: 0=Select, 1=Mount, 2=Mount+Launch
    uint8_t sort;        // 0=unsorted, 1=sorted directory listing
};
extern struct AppSettings settings;
```

`settings`, the favourites table and each pane's last path/drive/active-pane
are persisted to `0:/idi8b/locifm/locifm.cfg` (`FMCONFIG_PATH`) via
`config_save()`/`config_load()` (`src/dir.c`):

```c
#define FMCONFIG_FAV_SLOTS   8
#define FMCONFIG_FAV_PATHLEN 48
#define FMCONFIG_MAGIC       0xA5

struct FmConfig
{
    uint8_t magic;
    uint8_t confirm;
    uint8_t filter;
    uint8_t enterchoice;
    uint8_t sort;
    char    favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN];
    char    lastpath[2][FMCONFIG_FAV_PATHLEN];
    uint8_t lastdrive[2];
    uint8_t lastactivepane;
};
```

- `config_save()` writes the live `settings`, `favourites[]` and each pane's
  `presentdir[i].path`/`.drive` plus `activepane` into a `struct FmConfig`
  (`magic = FMCONFIG_MAGIC`) and saves it with `file_save()`, creating
  `FMCONFIG_DIR1`/`FMCONFIG_DIR2` first if needed.
- `config_load()` reads the file back with `file_load()`. If the read is
  short, or `cfg.magic != FMCONFIG_MAGIC` (missing/foreign/corrupt file), it
  falls back to writing out the compiled-in defaults via `config_save()`
  instead of loading.
- `favourites_add()`/`favourites_delete()`/`favourites_goto()`/
  `favourites_show()` manage the 8 bookmark slots (`favourites[i][0] ==
  '\0'` marks an unused slot) and call `config_save()` on any change.

Other global application state in `dir.h`/`dir.c`:

| Variable | Meaning |
|---|---|
| `activepane` | 0 = top pane active, 1 = bottom pane active; persisted as `lastactivepane` |
| `targetdrive` | Mount target: 0=A,1=B,2=C,3=D |
| `selection[2]` | Count of selected entries per pane |
| `insidetape[2]` | Non-zero if that pane is browsing inside a `.TAP` container |
| `namefilter[32]` | Wildcard (`*`/`?`) name filter, case-insensitive -- session-only, not part of `settings` |
| `favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN]` | Bookmarked directory paths, persisted via `config_save()`/`config_load()` |
| `pathbuffer[256]`, `pathbuffer2[256]`, `pathbuffer3[256]` | Scratch full-path buffers shared across modules |

### 4.4 LOCI API structures (`include/loci.h`)

```c
typedef struct {            // Oric tape file header (25 bytes)
    uint8_t flag_int, flag_str, type, autorun;
    uint8_t end_addr_hi, end_addr_lo, start_addr_hi, start_addr_lo;
    uint8_t reserved;
    uint8_t filename[16];
} LociTapHdr;

typedef struct {             // directory stream handle
    int16_t  fd;
    uint16_t off;
    char     name[64];
} LociDir;

#define LOCI_DIRENT_SIZE 72
typedef struct {             // 72 bytes -- mirrors CC65 struct dirent layout
    int16_t  d_fd;
    char     d_name[64];
    uint8_t  d_attrib;
    uint8_t  reserved;
    uint32_t d_size;
} LociDirent;

typedef struct { uint8_t major, minor, patch; } LociVersion;

typedef struct {              // 69 bytes -- mirrors CC65 struct utsname
    char sysname[17], nodename[9], release[9], version[9], machine[25];
} LociUname;                  // release = firmware version, e.g. "1.2.3"

#define MAXDEV 9
typedef struct {
    uint8_t     devnr;
    uint8_t     validdev[MAXDEV + 1];
    LociVersion version;
    LociUname   uname;
} LociCfg;
extern LociCfg locicfg;        // populated by get_locicfg()
```

`LociDirent`/`LociUname` sizes are exact-byte-count contracts: `MIA_OP_READDIR`
and `MIA_OP_UNAME` pop exactly `sizeof()` bytes from `XSTACK`, so these
structs must not be repacked or padded differently than the CC65 v1
reference layout.

Directory entry attribute bits: `DIR_ATTR_RDO 0x01`, `DIR_ATTR_SYS 0x04`,
`DIR_ATTR_DIR 0x10`. Recognised extensions: `.DSK`, `.TAP`, `.ROM`, `.LCE`.

### 4.5 Recursive walker (`src/recurse.h`)

```c
typedef enum { RECURSE_ENTER_DIR, RECURSE_FILE, RECURSE_LEAVE_DIR } RecurseEvent;

#define RECURSE_CONTINUE 0
#define RECURSE_ABORT    1

typedef int8_t (*RecurseCallback)(RecurseEvent ev, const LociDirent *entry,
                                   const char *fullpath, void *userdata);

extern uint8_t recurse_truncated;  // set if RECURSE_MAX_DEPTH (8) was hit

int8_t recurse_walk(const char *rootpath, RecurseCallback cb, void *userdata);
```

A single iterative depth-first walker (explicit frame-stack array, no
C-level recursion — required by the 512-byte software stack, §2.1) shared by
recursive copy/move (`file.c`), recursive delete (`dir.c`), and the
recursive directory-size calculation in the properties popup (`dir.c`).

---

## 5. Module Dependency Overview

```
main.c
 ├─ menu.h    (menu bar, pulldowns, popups, window stack)
 ├─ dir.h     (two-pane browser, settings, selection state)
 ├─ file.h    (copy/move/delete/rename/tape-browse)
 │   └─ recurse.h (shared iterative directory walk)
 ├─ drive.h   (mount/unmount/boot-mode flags)
 ├─ viewer.h  (full-screen text viewer)
 │   └─ charwin.h (OricViewport)
 ├─ input.h   (fm_getkey: keyboard + IJK)
 ├─ strings.h (-> strings_en.h / strings_fr.h)
 └─ splash_data.h (logo / QR bitmap)

menu.c, dir.c, file.c, drive.c, viewer.c, input.c, recurse.c
 └─ charwin.h  (windows, cursor, text input)
      └─ keyboard.h (raw key scan/decode)
 └─ loci.h     (MIA/TAP/XRAM, file & dir ops, mount, boot)
      └─ oric.h (hardware register layout, screen/attr constants)

input.c, ijk.c
 └─ ijk.h (VIA Port A joystick; independent of LOCI)
```

`charwin.h`, `loci.h`, `ijk.h`, `menu.h`, `dir.h`, `file.h`, `drive.h`,
`recurse.h`, `viewer.h`, `input.h` each carry a trailing
`#pragma compile("xxx.c")`, so including the header is sufficient to pull the
implementation into the build (Oscar64 whole-program compilation).

---

## 6. Application Flow & Logic

### 6.1 Startup sequence

1. Tape loader jumps to the **startup region** (`$0500`), which contains
   `oric_startup` (`include/oric_crt.c`).
2. `oric_startup`: `SEI` → clear BSS (via `__ip` as a 16-bit pointer) →
   clear the Oscar64 zero-page register file → set the Oscar64 software
   stack pointer (`sp = StackEnd-2`, i.e. top of `$B200–$B3FF`) → set the
   6502 hardware stack pointer to `$FF` → `JSR main` → on return, spin
   forever (`jmp spexit`; bare metal, no OS to return to).
3. `main()` (`src/main.c`):
   - `charwin_init()` — build the row-address lookup table.
   - `loci_present()` — if LOCI is absent (e.g. running under Oricutron),
     show `MSG_LOCI_NOT_FOUND` and exit; overlay-RAM save/restore (menus,
     `cwin_push`) requires LOCI.
   - `menu_init()` — reset the window-save stack, set the overlay pointer,
     call `ijk_detect()`.
   - Reset application state to defaults (`activepane=0`, `settings`
     zeroed (`confirm`/`filter`/`enterchoice`/`sort`), `targetdrive=0`,
     selections and `insidetape` cleared, all boot-mode flags cleared).
   - Populate the dynamic **App** pulldown labels (confirm/return-action/
     filter/sort) and the **Mounts → target drive** label from these
     defaults.
   - `get_locicfg()` — query device count, valid devices, firmware version,
     `uname`.
   - `drive_unmount_all()` — force a known unmounted state.
   - `menu_placetop(MSG_MENU_HEADER)` — clear screen, draw header (row 0,
     green) + menu bar (row 1).
   - Print the LOCI firmware version top-right of the header row.
   - Set pane 0 to `"0:/"` and draw it (`dir_draw(0,1)`); set pane 1 to the
     next available drive after 0 (`dir_get_next_drive(1)`).

### 6.2 Main loop

```c
for (;;) {
    dir_refresh_present();
    switch (fm_getkey()) {
        case KEY_RIGHT: mainmenuloop(); break;   // open menu bar
        case KEY_ENTER: /* enter dir, or act per settings.enterchoice */ break;
        case KEY_ESC:   /* "are you sure?" -> boot() */ break;
        case '.'/',':   dir_get_next_drive / dir_get_prev_drive
        case '/':       dir_switch_pane
        case '\\':      dir_gotoroot
        case KEY_LEFT:  dir_parentdir
        case KEY_UP/DOWN: dir_go_up / dir_go_down
        case 'd'/'p'/'t'/'b': page down/up, top, bottom
        case 's'/'a'/'n'/'i': select toggle/all/none/inverse
        case 'o': dir_togglesort
        case 'c'/'v': file_copy_move_selected(0|1)   // copy / move
        case 'f': select_filter
        case 'g': drive_targetdrive
        case 'r': file_rename
        case 'm'/'u': drive_mount / drive_unmount
        case 'w': file_browse_tape
        case 'e': dir_newdir
        case 'h': help
        case 'k': dir_show_properties
        case 'l': select_namefilter
        case 'j': viewer_show_text (if cursor is on a regular file)
        case KEY_DEL: dir_deletedir (empty selection, on a dir) or file_delete
        default: ignored
    }
}
```

`fm_getkey()` (`src/input.c`) blocks for the next key, decoding IJK joystick
directions to `KEY_UP/DOWN/LEFT/RIGHT` and fire to `KEY_ENTER` when no
keyboard key is pressed, with debounce back to neutral.

`dir_refresh_present()` reloads `presentdirelement` from XRAM for the active
pane's cursor position before every dispatch, so handlers always see
up-to-date metadata (type, selection, name).

### 6.3 Menu system (three-layer + LIFO window stack)

1. **Menu bar** (row 1): `menu_main()` runs the bar navigation loop —
   left/right move between the 6 bar items (App/File/Dir/Mounts/Tools/Info),
   ENTER/down opens that item's **pulldown**. Returns
   `menubarchoice * 10 + menuoptionchoice`, or `99` for ESC/STOP.
2. **Pulldowns**: `menu_pulldown(xpos, ypos, menunumber, escapable)` draws
   `pulldown_titles[menunumber]` (`pulldown_options[menunumber]` entries) and
   handles up/down/ENTER/ESC and left/right (to switch to the adjacent bar
   item, returned as `MENU_LEFT_ARROW`/`MENU_RIGHT_ARROW` = 18/19). Returns
   `1..N`, `MENU_CANCEL` (0), or 18/19.
3. **Popups**: `menu_areyousure()`, `menu_messagepopup()`,
   `menu_fileerrormessage()`, `menu_option_select()`, `menu_confirm_file()`
   — each calls `menu_popup_open(xpos, ypos, height)` /
   `menu_popup_close()` to save/restore the affected rows.

**Window stack** (`menu_winsave`/`menu_winrestore`, §4.2): each
`menu_popup_open` records `{overlay_addr, ypos, height}` on
`menu_win_stack[]`, bumps `menu_win_ptr` by `height*40`, then — if
`loci_present()` — wraps the row copy in `PHP`/`SEI`/`enable_overlay_ram()`
.../`disable_overlay_ram()`/`PLP` and copies `height*40` bytes from screen
RAM (`MENU_ROW(ypos)`) to overlay RAM at `overlay_addr`. `menu_popup_close`
reverses this. Under Oricutron (`!loci_present()`), the stack bookkeeping
still happens but the actual copy is skipped, so popups leave screen
residue — expected and harmless in the emulator.

Return-code convention from `mainmenuloop()`'s `switch`:
`menubarchoice*10 + menuoptionchoice` — e.g. `27` = File pulldown (bar item
2), option 7 (Copy); `61` = Tools pulldown (bar item 6), option 1
(Properties). See `pulldown_titles[]` (§4.2) for the live mapping.

### 6.4 Two-pane directory browser (`dir.c`)

Each pane (`presentdir[0]`/`presentdir[1]`) maintains an independent
**doubly-linked list of `struct DirElement`** stored in **XRAM**
(`DIR1BASE`/`DIR2BASE`, §2.5) — built by `dir_read()` from
`loci_opendir`/`loci_readdir`/`loci_closedir`, applying the active
`settings.filter` and `namefilter` (via `dir_wildcard_match`), and optionally
sorted (`dir_togglesort`/`settings.sort`).

- `dir_get_element(addr)` / `dir_save_element(addr)` — read/write a
  `struct DirElement` to/from an XRAM address via `xram_memcpy_from`/`_to`.
- `presentdirelement` is the cached copy of the element under the cursor;
  `dir_refresh_present()` reloads it from `presentdir[pane].present`.
- **Screen layout per pane** (`OricCharWin pane[2]`, `sx=2, wx=38, wy=12`,
  top-left at `PANE1_YPOS=3` / `PANE2_YPOS=15`):
  - row 0: device/drive name (or `"TAPE: <name>"` if `insidetape`)
  - row 1: drive id + path
  - rows 2–11 (`PANE_HEIGHT=10`): up to 10 visible entries, each
    `dir_print_entry()` — `-` marker if selected, highlighted (cyan/black)
    if it's the cursor row of the active pane, dimmed (white/black) if the
    pane is inactive.
- Navigation: `dir_go_up/down`, `dir_pagedown/pageup`, `dir_top/bottom`,
  `dir_parentdir` (`..`), `dir_gotoroot` (`\`), `dir_switch_pane` (`/`),
  `dir_get_next_drive`/`dir_get_prev_drive` (`.`/`,`).
- Selection: `dir_select_toggle`, `dir_select_all(0|1)`,
  `dir_select_inverse` — update `meta.select` in XRAM and `selection[pane]`.
- Tape browsing: `dir_tape_parse()` builds the in-pane listing from a
  mounted `.TAP` image's header records (`insidetape[pane]=1`); the first 4
  bytes of each `name` hold the tape position counter (skipped via `offs=4`
  in `dir_print_entry`).
- `dir_newdir()` / `dir_deletedir()` — create / recursively delete a
  directory (the latter via `recurse_walk`).
- `dir_show_properties()` — popup with type/attributes/size; for
  directories, size is computed recursively via `recurse_walk`
  (`RECURSE_FILE` events accumulate `d_size` into a `uint32_t`, formatted by
  `dir_dec10()`).

### 6.5 File operations (`file.c`)

- `file_copy_move_selected(move)` — for each selected entry (or the
  cursor entry if nothing is selected) in the active pane, copies or moves
  it to the path of the *other* pane:
  - **Files:** `file_copy_progress()` (loci.c) streams through the 2 KB
    XRAM copy buffer (`COPYBUF_XRAM_ADDR`), drawing a progress bar
    (`dirProgressBar[4]` glyphs) in the destination pane row; ESC during the
    transfer cancels — the partially-written destination file is removed
    (`file_copy_move_cb`/`file_copy_move_dir` handle cleanup).
  - **Directories:** `file_copy_move_dir()` uses `recurse_walk()` —
    `RECURSE_ENTER_DIR` creates the matching destination directory,
    `RECURSE_FILE` copies each file, `RECURSE_LEAVE_DIR` (on `move`) removes
    the now-empty source directory.
  - Overwrite and per-item confirmation use `menu_confirm_file()`, gated by
    `settings.confirm` (once vs. all).
- `file_delete()` — deletes selected file(s); for a non-empty directory with
  no selection, `main.c` instead calls `dir_deletedir()` (recursive).
- `file_rename()` — text-input popup + `loci_rename()`.
- `file_browse_tape()` — toggles `insidetape[pane]` and re-renders the pane
  from the mounted tape's header list (`dir_tape_parse`).

### 6.6 Drives, mounting and boot (`drive.c`)

- `mount_filename[6][21]` — filenames currently mounted on drives A–D, Tape,
  ROM (slots 0–5).
- Boot-mode flags `fdc_on`, `tap_on`, `b11_on`, `bit_on`, `ald_on` track
  which subsystems should be active when the machine reboots.
- `drive_mount()`/`drive_unmount()`/`drive_unmount_all()` wrap
  `loci_mount()`/`loci_umount()`; `drive_targetdrive()` cycles
  `targetdrive` (A–D) for subsequent mounts; `drive_showmounts()` is an info
  popup listing all six slots.
- **`boot()`** (`main.c`): if drive A has nothing mounted but a tape is
  mounted, sets `ald_on` (tape autoload). Calls
  `mia_call_boot(0x80 | ald_on<<4 | bit_on<<3 | b11_on<<2 | tap_on<<1 | fdc_on)`.
  On success this reboots the Oric directly into the mounted media in
  **disk > tape > ROM** preference order and never returns. On failure,
  shows `MSG_MAIN_BOOT_FAILED`. This is triggered by ESC→Yes, the App menu's
  Exit option, or (if `settings.enterchoice==2`, "Launch") by pressing ENTER
  on a non-directory entry.

### 6.7 IJK joystick (`ijk.c`, independent of LOCI)

`ijk_detect()` probes for the Raxiss IJK interface on VIA Port A (`$030F`)
+ Port B bit 4 at startup, setting `ijk_present`. `ijk_read()` (called from
`fm_getkey()`) reads `ijk_ljoy`/`ijk_rjoy` (bitmasks `IJK_JOY_RIGHT/LEFT/
FIRE/DOWN/UP`), decoded to cursor keys + ENTER. All VIA Port A access is
bracketed with `SEI`/`CLI` (shared with the keyboard scanner — see §2.6 for
why menu/charwin code uses `PHP`/`PLP` instead, since IJK's access window is
short and known-safe by construction).

### 6.8 Text viewer (`viewer.c`)

`viewer_show_text(path)` opens the file read-only via `loci_open`;
`menu_fileerrormessage()` is shown and the function returns if it can't be
opened. Otherwise it opens a full-screen popup with two `OricCharWin`s — a
38x27 content window and a 38x1 footer — and reads the file in
`VIEWER_CHUNK_SIZE` chunks, in one of two modes:

- **Text mode** (`viewer_run_text`): splits on `'\n'` (ignoring `'\r'`) and
  word-wraps each line into the content window via `cwin_printwrap()`.
- **Hex mode** (`viewer_run_hex`): renders `VIEWER_HEX_PER_LINE` (8) bytes per
  row as `"OFFS: b0 b1 .. b7"` plus an ASCII column.

In both modes, any byte outside printable ASCII (0x20-0x7E) is shown as a
`.` placeholder (`VIEWER_PLACEHOLDER_CHAR`) — raw control bytes are charwin
screen attributes, not characters, and an embedded NUL would truncate
`cwin_printwrap()`'s string early. Pagination is forward-only: once the
content window's last row is used, the footer shows a prompt and pauses for
a keypress before clearing for the next page. At any pause point, ESC exits
back to the main interface, and `X` toggles between text and hex mode,
seeking the file back to offset 0 and restarting in the new mode. Invoked
from the main loop (`j`) or menu (`Tools → View text file`, case 63) when the
cursor is on a non-directory entry outside a tape browse.

### 6.9 `libdemo.c`

A standalone `.tap` build (`make libdemo`) that exercises every
charwin/keyboard/LOCI/IJK library function in lettered sections (A–Q),
ending with a fixed "LIBRARY TEST COMPLETE. HALTED." screen. Used both for
manual library verification and as the target of `make test-libdemo`
(§7). It is independent of `main.c`'s application logic — a pure library
smoke test.

---

## 7. Testing Infrastructure

Automated headless testing runs the built `.tap` images under **Phosphoric**
(`oric1-emu`), which emulates a LOCI device backed by a host-filesystem
sandbox (`--loci-flash tests/sandbox`) and supports `--type-keys` (auto-typer)
+ `--dump-ram-at` (RAM dump for screen-content assertions).

```
make test            # full suite (all 11 targets below)
make test-quick      # boot smoke test: LOCI detection + main interface
make test-menus      # pulldown menu regression (5 menus, open + close)
make test-fileops    # mkdir/rename/delete/copy/move, via tests/sandbox state
make test-libdemo    # full libdemo walkthrough (sections A-Q + completion screen)
make test-recurse    # Tools pulldown + recursive copy/move/delete
make test-namefilter # wildcard name filter set/clear
make test-copycancel # ESC mid-copy cancellation + partial-file cleanup
make test-viewer     # text file viewer
make test-config     # persistent settings save/load (config_save/config_load)
make test-favourites # favourite directories (add/goto/delete, Tools popup)
make test-laststate  # remembered pane path/drive/active-pane across restart
make test-capture CYCLES=N TYPEKEYS='...'   # calibration helper for new scripts
```

- Each `make test-*` target runs `sandbox-reset` first: wipes
  `tests/sandbox/`, copies `tests/fixtures/` into it, and copies the
  freshly-built `.tap` images in.
- `tests/scripts/oric_screen.py` decodes the 40×28 `$BB80` text screen from a
  `--dump-ram-at` dump, providing `--find`/`--row` assertions used by the
  shell scripts in `tests/scripts/test_*.sh`.
- `test-fileops` (and other state-mutating tests) run via separate `$(MAKE)`
  invocations from `test:` so each gets its own `sandbox-reset`.
- LOCI overlay-RAM features (`cwin_push`/`cwin_pop`, menu window-save) are
  **not exercised** by Phosphoric/Oricutron — verified on real hardware only.

---

## 8. Key Constraints (carry into any new feature work)

- **512-byte software stack** (`$B200–$B3FF`) shared by the whole call
  chain → no C-level recursion; use `recurse_walk()`'s explicit frame stack
  (max depth `RECURSE_MAX_DEPTH=8`) for any tree-walking logic.
- **No working heap** — `crt_malloc` returns `NULL` in this runtime
  configuration (no `heap` region/`#pragma heapsize()`). All buffers are
  static/global, fixed-size.
- **`$B400` ceiling** — code/data/BSS/stack must not extend into charset RAM
  (`$B400+`).
- **No floats** (`-dNOFLOAT`) — `cwin_printf`/`_cwin_vformat` support only
  `%d/%u/%x/%s/%c/%%`; `dir_dec6()`/`dir_dec10()` are manual decimal
  formatters for `uint16_t`/`uint32_t`.
- **All user-visible strings are `MSG_*` macros** in `strings_en.h`/
  `strings_fr.h` — retroactively extracting raw literals is painful; new
  strings must be added to both files.
- **IRQs stay disabled for the whole program** — never use unconditional
  `SEI`/`CLI`; use `PHP`/`SEI`/`PLP` for any code that touches overlay RAM or
  VIA Port A.
- **Overlay-RAM/XRAM/MIA features require real LOCI hardware** — Oricutron
  cannot exercise them; rely on Phosphoric (host-fs-backed) for automated
  coverage and real hardware for overlay-RAM-specific checks.
