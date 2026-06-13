# locifilemanager-v2 — Library Reference Manual

Oscar64 bare-metal libraries for the Oric Atmos.  
Target: 6502A, 1 MHz, no ROM calls.

---

## Contents

1. [Hardware overview (oric.h)](#1-hardware-overview-orich)
2. [Keyboard scanner (keyboard.h)](#2-keyboard-scanner-keyboardh)
3. [Character window library (charwin.h)](#3-character-window-library-charwinh)
4. [IJK joystick (ijk.h)](#4-ijk-joystick-ijkh)
5. [LOCI mass-storage API (loci.h)](#5-loci-mass-storage-api-locih)
6. [Menu system (menu.h)](#6-menu-system-menuh)
7. [Build notes](#7-build-notes)

---

## 1. Hardware overview (oric.h)

Include `oric.h` in any file that references hardware registers, attribute
constants, or screen layout macros.

### Screen

| Symbol | Value | Meaning |
|---|---|---|
| `TEXTVRAM` | `0xBB80` | Base address of text screen RAM |
| `SCREEN_COLS` | `40` | Columns per row |
| `SCREEN_ROWS` | `28` | Rows |
| `SCREEN_SIZE` | `1120` | Total bytes (40 × 28) |

The Oric ULA processes screen RAM left-to-right on every raster line. A byte
with `(byte & 0x60) == 0` (i.e. values `0x00–0x1F`) is a **serial attribute**
that changes ink colour, paper colour, or character set for the rest of that
row.  All other byte values are character codes.  The ULA resets ink to white
and paper to black at the start of each raster line.

**Consequence:** attributes occupy one screen column each and shift all
subsequent characters one position to the right.

### Ink (foreground) colour constants

`A_FWBLACK` (0) · `A_FWRED` (1) · `A_FWGREEN` (2) · `A_FWYELLOW` (3)  
`A_FWBLUE` (4) · `A_FWMAGENTA` (5) · `A_FWCYAN` (6) · `A_FWWHITE` (7)

**Warning:** `A_FWBLACK = 0x00` is the C NUL terminator.  It cannot be
embedded in a string literal.  Write it with `cwin_put_attr(&w, A_FWBLACK)`.

### Paper (background) colour constants

`A_BGBLACK` (16) · `A_BGRED` (17) · `A_BGGREEN` (18) · `A_BGYELLOW` (19)  
`A_BGBLUE` (20) · `A_BGMAGENTA` (21) · `A_BGCYAN` (22) · `A_BGWHITE` (23)

### Character-set mode constants

| Constant | Value | Effect |
|---|---|---|
| `A_STD` | 8 | Standard character set |
| `A_ALT` | 9 | Alternate (mosaic/graphics) set |
| `A_STD2H` | 10 | Double-height standard |
| `A_ALT2H` | 11 | Double-height alternate |
| `A_BLINKSTD` | 12 | Blinking standard |
| `A_BLINKALT` | 13 | Blinking alternate |
| `A_BLINK2H` | 14 | Double-height blinking |
| `A_BLINK2HALT` | 15 | Double-height blinking alternate |

### Special character codes

| Constant | Value | Character |
|---|---|---|
| `CH_SPACE` | `0x20` | Space — safe for clearing |
| `CH_INVSPACE` | `0xA0` | Solid ink-colour block (cursor, animation) |
| `CH_POUND` | `0x5F` | £ sign (ROM maps ASCII `_`) |
| `CH_COPYRIGHT` | `0x60` | © sign (ROM maps ASCII `` ` ``) |

Avoid `_` and `` ` `` in display strings; they render as £ and ©.

### Inline string attribute macros (ASTR_*)

Embed colour or charset changes directly in string literals:

```c
cwin_putat_string(&w, 2, 5,
    ASTR_INK_RED "alert" ASTR_INK_WHITE " normal");
```

Each `ASTR_*` expands to a one-byte escape sequence.  Available macros:

- Ink: `ASTR_INK_RED` · `_GREEN` · `_YELLOW` · `_BLUE` · `_MAGENTA` · `_CYAN` · `_WHITE`
- Paper: `ASTR_PAPER_BLACK` · `_RED` · `_GREEN` · `_YELLOW` · `_BLUE` · `_MAGENTA` · `_CYAN` · `_WHITE`
- Charset: `ASTR_CHARSET_STD` · `_ALT` · `_STD2H` · `_ALT2H` · `_BLKSTD` · `_BLKALT` · `_BLK2H` · `_BLK2HA`

**Constraints:**
- `ASTR_INK_BLACK` (`\x00`) = NUL — cannot appear in a C string literal.
  Use `cwin_put_attr(&w, A_FWBLACK)`.
- `ASTR_CHARSET_STD2H` (`\x0A`) = `\n` — triggers scroll in
  `cwin_console_put_string`.  Use `cwin_put_attr` in console context.

### Overlay RAM

Overlay RAM (`0xC000–0xFFFF`) is normally the Atmos ROM.  When LOCI is
connected, writing to the Microdisc-compatible register at `0x0314` enables
RAM underneath the ROM.

```c
#define MICRODISCCFG    (*((volatile uint8_t *)0x0314))
#define OVERLAY_ON      0xFD   // enable overlay RAM
#define OVERLAY_OFF     0xFF   // restore ROM
#define OVERLAY_BASE    0xC000U
#define OVERLAY_SIZE    0x4000U
```

**Requires LOCI.** Not testable in Oricutron.  The high-level helpers
`enable_overlay_ram()` and `disable_overlay_ram()` in `loci.h` are the
preferred interface.

---

## 2. Keyboard scanner (keyboard.h)

Direct VIA/AY hardware scan — no ROM calls.  Include `keyboard.h`.

### Key codes

| Constant | Value | Key |
|---|---|---|
| `KEY_NONE` | `0x00` | No key |
| `KEY_ENTER` | `0x0D` | Return |
| `KEY_ESC` | `0x1B` | Escape |
| `KEY_DEL` | `0x7F` | Delete / Backspace |
| `KEY_UP` | `0x0B` | Cursor up |
| `KEY_DOWN` | `0x0A` | Cursor down |
| `KEY_LEFT` | `0x08` | Cursor left |
| `KEY_RIGHT` | `0x09` | Cursor right |
| `KEY_SPACE` | `0x20` | Space |
| `KEY_F1`–`KEY_F9` | `0xB1`–`0xB9` | FUNCT + 1–9 |
| `KEY_F10` | `0xB0` | FUNCT + 0 |

CTRL + letter produces `letter & 0x1F`.  Constants `KEY_CTRL_A` (1),
`KEY_CTRL_C` (3), `KEY_CTRL_X` (24), `KEY_CTRL_Z` (26) are provided.

`KEY_TAB` (`0x09`) is the same value as `KEY_RIGHT` — context-dependent.

### Modifier flags (keyb_modifiers)

| Constant | Bit | Meaning |
|---|---|---|
| `MOD_SHIFT` | `0x01` | Shift held |
| `MOD_CTRL` | `0x02` | Ctrl held |
| `MOD_FUNCT` | `0x04` | FUNCT held (Atmos) |
| `MOD_CAPSLOCK` | `0x08` | Caps Lock active |

`keyb_modifiers` is written by `keyb_poll()`.

### Global state

```c
extern uint8_t  keyb_matrix[8];   // raw 8×8 matrix bits
extern volatile uint8_t keyb_char;    // last decoded key
extern uint8_t  keyb_modifiers;   // current modifier flags
```

### Functions

```c
void    keyb_scan(void);
```
Scan the full 8×8 matrix into `keyb_matrix[]`.  Takes ~1–2 ms at 1 MHz.
Call this from a polling loop or timer IRQ.

```c
uint8_t keyb_decode(void);
```
Decode `keyb_matrix[]` into `keyb_char` and `keyb_modifiers`.  Returns the
decoded ASCII/key code (0 = no key).  Handles Shift, Ctrl, FUNCT, Caps Lock.

```c
uint8_t keyb_poll(void);
```
Scan + decode + handle key repeat.  Updates `keyb_char`.  Returns the key
code (0 = no key).  Repeat fires after ~400 ms, then every ~100 ms.
**Use this as the main polling function.**

```c
uint8_t keyb_getch(void);
```
Blocking: loops `keyb_poll()` until a key is pressed.  Returns the key code.

```c
uint8_t keyb_check(void);
```
Non-blocking check.  Returns non-zero if any key is currently held.

### Animation delay without sleep

`keyb_scan()` takes ~370 cycles at 1 MHz.  Use it as a delay unit:

```c
for (uint8_t d = 0; d < 80; d++) keyb_scan();   // ≈ 30 ms
```

Hardware register accesses in `keyb_scan` prevent the Oscar64 optimiser from
collapsing the loop.

---

## 3. Character window library (charwin.h)

A character-based windowed display library for the Oric 40×28 text screen.
All functions use direct screen RAM writes — no ROM calls.

Include `charwin.h`.  The library auto-compiles `charwin.c` via
`#pragma compile`.

### Window model

Each window is described by an `OricCharWin` struct:

```c
typedef struct {
    uint8_t sx, sy;   // top-left corner (sx >= 2)
    uint8_t wx, wy;   // width and height in characters
    uint8_t cx, cy;   // cursor position within the window (0-based)
    uint8_t ink;      // ink colour (A_FW* constant)
    uint8_t paper;    // paper colour (A_BG* constant)
} OricCharWin;
```

**Column layout:** `cwin_clear` writes the window's ink attribute at screen
column 0 and paper attribute at screen column 1 for every managed row.
Content occupies columns `sx` through `sx + wx - 1`.  The minimum `sx` is 2
to leave room for the two attribute columns.

### Initialisation

```c
void charwin_init(void);
```
Build the row address lookup table.  **Call once before any `cwin_*` function.**

```c
void cwin_init(OricCharWin *w,
               uint8_t sx, uint8_t sy,
               uint8_t wx, uint8_t wy,
               uint8_t ink, uint8_t paper);
```
Populate a window struct.  Enforces `sx >= 2`.  Does not draw anything.

### Clear and fill

```c
void cwin_clear(OricCharWin *w);
```
Fill content with spaces (`0x20`).  Write ink attribute at column 0 and paper
attribute at column 1 for every row in the window.  Reset cursor to (0, 0).

```c
void cwin_fill_rect(OricCharWin *w,
                    uint8_t x, uint8_t y,
                    uint8_t bw, uint8_t bh,
                    uint8_t ch);
```
Fill a `bw × bh` rectangle at window-relative `(x, y)` with character `ch`.
Clips to window bounds.

### Positional writes (no cursor update)

```c
void cwin_putat_char(OricCharWin *w, uint8_t x, uint8_t y, uint8_t ch);
```
Write one character at window-relative `(x, y)`.

```c
uint8_t cwin_getat_char(OricCharWin *w, uint8_t x, uint8_t y);
```
Read the character at window-relative `(x, y)` from screen RAM.

```c
void cwin_putat_string(OricCharWin *w, uint8_t x, uint8_t y, const char *s);
```
Write a null-terminated string starting at `(x, y)`.  Clips at the right
edge.  Attribute bytes embedded with `ASTR_*` macros are written as-is
(each occupies one column and shifts subsequent text right).

```c
void cwin_putat_dblhi_string(OricCharWin *w, uint8_t x, uint8_t y, const char *s);
```
Write `s` in double-height at `(x, y)`.  Outputs `A_STD2H`, the string, then
`A_STD` on **both** row `y` and row `y+1` in a single call (the hardware
produces top-half on `y`, bottom-half on `y+1`).  Requires `y + 1 < w->wy`.

```c
void cwin_putat_printf(OricCharWin *w, uint8_t x, uint8_t y,
                       const char *fmt, ...);
```
Printf-style write at `(x, y)`.  Clips at right edge.  Supported specifiers:
`%d` (int16), `%u` (uint16), `%x` (uint16 hex uppercase), `%s`, `%c`, `%%`,
with width and zero-fill (e.g. `%04u`).  No float support (`-dNOFLOAT`).

### Cursor-advancing writes

```c
void cwin_put_char(OricCharWin *w, uint8_t ch);
void cwin_put_string(OricCharWin *w, const char *s);
void cwin_put_attr(OricCharWin *w, uint8_t attr);
void cwin_printf(OricCharWin *w, const char *fmt, ...);
```
Write at the current cursor position and advance the cursor.  No newline
or scroll handling.  `cwin_put_attr` is the correct way to write `A_FWBLACK`
(0x00) since it cannot appear in a C string literal.

### Console mode (newline + scroll)

```c
void cwin_console_put_char(OricCharWin *w, uint8_t ch);
void cwin_console_put_string(OricCharWin *w, const char *s);
```
Write in console mode: `\n` advances the cursor to the start of the next row,
scrolling the window if the cursor is already on the last row.  Other
characters wrap at the right edge.

**Caution:** `ASTR_CHARSET_STD2H` (`\x0A`) equals `\n` and will trigger a
scroll.  Use `cwin_put_attr` to write it in console mode.

### Scrolling

```c
void cwin_scroll_up(OricCharWin *w);
```
Scroll window content up by one row.  The new bottom row is filled with
spaces and its attributes are refreshed.

```c
void cwin_scroll_down(OricCharWin *w);
```
Scroll window content down by one row.  The new top row is filled with spaces
and its attributes are refreshed.

### Insert and delete

```c
void cwin_insert_char(OricCharWin *w);
```
Insert a space at the cursor position, shifting the rest of the row right.
The character at column `wx - 1` is discarded.  Cursor is not moved.

```c
void cwin_delete_char(OricCharWin *w);
```
Delete the character at the cursor position, shifting the rest of the row
left.  Column `wx - 1` is filled with a space.  Cursor is not moved.

### Print line

```c
void cwin_printline(OricCharWin *w, const char *s);
```
Write `s` at the current cursor position, then advance the cursor to the
start of the next row.  Scrolls if on the last row.  Use with
`cwin_putat_printf` to output numbered lines that auto-scroll:

```c
uint8_t cy = w.cy;   // temp variable — avoids Oscar64 cast-before-member gotcha
cwin_putat_printf(&w, 0, cy, "Line %u", (uint16_t)i);
cwin_printline(&w, "");
```

### Cursor

```c
void cwin_cursor_show(OricCharWin *w, bool on);
```
Show (`on = true`) or hide the cursor at the current position using
inverse-video toggle.  The caller must track the show/hide state to avoid
double-toggling.

```c
bool cwin_cursor_left(OricCharWin *w);
bool cwin_cursor_right(OricCharWin *w);
bool cwin_cursor_up(OricCharWin *w);
bool cwin_cursor_down(OricCharWin *w);
```
Move the cursor one position.  Return `true` if moved, `false` if already at
the window edge.

### Text input widget

```c
signed int cwin_textinput(OricCharWin *w,
                          uint8_t x, uint8_t y,
                          uint8_t vwidth,
                          char *str, uint8_t maxlen,
                          uint8_t validation);
```
Interactive single-line text input at window-relative `(x, y)`.

| Parameter | Meaning |
|---|---|
| `vwidth` | Visible viewport width; `vwidth < maxlen` enables horizontal scrolling |
| `str` | Pre-initialised buffer of at least `maxlen + 1` bytes |
| `maxlen` | Maximum string length (not counting NUL) |
| `validation` | Input filter: `VINPUT_ALL` (0), `VINPUT_NUMS` (1), `VINPUT_ALPHA` (2), `VINPUT_WILD` (4) |

Returns the string length on ENTER, or `-1` on ESC.

Validation flags:

| Constant | Value | Allowed characters |
|---|---|---|
| `VINPUT_ALL` | `0` | All printable characters |
| `VINPUT_NUMS` | `1` | Digits 0–9 only |
| `VINPUT_ALPHA` | `2` | Letters + digits (adds `VINPUT_NUMS`) |
| `VINPUT_WILD` | `4` | Add `*` and `?` to any combination |

### Viewport — scrollable view into a source map

```c
typedef struct {
    uint8_t     *sourcebase;    // flat source character map
    uint16_t     sourcewidth;   // bytes per row in the map (>= win->wx)
    uint16_t     sourceheight;  // total rows in the map
    uint16_t     viewx;         // horizontal scroll offset
    uint16_t     viewy;         // vertical scroll offset
    OricCharWin *win;           // target display window
} OricViewport;
```

The source map is a flat byte array: `map[row * sourcewidth + col]`.  Only
character bytes are stored — attributes come from the window's `ink` and
`paper` settings.

**Important:** Large source maps must be declared `static`.  A 60 × 20 = 1200-byte
map allocated as a local variable would overflow the 256-byte 6502 stack.

```c
void cwin_viewport_init(OricViewport *vp,
                        uint8_t *sourcebase,
                        uint16_t sourcewidth, uint16_t sourceheight,
                        OricCharWin *win);
```
Initialise the viewport.  Sets `viewx = viewy = 0`.  Does not draw.

```c
void cwin_viewport_blit(OricViewport *vp);
```
Copy the current view region to the display window, including ink/paper
attributes on every row.

```c
void cwin_viewport_scroll(OricViewport *vp, uint8_t dir);
```
Scroll one unit in direction `dir` (`KEY_UP`, `KEY_DOWN`, `KEY_LEFT`,
`KEY_RIGHT`).  Clamps to source bounds, then calls `cwin_viewport_blit`.

Example interactive scroll loop:

```c
static uint8_t map[20 * 60];   // static — 1200 bytes, must not be on the stack
// ... fill map ...
OricCharWin vpwin;
OricViewport vp;
cwin_init(&vpwin, 2, 3, 34, 10, A_FWWHITE, A_BGBLUE);
cwin_viewport_init(&vp, map, 60, 20, &vpwin);
cwin_viewport_blit(&vp);

uint8_t k;
while ((k = keyb_poll()) != KEY_ESC) {
    if (k == KEY_UP || k == KEY_DOWN || k == KEY_LEFT || k == KEY_RIGHT)
        cwin_viewport_scroll(&vp, k);
}
```

### Overlay RAM save/restore

```c
void cwin_push(OricCharWin *w);
void cwin_pop(OricCharWin *w);
```
Save (`push`) and restore (`pop`) the rows covered by window `w` using
overlay RAM.  Up to 8 levels (LIFO stack).  **Requires LOCI device** —
not testable in Oricutron; skip gracefully when LOCI is absent.

### Key read

```c
uint8_t cwin_getch(void);
```
Blocking key read (delegates to `keyb_getch`).

---

## 4. IJK joystick (ijk.h)

Raxiss IJK interface using VIA Port A (`$030F`) and Port B bit 4.
Include `ijk.h` separately from `charwin.h` — it is not a dependency of
the window library.

The IJK interface shares VIA Port A with the keyboard scanner.  `ijk_read`
brackets all Port A accesses with SEI/CLI to prevent conflict.

### Direction bit masks

| Constant | Value | Direction |
|---|---|---|
| `IJK_JOY_RIGHT` | `0x01` | Right |
| `IJK_JOY_LEFT` | `0x02` | Left |
| `IJK_JOY_FIRE` | `0x04` | Fire button |
| `IJK_JOY_DOWN` | `0x08` | Down |
| `IJK_JOY_UP` | `0x10` | Up |

A bit is **1** when the direction is pressed (active-high after inversion).

### Global state

```c
extern uint8_t ijk_present;  // non-zero if IJK detected
extern uint8_t ijk_ljoy;     // left joystick state
extern uint8_t ijk_rjoy;     // right joystick state
```

### Functions

```c
void ijk_detect(void);
```
Probe the hardware for an IJK interface.  Sets `ijk_present` to non-zero if
found.  Call once after startup (or delegate to `menu_init` which calls it).

```c
void ijk_read(void);
```
Read both joysticks into `ijk_ljoy` and `ijk_rjoy`.  No-op if `ijk_present`
is zero.

### Typical use in a menu/input loop

```c
ijk_detect();
// ...
uint8_t k = keyb_poll();
if (ijk_present && k == KEY_NONE) {
    ijk_read();
    if      (ijk_ljoy & IJK_JOY_UP)    k = KEY_UP;
    else if (ijk_ljoy & IJK_JOY_DOWN)  k = KEY_DOWN;
    else if (ijk_ljoy & IJK_JOY_LEFT)  k = KEY_LEFT;
    else if (ijk_ljoy & IJK_JOY_RIGHT) k = KEY_RIGHT;
    else if (ijk_ljoy & IJK_JOY_FIRE)  k = KEY_ENTER;
}
```

---

## 5. LOCI mass-storage API (loci.h)

High-level C API for the LOCI mass-storage device.  Include `loci.h`.

> **All LOCI operations require real LOCI hardware.**  Oricutron does not
> emulate the MIA or TAP registers.  `loci_present()` returns `false` in
> the emulator; all file/directory/mount operations degrade gracefully.

### Hardware registers

#### MIA — Mass Interface Adapter at `$03A0`

```c
#define MIA (*(volatile struct __LOCI_MIA *)0x03A0)
```

Key fields used by high-level functions:

| Field | Address | Purpose |
|---|---|---|
| `MIA.ready` | `$03A0` | RX/TX ready bits (`MIA_READY_TX`, `MIA_READY_RX`) |
| `MIA.tx` | `$03A1` | Transmit byte to firmware |
| `MIA.rx` | `$03A2` | Receive byte from firmware |
| `MIA.xstack` | `$03AC` | XSTACK — parameter/result byte exchange |
| `MIA.errno_lo` | `$03AD` | Error code low byte |
| `MIA.op` | `$03AF` | Write an operation code to invoke it |
| `MIA.busy` | `$03B2` | Bit 7 set while operation is in progress |
| `MIA.areg` | `$03B4` | A register (argument/result low byte) |
| `MIA.xreg` | `$03B6` | X register (argument/result high byte) |

#### TAP — Tape controller at `$0315`

```c
#define TAP (*(volatile struct __LOCI_TAP *)0x0315)
```

| Field | Address | Purpose |
|---|---|---|
| `TAP.cmd` | `$0315` | Command (`TAP_CMD_PLAY/REC/REW/BIT/FFW`) |
| `TAP.status` | `$0316` | Status |
| `TAP.data` | `$0317` | Data |

### Global state

```c
extern uint8_t loci_errno;  // error code from last failed operation
extern LociCfg locicfg;     // device configuration (filled by get_locicfg)

#define LOCI_EACCES 3        // "not empty": loci_errno set by MIA_OP_UNLINK
                             // on a non-empty directory (FatFs FR_DENIED /
                             // POSIX ENOTEMPTY; confirmed against
                             // Phosphoric's loci_fs.c op_unlink, revisit
                             // against real LOCI firmware if it differs)
```

### Data structures

```c
typedef struct { uint8_t major, minor, patch; } LociVersion;
```

```c
typedef struct {
    uint8_t     devnr;
    uint8_t     validdev[10];
    LociVersion version;
    LociUname   uname;
} LociCfg;
```

```c
typedef struct {
    int16_t  fd;
    uint16_t off;
    char     name[64];
} LociDir;   // directory stream handle
```

```c
typedef struct {
    int16_t  d_fd;
    char     d_name[64];
    uint8_t  d_attrib;     // DIR_ATTR_RDO, DIR_ATTR_SYS, DIR_ATTR_DIR
    uint8_t  reserved;
    uint32_t d_size;
} LociDirent;   // 72 bytes
```

Directory attribute flags: `DIR_ATTR_RDO` (0x01 = read-only),
`DIR_ATTR_SYS` (0x04 = system), `DIR_ATTR_DIR` (0x10 = directory).

```c
typedef struct {
    uint8_t flag_int, flag_str, type, autorun;
    uint8_t end_addr_hi, end_addr_lo;
    uint8_t start_addr_hi, start_addr_lo;
    uint8_t reserved;
    uint8_t filename[16];
} LociTapHdr;   // 25-byte Oric tape header
```

File open flags: `O_RDONLY` (1) · `O_WRONLY` (2) · `O_RDWR` (3) ·
`O_CREAT` (0x10) · `O_TRUNC` (0x20) · `O_APPEND` (0x40) · `O_EXCL` (0x80).

Seek whence: `SEEK_CUR` (0) · `SEEK_END` (1) · `SEEK_SET` (2).

### Detection and configuration

```c
bool loci_present(void);
```
Return `true` if a LOCI device is active.  Checks for the `'L'` signature at
`$0319`.  **Call before any LOCI operation.**

```c
void get_locicfg(void);
```
Populate `locicfg`: device count, firmware version, system information via
`MIA_OP_UNAME`.

```c
bool loci_check_fw(uint8_t major, uint8_t minor, uint8_t patch);
```
Return `true` if the firmware version is ≥ `major.minor.patch`.  Suitable
for version-gating features.

```c
const char *get_loci_devname(uint8_t devid, uint8_t maxlength);
```
Return the drive label string for device `devid`.

```c
void loci_uname(LociUname *buf);
```
Populate a `LociUname` struct via XSTACK.

### System utilities

```c
int16_t phi2(void);
```
Return the CPU clock frequency in kHz (typically 1000 for Oric Atmos).

```c
int32_t loci_lrand(void);
```
Return a random 32-bit integer from the LOCI firmware RNG.

### Overlay RAM helpers

```c
void enable_overlay_ram(void);
void disable_overlay_ram(void);
```
Enable/disable the overlay RAM by writing to `MICRODISCCFG` at `$0314`.
Always disable before returning to normal ROM execution.  Used by the menu
system and `cwin_push`/`cwin_pop`.

### XRAM access

XRAM is extended RAM accessible only via MIA DMA channels.  Base address
for the copy buffer: `COPYBUF_XRAM_ADDR` (`0x8000`), size `COPYBUF_XRAM_SIZE`
(`0x0800` bytes).

```c
void    xram_poke(uint16_t addr, uint8_t val);
uint8_t xram_peek(uint16_t addr);
void    xram_memcpy_to(uint16_t dest, const void *src, uint16_t count);
void    xram_memcpy_from(void *dest, uint16_t src, uint16_t count);
```

### File I/O

```c
int16_t loci_open(const char *path, uint16_t flags);
```
Open a file.  Returns a file descriptor (≥ 0) on success, negative on error.
`flags` is a combination of `O_*` constants.

```c
int16_t loci_close(int16_t fd);
int16_t loci_read(int16_t fd, void *buf, uint16_t count);
int16_t loci_write(int16_t fd, const void *buf, uint16_t count);
int32_t loci_lseek(int16_t fd, int32_t offset, uint8_t whence);
int16_t loci_unlink(const char *path);   // delete file
int16_t loci_rename(const char *oldpath, const char *newpath);
```

On error, these functions set `loci_errno` and return a negative value.

### High-level file operations

```c
bool file_exists(const char *path);
```
Return `true` if the file at `path` exists.

```c
int16_t file_load(const char *path, void *dst, uint16_t count);
```
Open, read `count` bytes into `dst`, close.  Returns bytes read or negative.

```c
int16_t file_save(const char *path, const void *src, uint16_t count);
```
Create/overwrite, write `count` bytes from `src`, close.  Returns bytes
written or negative.

```c
int16_t file_copy(const char *dst, const char *src);
```
Copy file `src` to `dst` using the XRAM copy buffer.  Returns bytes copied
or negative on error.

```c
int16_t file_copy_progress(const char *dst, const char *src,
                            uint8_t progx, uint8_t progy, uint8_t progl);
```
Like `file_copy`, but draws a progress bar directly into screen RAM at
column `progx`, row `progy` (`progl` cells wide) while copying via the XRAM
buffer. Polls `keyb_check()` once per chunk; if `KEY_ESC` is pressed,
copying stops immediately, the partially written `dst` file is removed via
`loci_unlink`, and the function returns `-2`. Returns `0` on success, or
another negative `loci_errno`-setting error code on I/O failure.

### Directory operations

```c
LociDir    *loci_opendir(const char *path);
void        loci_closedir(LociDir *dir);
LociDirent *loci_readdir(LociDir *dir);
int16_t     loci_mkdir(const char *path);
void        loci_getcwd(char *buf, uint8_t len);
```

`loci_readdir` returns a pointer to a static `LociDirent` buffer, or `NULL`
at end-of-directory.  The buffer is overwritten on each call.

`loci_getcwd` fills `buf` (size `len`) with the current working directory
path.

### Mount operations

```c
int16_t loci_mount(int16_t drive, const char *path, const char *filename);
int16_t loci_umount(int16_t drive);
```

Mount/unmount a disk, tape, or ROM image on drive `drive` (0-based).

### Tape operations

```c
int32_t tap_seek(int32_t pos);
int32_t tap_tell(void);
int32_t tap_read_header(LociTapHdr *hdr);
```

`tap_seek` / `tap_tell` position the virtual tape.  `tap_read_header` reads
the 25-byte Oric tape header at the current position into `*hdr`.

---

## 6. Menu system (menu.h)

Full-screen pulldown menu system with popup dialogs, LIFO window save/restore,
and optional IJK joystick input.  Include `menu.h`.

> **Window save/restore requires LOCI** (overlay RAM).  In Oricutron, menus
> display correctly but screen backgrounds are not restored after a popup closes.

### Architecture

Three layers:
- **Menu bar** (row 1) — green background, six titled items.
- **Pulldown menus** — cyan items with yellow highlight; open below the bar.
- **Popup dialogs** — white background; use the Yes/No pulldown or a sub-menu.

### Capacity constants

| Constant | Value | Meaning |
|---|---|---|
| `MENUBAR_MAXOPTIONS` | 6 | Number of bar items |
| `MENUBAR_MAXLENGTH` | 12 | Max chars per bar title |
| `PULLDOWN_NUMBER` | 11 | Total pulldown menus (0–10) |
| `PULLDOWN_MAXOPTIONS` | 9 | Max items per pulldown |
| `PULLDOWN_MAXLENGTH` | 17 | 16 visible chars + NUL |
| `MENU_WIN_DEPTH` | 9 | Max overlay RAM save depth |

### Return codes

| Constant | Value | Meaning |
|---|---|---|
| `MENU_CANCEL` | 0 | ESC pressed (escapable pulldown) |
| `MENU_LEFT_ARROW` | 18 | Left arrow → open previous bar item |
| `MENU_RIGHT_ARROW` | 19 | Right arrow → open next bar item |
| `MENU_YESNO` | 10 | Index of the Yes/No pulldown |

### Pulldown index map

| Index | Menu |
|---|---|
| 0 | App (confirm mode, return action, filter, sort, Exit) |
| 1 | File (select, delete, rename, copy, move, browse tape) |
| 2 | Dir (root, back, page down/up, top, bottom, new dir) |
| 3 | Mounts (switch pane, next/prev drive, mount, unmount, target, show) |
| 4 | Info (version/credits, help) |
| 5 | Tools (properties, filter by name, view text) |
| 6 | Enter-action sub-menu (Select / Mount / Launch) |
| 7 | Filter sub-menu (None / .DSK / .TAP / .ROM / .LCE) |
| 8 | Target drive sub-menu (A / B / C / D) |
| 9 | Mount source sub-menu (A / B / C / D / Tape / ROM / None) |
| 10 | Yes/No dialog |

`pulldown_options[PULLDOWN_NUMBER]` (number of items per pulldown, indices as
above): `{ 5, 9, 7, 7, 2, 3, 3, 5, 4, 7, 2 }`.

### Data structures

```c
typedef struct {
    uint16_t  overlay_addr;  // address in overlay RAM for saved rows
    uint8_t   ypos;          // first screen row saved
    uint8_t   height;        // number of rows
} MenuWinRecord;

typedef struct {
    char    titles[MENUBAR_MAXOPTIONS][MENUBAR_MAXLENGTH];
    uint8_t xstart[MENUBAR_MAXOPTIONS];  // screen col of highlight attribute
    uint8_t ypos;
} MenuBar;
```

### Exported global data

```c
extern MenuBar menubar;
extern char    pulldown_options[PULLDOWN_NUMBER];
extern char    pulldown_titles[PULLDOWN_NUMBER][PULLDOWN_MAXOPTIONS][PULLDOWN_MAXLENGTH];
```

`pulldown_titles` is writable.  Dynamic entries (confirm mode, filter, sort,
target drive) are updated by the application before calling `menu_main()`.

### Initialisation

```c
void menu_init(void);
```
Reset the window stack, set the overlay RAM pointer, populate `menubar` and
`pulldown_titles` with default values from `MSG_MENU_*` macros, and call
`ijk_detect()`.  Call once after `charwin_init()` and the LOCI check.

### Drawing

```c
void menu_placeheader(const char *header);
```
Draw a green header bar at row 0 containing `header`.

```c
void menu_placebar(uint8_t y);
```
Draw the menu bar at screen row `y`, computing the `xstart[]` positions from
title lengths.

```c
void menu_placetop(const char *header);
```
Clear the screen to black, draw the header at row 0, draw the menu bar at
row 1.  Call this to set up the initial screen layout.

### Menu navigation

```c
uint8_t menu_pulldown(uint8_t xpos, uint8_t ypos,
                      uint8_t menunumber, uint8_t escapable);
```
Open pulldown menu `menunumber` at screen position `(xpos, ypos)`.

| Parameter | Meaning |
|---|---|
| `xpos` | Screen column for the left edge of the pulldown |
| `ypos` | Screen row for the first item |
| `menunumber` | Index into `pulldown_titles[]` (0–9) |
| `escapable` | `1` = ESC closes the menu; `0` = user must choose |

Returns:
- `1–N`: chosen item (1-based)
- `MENU_CANCEL` (0): ESC pressed
- `MENU_LEFT_ARROW` (18): left arrow (for bar-level navigation)
- `MENU_RIGHT_ARROW` (19): right arrow

```c
uint8_t menu_main(void);
```
Run the full menu bar navigation loop.  Opens pulldowns on selection.  Returns
`menubarchoice * 10 + menuoptionchoice` (e.g. `23` = bar item 2, option 3),
or `99` on ESC/STOP.

### Popup dialogs

```c
uint8_t menu_areyousure(const char *message);
```
Show `message` with "Are you sure?" and a Yes/No pulldown.
Returns `1` (Yes) or `2` (No).

```c
void menu_messagepopup(const char *message);
```
Show `message` and wait for any key press.

```c
void menu_fileerrormessage(void);
```
Show a file-error popup displaying `loci_errno`.  Waits for a key press.

```c
uint8_t menu_option_select(const char *message, uint8_t menu);
```
Show `message` above a pulldown from menu index `menu`.  Returns the chosen
item (1-based) or `MENU_CANCEL` (0) if ESC.

### Dynamic menu entries

Items 0–3 of pulldown 0 (App) and item 5 of pulldown 3 (Mounts) reflect
application state.  Update them with `sprintf` before calling `menu_main()`:

```c
menu_init();
sprintf(pulldown_titles[0][0], MSG_MENU_APP_CONFIRM_FMT, MSG_MENU_VAL_ONCE);
sprintf(pulldown_titles[0][1], MSG_MENU_APP_RETURN_FMT,  MSG_MENU_VAL_SELECT);
sprintf(pulldown_titles[0][2], MSG_MENU_APP_FILTER_FMT,  MSG_MENU_VAL_NONE);
sprintf(pulldown_titles[0][3], MSG_MENU_APP_SORT_FMT,    MSG_MENU_VAL_OFF);
sprintf(pulldown_titles[3][5], MSG_MENU_MNT_TARGET_FMT,  "A");
menu_placetop(MSG_MENU_HEADER);
```

Format macros (`MSG_MENU_APP_CONFIRM_FMT` etc.) and value strings
(`MSG_MENU_VAL_ONCE` etc.) are defined in `strings_en.h` / `strings_fr.h`.
The combined output must fit within `PULLDOWN_MAXLENGTH - 1` (16) characters.
Use `sprintf`, not `snprintf` — Oscar64 does not provide `snprintf`.

### Window save/restore internals

The menu module maintains its own LIFO stack using overlay RAM starting at
`MENU_OVERLAY_BASE` (`0xE300`).  Each entry saves full 40-byte rows
(unlike `cwin_push`/`cwin_pop` which saves only content columns).

Overlay RAM partition:
- `0xC000–0xE2FF` — reserved for `cwin_push`/`cwin_pop` (8 depths × 28 rows × 40 bytes)
- `0xE300–0xFFFF` — menu window saves

---

## 7. Build notes

### Compiler

Oscar64 at `/home/xahmol/oscar64/bin/oscar64`.  Standard flags:

```
-n -tf=bin -rt=include/oric_crt.c -i=include -O2 -dNOFLOAT
```

Localisation: add `-dLANG_FR` for the French build.

### #pragma compile chain

Each library header ends with `#pragma compile("filename.c")`.  Oscar64
resolves this path relative to the include search directory (`include/`).
**All `.c` implementation files must live in `include/`** — the compiler
cannot navigate `../` path traversal.

### Known Oscar64 gotchas

| Symptom | Cause | Fix |
|---|---|---|
| `va_arg` crash | `va_arg` is broken in native mode (`-n`) | Do not use `<stdarg.h>` |
| `#if MACRO` fails when set via `-d` | `-d` defines have no value | Use `#ifdef MACRO` |
| Cast applied before member access | Oscar64 parses `(type)struct.member` wrong | Use a temp variable |
| Ternary `? ptr : 0` in pointer function | Parser issue | Use `if`/`return` |
| Large local array crashes | 6502 stack is only 256 bytes | Declare large arrays `static` |
| `snprintf` not found | Not in Oscar64 `<stdio.h>` | Use `sprintf` instead |
