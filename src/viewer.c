// viewer.c - Full-screen paged text file viewer for locifilemanager v2
//
// Reads a file in chunks, splits it into lines on '\n' (ignoring '\r'),
// and prints each line word-wrapped into a full-screen content window via
// cwin_printwrap(). Pagination is forward-only: once the content window's
// last row has been used, viewer_flush_line() pauses for a keypress before
// clearing the window for the next page. ESC exits at any pause point.

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

static char viewer_chunk[VIEWER_CHUNK_SIZE];
static char viewer_line[VIEWER_LINE_MAX];

// Word-wrap the buffered line into content, then either advance to the next
// line or, if the page is now full, pause for a keypress and start a fresh
// page. Returns true if ESC was pressed (caller should stop).
static bool viewer_flush_line(OricCharWin *content, OricCharWin *footer, uint8_t *linelen)
{
    viewer_line[*linelen] = 0;
    cwin_printwrap(content, viewer_line);
    *linelen = 0;

    if (content->cy >= content->wy - 1)
    {
        cwin_putat_string(footer, 0, 0, MSG_VIEWER_PRESS_KEY);
        if (cwin_getch() == KEY_ESC) return true;
        cwin_clear(content);
        cwin_clear(footer);
    }
    else
    {
        cwin_console_put_char(content, '\n');
    }
    return false;
}

void viewer_show_text(const char *path)
{
    OricCharWin content, footer;
    int16_t got;
    uint8_t linelen = 0;
    bool    aborted = false;

    int16_t fd = loci_open(path, O_RDONLY);
    if (fd < 0)
    {
        menu_fileerrormessage();
        return;
    }

    menu_popup_open(0, 0, 28);
    cwin_init(&content, 2, 0, 38, 27, A_FWBLACK, A_BGWHITE);
    cwin_clear(&content);
    cwin_init(&footer, 2, 27, 38, 1, A_FWBLACK, A_BGWHITE);
    cwin_clear(&footer);

    while (!aborted)
    {
        got = loci_read(fd, viewer_chunk, VIEWER_CHUNK_SIZE);
        if (got <= 0) break;

        for (uint16_t i = 0; i < (uint16_t)got; i++)
        {
            uint8_t ch = (uint8_t)viewer_chunk[i];

            if (ch == '\r') continue;

            if (ch == '\n')
            {
                if (viewer_flush_line(&content, &footer, &linelen)) { aborted = true; break; }
                continue;
            }

            if (linelen >= VIEWER_LINE_MAX - 1)
            {
                if (viewer_flush_line(&content, &footer, &linelen)) { aborted = true; break; }
            }
            viewer_line[linelen++] = (char)ch;
        }
    }

    if (!aborted && linelen > 0)
        viewer_flush_line(&content, &footer, &linelen);

    if (!aborted)
    {
        cwin_putat_string(&footer, 0, 0, MSG_MAIN_PRESS_CONTINUE);
        cwin_getch();
    }

    loci_close(fd);
    menu_popup_close();
}
