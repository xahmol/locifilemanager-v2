// viewer.c - Full-screen paged text/hex file viewer for locifilemanager v2
//
// Reads a file in chunks and displays it either word-wrapped (text mode,
// splitting on '\n' and ignoring '\r', via cwin_printwrap()) or as a hex
// dump with an ASCII column (hex mode). Pagination is forward-only: once
// the content window's last row has been used, the footer shows a prompt
// and pauses for a keypress before clearing the window for the next page.
// ESC exits at any pause point; X toggles between text and hex mode,
// restarting the file from the beginning in the new mode.

#include <stdint.h>
#include <stdbool.h>
#include "oric.h"
#include "charwin.h"
#include "keyboard.h"
#include "loci.h"
#include "menu.h"
#include "strings.h"
#include "viewer.h"

#define VIEWER_CHUNK_SIZE 512
#define VIEWER_LINE_MAX   128
#define VIEWER_HEX_PER_LINE 8

// Bytes outside printable ASCII are shown as this placeholder -- raw control
// bytes (0x00-0x1F) are charwin screen attributes, not characters (see "Oric
// Screen Model" in CLAUDE.md), and an embedded NUL would truncate
// cwin_printwrap()'s string early, silently dropping the rest of the line.
#define VIEWER_PLACEHOLDER_CHAR '.'

// Result of a pause-point keypress: which mode the caller should switch to,
// or that the viewer should close entirely.
#define VIEWER_KEY_EXIT     0
#define VIEWER_KEY_TOGGLE   1
#define VIEWER_KEY_CONTINUE 2

static char viewer_chunk[VIEWER_CHUNK_SIZE];
static char viewer_line[VIEWER_LINE_MAX];

// Show a pause-point prompt naming the mode X would switch to, then wait
// for a key: ESC -> exit, X/x -> toggle mode, anything else -> continue.
static uint8_t viewer_wait_key(OricCharWin *footer, const char *fmt, const char *togglemode)
{
    cwin_putat_printf(footer, 0, 0, fmt, togglemode);
    uint8_t key = cwin_getch();
    if (key == KEY_ESC) return VIEWER_KEY_EXIT;
    if (key == 'x' || key == 'X') return VIEWER_KEY_TOGGLE;
    return VIEWER_KEY_CONTINUE;
}

// Word-wrap the buffered line into content, then either advance to the next
// line or, if the page is now full, pause for a keypress and start a fresh
// page.
static uint8_t viewer_flush_line(OricCharWin *content, OricCharWin *footer, uint8_t *linelen, const char *togglemode)
{
    viewer_line[*linelen] = 0;
    cwin_printwrap(content, viewer_line);
    *linelen = 0;

    if (content->cy >= content->wy - 1)
    {
        uint8_t result = viewer_wait_key(footer, MSG_VIEWER_PRESS_KEY_FMT, togglemode);
        if (result != VIEWER_KEY_CONTINUE) return result;
        cwin_clear(content);
        cwin_clear(footer);
    }
    else
    {
        cwin_console_put_char(content, '\n');
    }
    return VIEWER_KEY_CONTINUE;
}

// Word-wrapped text view: reads fd from its current position to EOF (or
// until the user exits/toggles), sanitizing non-printable bytes.
static uint8_t viewer_run_text(OricCharWin *content, OricCharWin *footer, int16_t fd, const char *togglemode)
{
    int16_t got;
    uint8_t linelen = 0;
    uint8_t result = VIEWER_KEY_CONTINUE;

    while (result == VIEWER_KEY_CONTINUE)
    {
        got = loci_read(fd, viewer_chunk, VIEWER_CHUNK_SIZE);
        if (got <= 0) break;

        for (uint16_t i = 0; i < (uint16_t)got; i++)
        {
            uint8_t ch = (uint8_t)viewer_chunk[i];

            if (ch == '\r') continue;

            if (ch == '\n')
            {
                result = viewer_flush_line(content, footer, &linelen, togglemode);
                if (result != VIEWER_KEY_CONTINUE) break;
                continue;
            }

            if (linelen >= VIEWER_LINE_MAX - 1)
            {
                result = viewer_flush_line(content, footer, &linelen, togglemode);
                if (result != VIEWER_KEY_CONTINUE) break;
            }

            if (ch < 0x20 || ch > 0x7E)
                ch = VIEWER_PLACEHOLDER_CHAR;
            viewer_line[linelen++] = (char)ch;
        }
    }

    if (result == VIEWER_KEY_CONTINUE && linelen > 0)
        result = viewer_flush_line(content, footer, &linelen, togglemode);

    if (result == VIEWER_KEY_CONTINUE)
        result = viewer_wait_key(footer, MSG_VIEWER_EOF_FMT, togglemode);

    return result;
}

// Render one hex-dump row: "OFFS: b0 b1 .. b7" followed by an 8-char ASCII
// column (sanitized the same way as text mode). n < VIEWER_HEX_PER_LINE for
// the final, partial row -- unused hex/ASCII columns are left blank.
static void viewer_print_hexline(OricCharWin *content, uint8_t row, uint16_t offset, const uint8_t *bytes, uint8_t n)
{
    cwin_putat_printf(content, 0, row, "%04x: ", offset);
    for (uint8_t b = 0; b < n; b++)
        cwin_putat_printf(content, 6 + b * 3, row, "%02x ", bytes[b]);
    for (uint8_t b = 0; b < n; b++)
    {
        uint8_t ch = bytes[b];
        if (ch < 0x20 || ch > 0x7E)
            ch = VIEWER_PLACEHOLDER_CHAR;
        cwin_putat_char(content, 30 + b, row, ch);
    }
}

// Hex dump view: reads fd from its current position to EOF (or until the
// user exits/toggles), VIEWER_HEX_PER_LINE bytes per row.
static uint8_t viewer_run_hex(OricCharWin *content, OricCharWin *footer, int16_t fd, const char *togglemode)
{
    uint8_t  linebuf[VIEWER_HEX_PER_LINE];
    uint8_t  linecount = 0;
    uint16_t offset = 0;
    uint8_t  row = 0;
    int16_t  got;
    uint8_t  result = VIEWER_KEY_CONTINUE;

    while (result == VIEWER_KEY_CONTINUE)
    {
        got = loci_read(fd, viewer_chunk, VIEWER_CHUNK_SIZE);
        if (got <= 0) break;

        for (uint16_t i = 0; i < (uint16_t)got; i++)
        {
            linebuf[linecount++] = (uint8_t)viewer_chunk[i];
            if (linecount == VIEWER_HEX_PER_LINE)
            {
                viewer_print_hexline(content, row, offset, linebuf, linecount);
                offset += linecount;
                linecount = 0;
                row++;

                if (row >= content->wy)
                {
                    result = viewer_wait_key(footer, MSG_VIEWER_PRESS_KEY_FMT, togglemode);
                    if (result != VIEWER_KEY_CONTINUE) break;
                    cwin_clear(content);
                    cwin_clear(footer);
                    row = 0;
                }
            }
        }
    }

    if (result == VIEWER_KEY_CONTINUE && linecount > 0)
        viewer_print_hexline(content, row, offset, linebuf, linecount);

    if (result == VIEWER_KEY_CONTINUE)
        result = viewer_wait_key(footer, MSG_VIEWER_EOF_FMT, togglemode);

    return result;
}

void viewer_show_text(const char *path)
{
    OricCharWin content, footer;
    bool    hexmode = false;
    uint8_t result;

    int16_t fd = loci_open(path, O_RDONLY);
    if (fd < 0)
    {
        menu_fileerrormessage();
        return;
    }

    menu_popup_open(0, 0, 28);
    cwin_init(&content, 2, 0, 38, 27, A_FWBLACK, A_BGWHITE);
    cwin_init(&footer, 2, 27, 38, 1, A_FWBLACK, A_BGWHITE);

    do
    {
        cwin_clear(&content);
        cwin_clear(&footer);

        if (hexmode)
            result = viewer_run_hex(&content, &footer, fd, MSG_VIEWER_MODE_TEXT);
        else
            result = viewer_run_text(&content, &footer, fd, MSG_VIEWER_MODE_HEX);

        if (result == VIEWER_KEY_TOGGLE)
        {
            hexmode = !hexmode;
            loci_lseek(fd, 0, SEEK_SET);
        }
    } while (result == VIEWER_KEY_TOGGLE);

    loci_close(fd);
    menu_popup_close();
}
