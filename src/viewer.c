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

/**
 * Show a pause-point prompt naming the mode X would switch to, then wait for
 * a key: ESC exits, X/x toggles mode, anything else continues.
 *
 * @param footer     Footer window the prompt is printed into (row 0, col 0).
 * @param fmt        printf-style format string with one %s for togglemode.
 * @param togglemode Name of the mode pressing X would switch to (substituted
 *                    into fmt).
 * @return VIEWER_KEY_EXIT, VIEWER_KEY_TOGGLE, or VIEWER_KEY_CONTINUE
 *         depending on the key pressed.
 */
static uint8_t viewer_wait_key(OricCharWin *footer, const char *fmt, const char *togglemode)
{
    cwin_putat_printf(footer, 0, 0, fmt, togglemode);
    uint8_t key = cwin_getch();
    if (key == KEY_ESC) return VIEWER_KEY_EXIT;
    if (key == 'x' || key == 'X') return VIEWER_KEY_TOGGLE;
    return VIEWER_KEY_CONTINUE;
}

/**
 * Count the row-advances cwin_printwrap() would perform when word-wrapping
 * str into a window of the given width, starting at column 0 (always true
 * here: viewer_flush_line() is only called right after cwin_clear() or a
 * '\n', both of which leave cx==0). Mirrors cwin_printwrap()'s chunking
 * (runs of up to maxBuf=41 non-space chars, optionally with one trailing
 * space) and its column bookkeeping, including the hard-split case for
 * chunks longer than wx and a chunk that ends exactly on the right edge.
 *
 * @param str String to be word-wrapped (as cwin_printwrap() would do).
 * @param wx  Width of the destination window in columns.
 * @return    Number of rows cwin_printwrap(str) would advance.
 */
static uint8_t viewer_wrapped_rows(const char *str, uint8_t wx)
{
    uint8_t len = 0;
    while (str[len]) len++;

    const uint8_t maxBuf = 41;
    uint8_t i = 0, col = 0, rows = 0;

    while (i < len)
    {
        uint8_t chunklen = 0;
        while (i + chunklen < len && chunklen < maxBuf)
        {
            bool isspace = (str[i + chunklen] == ' ');
            chunklen++;
            if (isspace) break;
        }
        i += chunklen;

        uint8_t wordLen = chunklen;
        while (wordLen > wx)
        {
            rows++;
            wordLen -= wx;
            col = wx;
        }

        if ((uint8_t)(col + wordLen) > wx)
        {
            rows++;
            col = 0;
        }
        col += wordLen;

        if (col >= wx)
        {
            rows++;
            col = 0;
        }
    }
    return rows;
}

/**
 * Word-wrap the buffered line into content. If the line would not fit on
 * the current page, pause for a keypress and start a fresh page BEFORE
 * printing, so cwin_printwrap() never has to scroll the page mid-line. Then
 * either advance to the next line or, if the page is now exactly full, pause
 * for a keypress and start a fresh page.
 *
 * @param content    Content window the line is printed into.
 * @param footer     Footer window used for the pause-point prompt.
 * @param linelen    In/out: number of bytes buffered in viewer_line; the
 *                    buffer is NUL-terminated and *linelen reset to 0.
 * @param togglemode Name of the mode pressing X would switch to, passed
 *                    through to viewer_wait_key().
 * @return VIEWER_KEY_CONTINUE on success, or VIEWER_KEY_EXIT/VIEWER_KEY_TOGGLE
 *         if the user pressed ESC or X at a pause point.
 */
static uint8_t viewer_flush_line(OricCharWin *content, OricCharWin *footer, uint8_t *linelen, const char *togglemode)
{
    viewer_line[*linelen] = 0;
    *linelen = 0;

    if ((uint8_t)(content->cy + viewer_wrapped_rows(viewer_line, content->wx)) >= content->wy)
    {
        uint8_t result = viewer_wait_key(footer, MSG_VIEWER_PRESS_KEY_FMT, togglemode);
        if (result != VIEWER_KEY_CONTINUE) return result;
        cwin_clear(content);
        cwin_clear(footer);
    }

    cwin_printwrap(content, viewer_line);

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

/**
 * Word-wrapped text view: reads fd from its current position to EOF (or
 * until the user exits/toggles), sanitizing non-printable bytes.
 *
 * @param content    Content window text is printed into.
 * @param footer     Footer window used for pause-point/EOF prompts.
 * @param fd         Open LOCI file descriptor to read from.
 * @param togglemode Name of the mode pressing X would switch to, passed
 *                    through to viewer_flush_line()/viewer_wait_key().
 * @return VIEWER_KEY_EXIT or VIEWER_KEY_TOGGLE depending on the key pressed
 *         at the final pause/EOF prompt.
 */
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

/**
 * Render one hex-dump row: "OFFS: b0 b1 .. b7" followed by an 8-char ASCII
 * column (sanitized the same way as text mode). n < VIEWER_HEX_PER_LINE for
 * the final, partial row -- unused hex/ASCII columns are left blank.
 *
 * @param content Content window the row is printed into.
 * @param row     Row within content to print on.
 * @param offset  File offset of the first byte in bytes[], shown as a
 *                4-digit hex address.
 * @param bytes   Buffer of up to VIEWER_HEX_PER_LINE raw bytes to render.
 * @param n       Number of valid bytes in bytes[] (<= VIEWER_HEX_PER_LINE).
 * @return (none)
 */
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

/**
 * Hex dump view: reads fd from its current position to EOF (or until the
 * user exits/toggles), VIEWER_HEX_PER_LINE bytes per row.
 *
 * @param content    Content window the hex dump is printed into.
 * @param footer     Footer window used for pause-point/EOF prompts.
 * @param fd         Open LOCI file descriptor to read from.
 * @param togglemode Name of the mode pressing X would switch to, passed
 *                    through to viewer_wait_key().
 * @return VIEWER_KEY_EXIT or VIEWER_KEY_TOGGLE depending on the key pressed
 *         at the final pause/EOF prompt.
 */
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

/**
 * Open path read-only and display its contents full-screen, word-wrapped
 * (text mode) or as a hex dump (after pressing X at a pause point), one page
 * at a time; ESC exits at any pause point. Toggling mode restarts the file
 * from the beginning (loci_lseek to offset 0) in the new mode. Shows
 * menu_fileerrormessage() and returns immediately if the file cannot be
 * opened.
 *
 * @param path Drive-prefixed path of the file to display.
 * @return (none)
 */
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
    cwin_init(&content, 2, 0, 38, 27, A_FWWHITE, A_BGBLACK);
    cwin_init(&footer, 2, 27, 38, 1, A_FWCYAN, A_BGBLACK);

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
