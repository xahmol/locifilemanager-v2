// loci.c - LOCI mass-storage device library for Oscar64 / Oric Atmos (bare-metal)
//
// Based on:
//   LOCI ROM by Sodiumlightbaby, 2024  https://github.com/sodiumlb/loci-rom
//   Picocomputer 6502 by Rumbledethumps, 2023  https://github.com/picocomputer/rp6502
//   locifilemanager v1 (CC65) by Xander Mol, 2025
//     libsrc/mia.s, open.c, read_xstack.c, opendir.c, readdir.c,
//     fileops.c, mount.c, xram_memcpy.s, ijk-driver.s, getstoragecfg.c
//
// Adapted: Oscar64 native mode (-n); C functions replace assembly helpers;
// stdint types throughout; SEI/CLI via __asm for IRQ-safe sections.

#include "loci.h"
#include "charwin.h"
#include "strings.h"

// ─────────────────────────────────────────────────────────────────────────────
// Global state
// ─────────────────────────────────────────────────────────────────────────────

uint8_t loci_errno = 0;
LociCfg locicfg;

// ─────────────────────────────────────────────────────────────────────────────
// MIA helper implementations
//
// Calling convention (matches CC65 v1 mia.s byte ordering — see loci.h header).
// Field names MIA.areg / MIA.xreg used instead of MIA.a / MIA.x because
// Oscar64 native mode (-n) treats 'a' and 'x' as 6502 register keywords.
// ─────────────────────────────────────────────────────────────────────────────

void mia_push_int(uint16_t v)
{
    MIA.xstack = (uint8_t)(v >> 8);
    MIA.xstack = (uint8_t)v;
}

int16_t mia_pop_int(void)
{
    uint8_t lo = MIA.xstack;
    uint8_t hi = MIA.xstack;
    return (int16_t)((uint16_t)hi << 8 | lo);
}

void mia_push_long(uint32_t v)
{
    MIA.xstack = (uint8_t)(v >> 24);
    MIA.xstack = (uint8_t)(v >> 16);
    MIA.xstack = (uint8_t)(v >> 8);
    MIA.xstack = (uint8_t)v;
}

uint32_t mia_pop_long(void)
{
    uint8_t b0 = MIA.xstack;
    uint8_t b1 = MIA.xstack;
    uint8_t b2 = MIA.xstack;
    uint8_t b3 = MIA.xstack;
    return ((uint32_t)b3 << 24) | ((uint32_t)b2 << 16) | ((uint32_t)b1 << 8) | (uint32_t)b0;
}

void mia_set_ax(uint16_t v)
{
    MIA.xreg = (uint8_t)(v >> 8);
    MIA.areg = (uint8_t)v;
}

void mia_set_axsreg(uint32_t v)
{
    MIA.sreg = (uint16_t)(v >> 16);
    mia_set_ax((uint16_t)v);
}

// MIA_OP_BOOT (with the FAST bit boot() always sets) ends its MIA.spin
// sequence with api_return_boot()'s "CLV; BVC+0; JMP ($FFFC)" instead of
// the normal "CLV; BVC+0; LDA #lo; LDX #hi; RTS" — that JMP to the 6502
// reset vector IS the reboot. Polling MIA.busy and reading MIA.areg/xreg
// as plain data (the previous implementation) never executes it, so boot
// silently never reboots, and the $FC/$FF JMP-operand bytes misread as
// AX=$FFFC (-4), a false error. JSR MIA.spin ($03B1) executes the
// firmware-written sequence directly: it blocks via an in-place BVC loop
// while busy, then either returns with the result in A/X (normal ops) or
// jumps into the freshly booted ROM and never returns (MIA_OP_BOOT
// success). Matches v1 mia.s _mia_call_int (sta MIA_OP; jmp MIA_SPIN).
//
// CLV before the JSR is required: $03B1 is the BVC opcode itself (the
// CLV the firmware wrote sits at $03B0, one byte EARLIER) so JSR 0x03b1
// enters past it, at the mercy of whatever the V flag happens to be from
// preceding code. If V is set on entry, "BVC -2" (the busy pattern) does
// NOT branch on the first check and falls straight through to the stale
// LDA #areg/LDX #xreg/RTS bytes — returning immediately with garbage
// before the firmware has even started, right as it begins overwriting
// low RAM for the boot. At -O2 a preceding BIT-style flag test can leave
// V set depending on the call site, so this can't be left to chance.
int16_t mia_call_int(uint8_t op)
{
    return __asm {
        lda op
        sta [0x03af]
        clv
        jsr 0x03b1
        sta accu
        stx accu + 1
    };
}

int16_t mia_call_int_errno(uint8_t op)
{
    int16_t r = mia_call_int(op);
    if (r < 0) { loci_errno = MIA.errno_lo; return -1; }
    return r;
}

// MIA_OP_BOOT only. mia_call_int's "sta [0x03af]; jsr 0x03b1" — writing
// the op and immediately JSR-ing into the busy/done dispatch at $03B1 —
// hangs on real LOCI hardware for this op specifically (confirmed: same
// settings via mia_call_int_errno(MIA_OP_BOOT) freeze with no further
// code executing, while the sequence below completes and reboots
// correctly for both ESC-exit ($80) and TAP-launch ($92) settings).
// Splitting the wait from the jump avoids whatever the hazard is: poll
// MIA.busy as plain data until the firmware finishes (success rewrites
// $03B0-$03B7 to "CLV;BVC+0;JMP($FFFC)"; failure rewrites it to the
// normal "CLV;BVC+0;LDA#lo;LDX#hi;RTS" released form), THEN jsr 0x03b1.
// On success that JMP reboots the machine and this never returns; on
// failure it returns the LDA/LDX result like mia_call_int.
int16_t mia_call_boot(uint8_t settings)
{
    int16_t r;

    mia_set_ax((uint16_t)settings);
    MIA.op = MIA_OP_BOOT;

    while (MIA.busy & MIA_BUSY_BIT) { }

    // On success the jump below lands in the freshly-loaded ROM's cold-start
    // (e.g. BASIC10 $F84A), which performs its own VIA init and eventually
    // executes CLI. v2 runs permanently under SEI with no IRQ handler, so
    // VIA.ifr can accumulate a stale, unacknowledged Timer 1 flag (set once,
    // ~10ms after startup, and never cleared since nothing ever reads T1C-L
    // or rewrites T1C-H). If the cold-start enables VIA.ier's T1 bit before
    // it clears/reads T1C-L, that stale IFR bit fires an IRQ the instant it
    // executes CLI -- before its own zero-page setup is complete -- hanging
    // the machine. v1 avoided this by running with IRQs enabled throughout
    // (IFR serviced every jiffy) and writing VIA.ier = 0x7F just before this
    // same call. Here we both disable IER and clear IFR, leaving VIA in a
    // clean, reset-like state for the booted ROM to initialise from.
    VIA.ier = 0x7F;
    VIA.ifr = 0x7F;

    r = __asm {
        jsr 0x03b1
        sta accu
        stx accu + 1
    };

    if (r < 0) { loci_errno = MIA.errno_lo; return -1; }
    return r;
}

// JSR-based, like mia_call_int (see its comment) — executes the firmware's
// busy/done routine at MIA.spin rather than reading MIA.areg/xreg/sreg as
// plain data. Matches v1 mia.s _mia_call_long (sta MIA_OP; jsr MIA_SPIN;
// ldy MIA_SREG; ...). None of mia_call_long's ops end in a JMP-style
// return, so A/X come back as the normal LDA #lo / LDX #hi result.
//
// CLV before the JSR — see mia_call_int's comment: $03B1 is the BVC
// opcode itself, one byte past the firmware's own CLV at $03B0, so the
// busy-loop's correctness depends on V being clear on entry.
int32_t mia_call_long(uint8_t op)
{
    return __asm {
        lda op
        sta [0x03af]
        clv
        jsr 0x03b1
        sta accu
        stx accu + 1
        lda [0x03b8]
        sta accu + 2
        lda [0x03b9]
        sta accu + 3
    };
}

int32_t mia_call_long_errno(uint8_t op)
{
    int32_t r = mia_call_long(op);
    if (r < 0) { loci_errno = MIA.errno_lo; return -1; }
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers (static — not part of the public API)
// ─────────────────────────────────────────────────────────────────────────────

// Push a null-terminated string to XSTACK in reverse order (last char first).
// MIA firmware reverses on dequeue, presenting bytes in forward order.
static void push_path(const char *s)
{
    uint8_t len = 0;
    while (s[len]) len++;
    while (len) mia_push_char((uint8_t)s[--len]);
}

// Read up to count bytes from an open file to buf via XSTACK (≤256 per call).
static int16_t read_xstack(void *buf, uint16_t count, int16_t fildes)
{
    int16_t got;
    mia_push_int(count);
    mia_set_ax((uint16_t)fildes);
    got = mia_call_int_errno(MIA_OP_READ_XSTACK);
    if (got > 0)
    {
        uint8_t *bbuf = (uint8_t *)buf;
        int16_t i;
        for (i = 0; i < got; i++)
        {
            uint8_t ch = MIA.xstack;
            bbuf[i] = ch;
        }
    }
    return got;
}

// Write up to count bytes from buf to an open file via XSTACK (≤256 per call).
static int16_t write_xstack(const void *buf, uint16_t count, int16_t fildes)
{
    uint16_t i = count;
    while (i) mia_push_char(((const uint8_t *)buf)[--i]);
    mia_push_int(count);
    mia_set_ax((uint16_t)fildes);
    return mia_call_int_errno(MIA_OP_WRITE_XSTACK);
}

// Read count bytes from an open file into XRAM at xram_addr.
//
// xram_addr and count are passed via XSTACK (buf pushed first, then count
// — MIA_OP_READ_XRAM pops count first, then xram_addr), matching v1's
// read_xram() (libsrc/read_xram.c). MIA.addr0/.step0/.rw0 are a *different*
// direct register DMA window (used by xram_memcpy_*()/xram_peek/poke) and
// are not consulted by this op — writing MIA.addr0 here left only 2 bytes
// on XSTACK, one short of the 4 MIA_OP_READ_XRAM expects, so the firmware
// returned EINVAL on every call.
static int16_t read_xram(uint16_t xram_addr, uint16_t count, int16_t fildes)
{
    mia_push_int(xram_addr);
    mia_push_int(count);
    mia_set_ax((uint16_t)fildes);
    return mia_call_int_errno(MIA_OP_READ_XRAM);
}

// Write count bytes from XRAM at xram_addr to an open file. See read_xram()
// for why xram_addr/count are pushed via XSTACK rather than MIA.addr1.
static int16_t write_xram(uint16_t xram_addr, uint16_t count, int16_t fildes)
{
    mia_push_int(xram_addr);
    mia_push_int(count);
    mia_set_ax((uint16_t)fildes);
    return mia_call_int_errno(MIA_OP_WRITE_XRAM);
}

// ─────────────────────────────────────────────────────────────────────────────
// Detection & configuration
// ─────────────────────────────────────────────────────────────────────────────

bool loci_present(void)
{
    return *LOCI_SIGNATURE_ADDR == 'L';
}

// uname: populate LociUname struct via XSTACK.
// Firmware pops sizeof(LociUname) == 69 bytes from XSTACK after MIA_OP_UNAME.
// Based on sysuname.c in sodiumlb/loci-rom (XSTACK loop, not DMA).
void loci_uname(LociUname *buf)
{
    uint8_t  i;
    uint8_t *p = (uint8_t *)buf;
    mia_call_int(MIA_OP_UNAME);
    for (i = 0; i < (uint8_t)sizeof(LociUname); i++)
    {
        uint8_t ch = MIA.xstack;
        p[i] = ch;
    }
}

void get_locicfg(void)
{
    uint8_t     devid;
    LociDir    *dir;
    LociDirent *fil;

    if (!loci_present())
    {
        OricCharWin err;
        cwin_init(&err, 2, 12, 38, 2, A_FWRED, A_BGBLACK);
        cwin_clear(&err);
        cwin_putat_string(&err, 0, 0, MSG_LOCI_NOT_FOUND);
        cwin_putat_string(&err, 0, 1, MSG_PRESS_KEY_EXIT);
        cwin_getch();
        while (1) {}    // bare-metal halt (no exit() on Oric)
    }

    // Zero-fill config struct
    {
        uint8_t *p = (uint8_t *)&locicfg;
        uint8_t  i;
        for (i = 0; i < sizeof(locicfg); i++) p[i] = 0;
    }

    // Get firmware version via uname release string (e.g. "1.2.3" or "1.2.34")
    loci_uname(&locicfg.uname);
    {
        const char *rel = locicfg.uname.release;
        locicfg.version.major = (uint8_t)(rel[0] - '0');
        locicfg.version.minor = (uint8_t)(rel[2] - '0');
        locicfg.version.patch = (uint8_t)(rel[4] - '0');
        if (rel[5] && rel[5] != '\0')
            locicfg.version.patch = (uint8_t)(locicfg.version.patch * 10 + (rel[5] - '0'));
    }

    // Drive 0 always valid
    locicfg.validdev[0] = 1;

    // Walk root dir to enumerate USB MSC devices (entries "N.MSC.*")
    dir = loci_opendir("");
    if (!dir) return;
    while (1)
    {
        fil = loci_readdir(dir);
        if (!fil || fil->d_name[0] == '\0') break;
        if (locicfg.devnr >= MAXDEV) break;

        devid = (uint8_t)(fil->d_name[0] - '0');
        if (devid && fil->d_name[3] == 'M' && fil->d_name[4] == 'S' && fil->d_name[5] == 'C')
        {
            locicfg.devnr++;
            locicfg.validdev[devid] = 1;
        }
    }
    loci_closedir(dir);
}

bool loci_check_fw(uint8_t major, uint8_t minor, uint8_t patch)
{
    if (locicfg.version.major > major) return true;
    if (locicfg.version.major == major && locicfg.version.minor > minor) return true;
    if (locicfg.version.major == major && locicfg.version.minor == minor &&
        locicfg.version.patch >= patch) return true;
    return false;
}

const char *get_loci_devname(uint8_t devid, uint8_t maxlength)
{
    static LociDirent entry;
    LociDir    *dir;
    LociDirent *fil = 0;
    uint8_t     i;

    dir = loci_opendir("");
    for (i = 0; i <= devid; i++)
        fil = loci_readdir(dir);
    loci_closedir(dir);

    if (!fil) return "";

    // Copy to static entry so result survives the dir close
    for (i = 0; i < 64; i++) entry.d_name[i] = fil->d_name[i];

    // Truncate at maxlength (name starts at d_name[3])
    {
        char   *name = entry.d_name + 3;
        uint8_t len  = 0;
        while (name[len]) len++;
        if (len > maxlength) name[maxlength] = '\0';
        return name;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// System
// ─────────────────────────────────────────────────────────────────────────────

int16_t phi2(void)
{
    return mia_call_int(MIA_OP_PHI2);
}

int32_t loci_lrand(void)
{
    return mia_call_long(MIA_OP_LRAND);
}

// ─────────────────────────────────────────────────────────────────────────────
// XRAM direct access
//
// Protocol (from libsrc/xram_memcpy.s):
//   step0 = 1 (auto-increment address after each byte)
//   addr0 = target/source XRAM address
//   Writes: store bytes to rw0; spin after last byte
//   Reads:  load bytes from rw0 (synchronous, no spin needed)
// ─────────────────────────────────────────────────────────────────────────────

void xram_poke(uint16_t addr, uint8_t val)
{
    MIA.step0 = 1;
    MIA.addr0 = addr;
    MIA.rw0   = val;
    while (MIA.busy & MIA_BUSY_BIT) {}
}

uint8_t xram_peek(uint16_t addr)
{
    MIA.step0 = 1;
    MIA.addr0 = addr;
    return MIA.rw0;
}

void xram_memcpy_to(uint16_t dest, const void *src, uint16_t count)
{
    const uint8_t *p = (const uint8_t *)src;
    uint16_t       i;
    MIA.step0 = 1;
    MIA.addr0 = dest;
    for (i = 0; i < count; i++)
        MIA.rw0 = p[i];
    while (MIA.busy & MIA_BUSY_BIT) {}
}

void xram_memcpy_from(void *dest, uint16_t src, uint16_t count)
{
    uint8_t  *p = (uint8_t *)dest;
    uint16_t  i;
    MIA.step0 = 1;
    MIA.addr0 = src;
    for (i = 0; i < count; i++)
        p[i] = MIA.rw0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Overlay RAM ($C000–$FFFF via MICRODISCCFG $0314)
// ─────────────────────────────────────────────────────────────────────────────

void enable_overlay_ram(void)  { MICRODISCCFG = 0xFD; }
void disable_overlay_ram(void) { MICRODISCCFG = 0xFF; }

// ─────────────────────────────────────────────────────────────────────────────
// File I/O
// ─────────────────────────────────────────────────────────────────────────────

int16_t loci_open(const char *path, uint16_t flags)
{
    push_path(path);
    mia_set_ax(flags);
    return mia_call_int_errno(MIA_OP_OPEN);
}

int16_t loci_close(int16_t fd)
{
    mia_set_ax((uint16_t)fd);
    return mia_call_int_errno(MIA_OP_CLOSE);
}

int16_t loci_read(int16_t fd, void *buf, uint16_t count)
{
    int16_t  total = 0;
    uint8_t *p     = (uint8_t *)buf;
    while (count)
    {
        uint16_t block = (count > 256) ? 256 : count;
        int16_t  got   = read_xstack(p + total, block, fd);
        if (got < 0) return got;
        total = (int16_t)(total + got);
        count = (uint16_t)(count - (uint16_t)got);
        if ((uint16_t)got < block) break;
    }
    return total;
}

int16_t loci_write(int16_t fd, const void *buf, uint16_t count)
{
    int16_t        total = 0;
    const uint8_t *p     = (const uint8_t *)buf;
    while (count)
    {
        uint16_t block = (count > 256) ? 256 : count;
        int16_t  put   = write_xstack(p + total, block, fd);
        if (put < 0) return put;
        total = (int16_t)(total + put);
        count = (uint16_t)(count - (uint16_t)put);
        if ((uint16_t)put < block) break;
    }
    return total;
}

int32_t loci_lseek(int16_t fd, int32_t offset, uint8_t whence)
{
    mia_push_long((uint32_t)offset);
    mia_push_char(whence);
    mia_set_ax((uint16_t)fd);
    return mia_call_long_errno(MIA_OP_LSEEK);
}

int16_t loci_unlink(const char *path)
{
    push_path(path);
    return mia_call_int_errno(MIA_OP_UNLINK);
}

int16_t loci_rename(const char *oldpath, const char *newpath)
{
    // Push old path, then a NUL separator, then new path (each reversed).
    // MIA firmware reverses each segment back to forward order on dequeue.
    // Matches sysrename.c in sodiumlb/loci-rom (libsrc reference client).
    push_path(oldpath);
    mia_push_char(0);
    push_path(newpath);
    return mia_call_int_errno(MIA_OP_RENAME);
}

// ─────────────────────────────────────────────────────────────────────────────
// High-level file operations
// ─────────────────────────────────────────────────────────────────────────────

bool file_exists(const char *path)
{
    int16_t fd = loci_open(path, O_RDONLY | O_EXCL);
    if (fd < 0) return false;
    loci_close(fd);
    return true;
}

int16_t file_load(const char *path, void *dst, uint16_t count)
{
    int16_t fd = loci_open(path, O_RDONLY | O_EXCL);
    int16_t r;
    if (fd < 0) return fd;
    r = loci_read(fd, dst, count);
    loci_close(fd);
    return r;
}

int16_t file_save(const char *path, const void *src, uint16_t count)
{
    int16_t fd = loci_open(path, O_WRONLY | O_CREAT);
    int16_t r;
    if (fd < 0) return fd;
    r = loci_write(fd, src, count);
    loci_close(fd);
    return r;
}

int16_t file_copy(const char *dst, const char *src)
{
    int16_t fd_src, fd_dst;
    int16_t len;
    int16_t result = 0;

    fd_src = loci_open(src, O_RDONLY | O_EXCL);
    if (fd_src < 0) return fd_src;

    fd_dst = loci_open(dst, O_WRONLY | O_CREAT);
    if (fd_dst < 0) { loci_close(fd_src); return fd_dst; }

    do {
        len = read_xram(COPYBUF_XRAM_ADDR, COPYBUF_XRAM_SIZE, fd_src);
        if (len < 0) { result = len; break; }
        if (len > 0)
        {
            int16_t wr = write_xram(COPYBUF_XRAM_ADDR, (uint16_t)len, fd_dst);
            if (wr < 0) { result = wr; break; }
        }
    } while (len == (int16_t)COPYBUF_XRAM_SIZE);

    loci_close(fd_src);
    loci_close(fd_dst);
    return result;
}

// Animated progress-bar characters, cycled every read/write block.
static const uint8_t progressBar[4] = { 48, 53, 93, 95 };

// Based on libsrc/fileops.c file_copy() (CC65, prog/progx/progy/progl args) by
// Xander Mol, 2025 — local reference at locifilemanager/libsrc/fileops.c.
// Adapted: no conio gotoxy/cputc/cclear in Oscar64 bare-metal — progress bar
// chars are written directly to TEXTVRAM via a row pointer (same pattern as
// menu.c's MENU_ROW macro).
int16_t file_copy_progress(const char *dst, const char *src,
                            uint8_t progx, uint8_t progy, uint8_t progl)
{
    uint8_t *row = (uint8_t *)((uint16_t)TEXTVRAM + (uint16_t)progy * 40U);
    int16_t  fd_src, fd_dst;
    int16_t  len;
    int16_t  result = 0;
    uint8_t  cnt = 0;
    uint8_t  x;

    fd_src = loci_open(src, O_RDONLY | O_EXCL);
    if (fd_src < 0) return fd_src;

    fd_dst = loci_open(dst, O_WRONLY | O_CREAT);
    if (fd_dst < 0) { loci_close(fd_src); return fd_dst; }

    row[progx] = A_ALT;
    for (x = 0; x < progl; x++) row[progx + 1 + x] = 0x20;

    do {
        if (keyb_check() == KEY_ESC) { result = -2; break; }

        if ((cnt >> 2) > (uint8_t)(progl - 2))
        {
            cnt = 0;
            for (x = 0; x < (uint8_t)(progl - 1); x++)
                row[progx + 1 + x] = 0x20;
        }
        else
        {
            row[progx + 1 + (cnt >> 2)] = progressBar[cnt & 3];
            cnt++;
        }

        len = read_xram(COPYBUF_XRAM_ADDR, COPYBUF_XRAM_SIZE, fd_src);
        if (len < 0) { result = len; break; }
        if (len > 0)
        {
            int16_t wr = write_xram(COPYBUF_XRAM_ADDR, (uint16_t)len, fd_dst);
            if (wr < 0) { result = wr; break; }
        }
    } while (len == (int16_t)COPYBUF_XRAM_SIZE);

    loci_close(fd_src);
    loci_close(fd_dst);

    // -2 = cancelled mid-copy (ESC) -- remove the partial destination file.
    if (result == -2)
        loci_unlink(dst);

    for (x = 0; x < progl; x++) row[progx + x] = 0x20;

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Directory operations
// ─────────────────────────────────────────────────────────────────────────────

static LociDir    s_dir;
static LociDirent s_dirent;

LociDir *loci_opendir(const char *path)
{
    int16_t fd;
    uint8_t i = 0;
    // Flush any XSTACK residue from prior operations (e.g. loci_uname)
    // before pushing the path argument so the firmware sees a clean stack.
    mia_call_int(MIA_OP_ZXSTACK);
    push_path(path);
    fd = mia_call_int_errno(MIA_OP_OPENDIR);
    s_dir.fd  = fd;
    s_dir.off = 0;
    while (path[i] && i < 63) { s_dir.name[i] = path[i]; i++; }
    s_dir.name[i] = '\0';
    if (fd < 0) return 0;
    return &s_dir;
}

void loci_closedir(LociDir *dir)
{
    mia_set_ax((uint16_t)dir->fd);
    mia_call_int_errno(MIA_OP_CLOSEDIR);
}

LociDirent *loci_readdir(LociDir *dir)
{
    uint8_t i;
    uint8_t *p;
    mia_set_ax((uint16_t)dir->fd);
    if (mia_call_int_errno(MIA_OP_READDIR) < 0) return 0;

    // MIA pops LOCI_DIRENT_SIZE bytes from XSTACK in forward order
    p = (uint8_t *)&s_dirent;
    for (i = 0; i < LOCI_DIRENT_SIZE; i++)
    {
        uint8_t ch = MIA.xstack;
        p[i] = ch;
    }

    dir->off++;
    return &s_dirent;
}

int16_t loci_mkdir(const char *path)
{
    push_path(path);
    return mia_call_int_errno(MIA_OP_MKDIR);
}

// getcwd protocol (from initcwd.s in sodiumlb/loci-rom):
//   ax = max_length (len-1); call MIA_OP_GETCWD; then pop bytes from XSTACK
//   until '\0' or max reached.
void loci_getcwd(char *buf, uint8_t len)
{
    uint8_t i = 0;
    mia_set_ax((uint16_t)(len - 1));
    mia_call_int_errno(MIA_OP_GETCWD);
    while (i < (uint8_t)(len - 1))
    {
        uint8_t ch = MIA.xstack;
        buf[i++] = (char)ch;
        if (!ch) return;
    }
    buf[i] = '\0';
}

// ─────────────────────────────────────────────────────────────────────────────
// Mount operations
// ─────────────────────────────────────────────────────────────────────────────

int16_t loci_mount(int16_t drive, const char *path, const char *filename)
{
    mia_set_ax((uint16_t)drive);
    push_path(filename);
    mia_push_char('/');
    push_path(path);
    return mia_call_int_errno(MIA_OP_MOUNT);
}

int16_t loci_umount(int16_t drive)
{
    mia_set_ax((uint16_t)drive);
    return mia_call_int_errno(MIA_OP_UMOUNT);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tape operations
// ─────────────────────────────────────────────────────────────────────────────

int32_t tap_seek(int32_t pos)
{
    // MIA_OP_TAP_SEEK reads its single argument from API_AXSREG (registers),
    // not XSTACK — matches v1 tap.c tap_seek() (mia_set_axsreg).
    mia_set_axsreg((uint32_t)pos);
    return mia_call_long_errno(MIA_OP_TAP_SEEK);
}

int32_t tap_tell(void)
{
    return mia_call_long_errno(MIA_OP_TAP_TELL);
}

// MIA_OP_TAP_HDR pushes sizeof(LociTapHdr) header bytes onto XSTACK
// (regardless of success/failure) — pop them into *hdr, matching v1
// tap.c tap_read_header().
int32_t tap_read_header(LociTapHdr *hdr)
{
    int32_t  pos = mia_call_long_errno(MIA_OP_TAP_HDR);
    uint8_t *h   = (uint8_t *)hdr;
    uint8_t  i;
    for (i = 0; i < sizeof(LociTapHdr); i++)
        h[i] = MIA.xstack;
    return pos;
}

