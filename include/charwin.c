// charwin.c - Character window library for Oric Atmos bare-metal
//
// Screen model (confirmed from Oricutron ula.c):
//   Attribute: (byte & 0x60) == 0  → INK/PAPER/mode serial attribute
//   Character:  byte in 0x20-0x7F (normal) or 0xA0-0xFF (inverse)
//   ULA resets to white-ink/black-paper at start of each rasterline.
//   => Cols 0 and 1 of every managed row must hold INK and PAPER attrs.
//
// Based on v1 locifilemanager generic.c windowing by Xander Mol.

#include "oric.h"
#include "keyboard.h"
#include "charwin.h"

// -------------------------------------------------------------------------
// Row address lookup table — computed once in charwin_init()
// row_base[r] = TEXTVRAM + r * SCREEN_COLS
// -------------------------------------------------------------------------

static uint16_t row_base[SCREEN_ROWS];

// -------------------------------------------------------------------------
// Overlay RAM save stack (LIFO, LOCI-only)
// -------------------------------------------------------------------------

#define OVERLAY_STACK_DEPTH 8

typedef struct {
    uint8_t  sy, wy;
    uint16_t addr;
    uint16_t size;
} SaveRecord;

static SaveRecord save_stack[OVERLAY_STACK_DEPTH];
static uint8_t   save_depth;
static uint16_t  overlay_sp;

// -------------------------------------------------------------------------
// Init
// -------------------------------------------------------------------------

void charwin_init(void)
{
    // Use addition to fill row_base — avoids 16-bit multiply on 6502
    uint16_t addr = TEXTVRAM;
    for (uint8_t r = 0; r < SCREEN_ROWS; r++)
    {
        row_base[r] = addr;
        addr = (uint16_t)(addr + SCREEN_COLS);
    }
    overlay_sp = OVERLAY_BASE;
    save_depth = 0;
}

void cwin_init(OricCharWin *w,
               uint8_t sx, uint8_t sy,
               uint8_t wx, uint8_t wy,
               uint8_t ink, uint8_t paper)
{
    if (sx < 2) sx = 2;
    w->sx    = sx;
    w->sy    = sy;
    w->wx    = wx;
    w->wy    = wy;
    w->cx    = 0;
    w->cy    = 0;
    w->ink   = ink;
    w->paper = paper;
}

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------

// Write INK at col 0, PAPER at col 1 of the given absolute screen row.
static void row_setattr(uint8_t row, uint8_t ink, uint8_t paper)
{
    uint8_t *base = (uint8_t *)row_base[row];
    base[0] = ink;    // 0x00-0x07: INK attribute
    base[1] = paper;  // 0x10-0x17: PAPER attribute
}

// -------------------------------------------------------------------------
// Screen I/O
// -------------------------------------------------------------------------

void cwin_clear(OricCharWin *w)
{
    for (uint8_t y = 0; y < w->wy; y++)
    {
        uint8_t  row  = w->sy + y;
        uint8_t *base = (uint8_t *)row_base[row];
        row_setattr(row, w->ink, w->paper);
        for (uint8_t x = w->sx; x < w->sx + w->wx; x++)
            base[x] = 0x20;   // space character
    }
    w->cx = 0;
    w->cy = 0;
}

void cwin_putat_char(OricCharWin *w, uint8_t x, uint8_t y, uint8_t ch)
{
    uint8_t *base = (uint8_t *)row_base[w->sy + y];
    base[w->sx + x] = ch;
}

uint8_t cwin_getat_char(OricCharWin *w, uint8_t x, uint8_t y)
{
    uint8_t *base = (uint8_t *)row_base[w->sy + y];
    return base[w->sx + x];
}

void cwin_putat_string(OricCharWin *w, uint8_t x, uint8_t y, const char *s)
{
    uint8_t *base = (uint8_t *)row_base[w->sy + y] + w->sx + x;
    uint8_t  rem  = w->wx - x;
    while (*s && rem--)
        *base++ = (uint8_t)*s++;
}

void cwin_put_char(OricCharWin *w, uint8_t ch)
{
    if (w->cx >= w->wx) return;
    cwin_putat_char(w, w->cx, w->cy, ch);
    w->cx++;
}

void cwin_put_string(OricCharWin *w, const char *s)
{
    while (*s)
        cwin_put_char(w, (uint8_t)*s++);
}

void cwin_fill_rect(OricCharWin *w,
                    uint8_t x, uint8_t y,
                    uint8_t bw, uint8_t bh,
                    uint8_t ch)
{
    for (uint8_t row = 0; row < bh && (y + row) < w->wy; row++)
    {
        uint8_t *base = (uint8_t *)row_base[w->sy + y + row] + w->sx + x;
        for (uint8_t col = 0; col < bw && (x + col) < w->wx; col++)
            base[col] = ch;
    }
}

void cwin_scroll_up(OricCharWin *w)
{
    // Copy rows upward within window content columns only
    for (uint8_t y = 0; y < w->wy - 1; y++)
    {
        uint8_t *dst = (uint8_t *)row_base[w->sy + y]     + w->sx;
        uint8_t *src = (uint8_t *)row_base[w->sy + y + 1] + w->sx;
        for (uint8_t x = 0; x < w->wx; x++)
            dst[x] = src[x];
    }
    // Clear bottom row: attrs + spaces
    uint8_t last = w->sy + w->wy - 1;
    row_setattr(last, w->ink, w->paper);
    uint8_t *base = (uint8_t *)row_base[last] + w->sx;
    for (uint8_t x = 0; x < w->wx; x++)
        base[x] = 0x20;
}

void cwin_console_put_char(OricCharWin *w, uint8_t ch)
{
    if (ch == '\n')
    {
        w->cx = 0;
        if (w->cy < w->wy - 1)
        {
            w->cy++;
            row_setattr(w->sy + w->cy, w->ink, w->paper);
        }
        else
        {
            cwin_scroll_up(w);
        }
        return;
    }

    cwin_putat_char(w, w->cx, w->cy, ch);
    w->cx++;
    if (w->cx >= w->wx)
    {
        w->cx = 0;
        if (w->cy < w->wy - 1)
        {
            w->cy++;
            row_setattr(w->sy + w->cy, w->ink, w->paper);
        }
        else
        {
            cwin_scroll_up(w);
        }
    }
}

void cwin_console_put_string(OricCharWin *w, const char *s)
{
    while (*s)
        cwin_console_put_char(w, (uint8_t)*s++);
}

// -------------------------------------------------------------------------
// Cursor
// -------------------------------------------------------------------------

void cwin_cursor_show(OricCharWin *w, bool on)
{
    uint8_t *cell = (uint8_t *)row_base[w->sy + w->cy] + w->sx + w->cx;
    if (on)
    {
        // Invert: space→inverse-space block, other chars set bit 7
        *cell = (*cell == 0x20) ? 0xA0 : (*cell | 0x80);
    }
    else
    {
        // Restore: clear bit 7, map inverse-space back to space
        uint8_t c = *cell & 0x7F;
        *cell = (c == 0x00) ? 0x20 : c;   // 0xA0 & 0x7F = 0x00 → space
    }
}

bool cwin_cursor_left(OricCharWin *w)
{
    if (w->cx == 0) return false;
    w->cx--;
    return true;
}

bool cwin_cursor_right(OricCharWin *w)
{
    if (w->cx >= w->wx - 1) return false;
    w->cx++;
    return true;
}

bool cwin_cursor_up(OricCharWin *w)
{
    if (w->cy == 0) return false;
    w->cy--;
    return true;
}

bool cwin_cursor_down(OricCharWin *w)
{
    if (w->cy >= w->wy - 1) return false;
    w->cy++;
    return true;
}

// -------------------------------------------------------------------------
// Overlay RAM save/restore — REQUIRES LOCI device (not available in Oricutron)
// SEI/CLI brackets the overlay-RAM window; ROM is invisible during that time.
// -------------------------------------------------------------------------

void cwin_push(OricCharWin *w)
{
    if (save_depth >= OVERLAY_STACK_DEPTH) return;

    // Compute size as addition to avoid 16-bit multiply on 6502
    uint16_t size = 0;
    for (uint8_t i = 0; i < w->wy; i++) size = (uint16_t)(size + SCREEN_COLS);
    uint16_t src_addr = row_base[w->sy];
    uint16_t dst_addr = overlay_sp;

    save_stack[save_depth].sy   = w->sy;
    save_stack[save_depth].wy   = w->wy;
    save_stack[save_depth].addr = dst_addr;
    save_stack[save_depth].size = size;
    save_depth++;

    __asm { sei }
    MICRODISCCFG = OVERLAY_ON;

    uint8_t *src = (uint8_t *)src_addr;
    uint8_t *dst = (uint8_t *)dst_addr;
    for (uint16_t i = 0; i < size; i++)
        dst[i] = src[i];

    overlay_sp  += size;
    MICRODISCCFG = OVERLAY_OFF;
    __asm { cli }
}

void cwin_pop(OricCharWin *w)
{
    if (save_depth == 0) return;

    save_depth--;
    uint16_t src_addr = save_stack[save_depth].addr;
    uint16_t size     = save_stack[save_depth].size;
    uint8_t  sy       = save_stack[save_depth].sy;
    uint16_t dst_addr = row_base[sy];

    __asm { sei }
    MICRODISCCFG = OVERLAY_ON;

    uint8_t *src = (uint8_t *)src_addr;
    uint8_t *dst = (uint8_t *)dst_addr;
    for (uint16_t i = 0; i < size; i++)
        dst[i] = src[i];

    overlay_sp  -= size;
    MICRODISCCFG = OVERLAY_OFF;
    __asm { cli }

    (void)w;  // w is unused; geometry is stored in save_stack
}

// -------------------------------------------------------------------------
// Key input
// -------------------------------------------------------------------------

uint8_t cwin_getch(void)
{
    return keyb_getch();
}

// -------------------------------------------------------------------------
// Text input widget
//
// Based on v1 locifilemanager textInput() by Xander Mol, adapted from
// DraBrowse 1.0e by Sascha Bader (2009). Adapted for charwin API and Oscar64.
// -------------------------------------------------------------------------

signed int cwin_textinput(OricCharWin *w,
                          uint8_t x, uint8_t y,
                          uint8_t vwidth,
                          char *str, uint8_t maxlen,
                          uint8_t validation)
{
    uint8_t idx = 0;
    while (str[idx]) idx++;   // position cursor at existing content end

    while (1)
    {
        // Count current length
        uint8_t len = 0;
        while (str[len]) len++;

        // Viewport offset: keep cursor visible in right portion
        uint8_t offs = (idx + 1 > vwidth) ? (uint8_t)(idx + 1 - vwidth) : 0;

        // Redraw viewport: clear, print vwidth-1 visible chars, draw cursor
        for (uint8_t i = 0; i < vwidth; i++)
            cwin_putat_char(w, x + i, y, 0x20);

        for (uint8_t i = 0; i < vwidth - 1 && str[offs + i]; i++)
            cwin_putat_char(w, x + i, y, (uint8_t)str[offs + i]);

        uint8_t cur_ch = str[idx] ? (uint8_t)(str[idx] | 0x80) : 0xA0;
        cwin_putat_char(w, x + (uint8_t)(idx - offs), y, cur_ch);

        // Get key
        uint8_t c = cwin_getch();

        switch (c)
        {
        case KEY_ESC:
            for (uint8_t i = 0; i < vwidth; i++)
                cwin_putat_char(w, x + i, y, 0x20);
            return -1;

        case KEY_ENTER:
            for (uint8_t i = 0; i < vwidth; i++)
                cwin_putat_char(w, x + i, y, 0x20);
            for (uint8_t i = 0; i < vwidth && str[i]; i++)
                cwin_putat_char(w, x + i, y, (uint8_t)str[i]);
            return (signed int)len;

        case KEY_DEL:
            if (idx > 0)
            {
                idx--;
                uint8_t i = idx;
                while (str[i + 1]) { str[i] = str[i + 1]; i++; }
                str[i] = 0;
            }
            break;

        case KEY_LEFT:
            if (idx > 0) idx--;
            break;

        case KEY_RIGHT:
            if (idx < len && idx < maxlen) idx++;
            break;

        default:
        {
            bool valid = (validation == VINPUT_ALL);
            if ((validation & VINPUT_NUMS)  && c >= '0' && c <= '9') valid = true;
            if ((validation & VINPUT_ALPHA) && c >= 'A' && c <= 'Z') valid = true;
            if ((validation & VINPUT_ALPHA) && c >= 'a' && c <= 'z') valid = true;
            if ((validation & VINPUT_WILD)  && (c == '*' || c == '?')) valid = true;

            if (valid && c >= 0x20 && c <= 0x7E && idx < maxlen)
            {
                // Overwrite at idx; extend string if at null terminator
                if (str[idx] == 0) str[idx + 1] = 0;
                str[idx] = (char)c;
                idx++;
            }
            break;
        }
        }
    }
    return 0;
}
