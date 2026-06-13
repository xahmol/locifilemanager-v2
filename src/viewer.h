// viewer.h - Full-screen paged text/hex file viewer for locifilemanager v2
//
// viewer_show_text() opens a file read-only and displays it word-wrapped
// in a full-screen popup, pausing for a keypress whenever the visible page
// fills up (forward-only paging, no scroll-back in v1). Pressing X at any
// pause point switches to a hex dump view of the same file (and back),
// restarting from the beginning in the new mode.

#ifndef VIEWER_H
#define VIEWER_H

// Open path read-only and display its contents, word-wrapped (or as a hex
// dump after pressing X), one page at a time. ESC exits at any pause point.
// Shows menu_fileerrormessage() and returns if the file cannot be opened.
void viewer_show_text(const char *path);

#pragma compile("viewer.c")

#endif  // VIEWER_H
