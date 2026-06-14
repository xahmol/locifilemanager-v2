// dir.c - Directory engine for locifilemanager v2 (two-pane browser)
// Based on v1 locifilemanager dir.c by Xander Mol, 2025 — local reference at
// locifilemanager/src/dir.c. See dir.h for adaptation notes.

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "oric.h"
#include "charwin.h"
#include "loci.h"
#include "menu.h"
#include "drive.h"
#include "recurse.h"
#include "strings.h"
#include "dir.h"

// -------------------------------------------------------------------------
// Module state (storage for dir.h externs)
// -------------------------------------------------------------------------

struct DirElement presentdirelement;
struct Directory  presentdir[2];
LociDir    *dir;
LociDirent *file;

char dir_entry_types[8][4] =
{
    "DIR",
    "DSK",
    "TAP",
    "ROM",
    "LCE",
    "   ",
    "BAS",
    "BIN"
};

struct AppSettings settings;

uint8_t  activepane;
uint16_t present;
uint16_t previous;
uint16_t next;
uint8_t  targetdrive;
uint16_t selection[2];
uint8_t  insidetape[2];
char     namefilter[32] = "";
char     favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN];

char pathbuffer[256];
char pathbuffer2[256];
char pathbuffer3[256];

// -------------------------------------------------------------------------
// Local state and helpers
// -------------------------------------------------------------------------

static OricCharWin pane[2] = {
    { 2, PANE1_YPOS, 38, 12, 0, 0, A_FWWHITE, A_BGBLACK },
    { 2, PANE2_YPOS, 38, 12, 0, 0, A_FWWHITE, A_BGBLACK },
};

static char dirbuffer[41];
static const uint8_t dirProgressBar[4] = { 48, 53, 93, 95 };

/**
 * Write the INK and PAPER attribute bytes at columns 0 and 1 of an absolute
 * screen row, directly via TEXTVRAM (see comment below for why).
 *
 * @param row   Absolute screen row (0-based).
 * @param ink   INK attribute byte written to column 0.
 * @param paper PAPER attribute byte written to column 1.
 * @return (none)
 */
// Write INK/PAPER attribute bytes at screen columns 0/1 of an absolute row.
// Duplicates charwin's private row_setattr() (not exported).
static void dir_row_attr(uint8_t row, uint8_t ink, uint8_t paper)
{
    uint8_t *p = (uint8_t *)((uint16_t)TEXTVRAM + (uint16_t)row * 40U);
    p[0] = ink;
    p[1] = paper;
}

/**
 * Case-insensitive comparison of two NUL-terminated strings.
 *
 * @param a First string.
 * @param b Second string.
 * @return <0, 0, or >0 if a is (case-insensitively) less than, equal to, or
 *         greater than b, matching strcmp() semantics.
 */
// Case-insensitive string compare (Oscar64 has no stricmp/strcasecmp).
static int16_t dir_stricmp(const char *a, const char *b)
{
    char ca, cb;
    do {
        ca = (char)tolower((unsigned char)*a++);
        cb = (char)tolower((unsigned char)*b++);
    } while (ca && ca == cb);
    return (int16_t)ca - (int16_t)cb;
}

/**
 * Case-insensitive glob match of name against pattern, supporting '*'
 * (matches any sequence, including empty) and '?' (matches exactly one
 * character). Recurses on each '*' (see comment below for why this is
 * safe).
 *
 * @param pattern NUL-terminated glob pattern.
 * @param name    NUL-terminated name to test against pattern.
 * @return 1 if name matches pattern, 0 otherwise.
 */
// Small bounded recursion only -- not directory-depth recursion (namefilter
// is <=31 chars, names <=63 chars).
static uint8_t dir_wildcard_match(const char *pattern, const char *name)
{
    while (*pattern)
    {
        if (*pattern == '*')
        {
            while (*pattern == '*') pattern++;
            if (!*pattern) return 1;

            while (*name)
            {
                if (dir_wildcard_match(pattern, name)) return 1;
                name++;
            }
            return 0;
        }
        else if (*pattern == '?')
        {
            if (!*name) return 0;
            pattern++;
            name++;
        }
        else
        {
            if (!*name || tolower((unsigned char)*pattern) != tolower((unsigned char)*name))
                return 0;
            pattern++;
            name++;
        }
    }
    return !*name;
}

/**
 * Format a directory entry as a fixed-width "%-32.32s %.3s" field: name
 * left-justified/truncated/padded to 32 chars, a space, then the 3-char
 * type string, NUL-terminated (37 bytes total including NUL). Written
 * manually because Oscar64's sprintf() does not honour %s precision (see
 * comment below).
 *
 * @param out     Destination buffer, at least 37 bytes.
 * @param name    Entry name (truncated to 32 chars if longer, space-padded
 *                  if shorter).
 * @param typestr 3-character type string (e.g. an entry of dir_entry_types[]).
 * @return (none) -- result is written to out.
 */
// Oscar64 sprintf does not honour %s precision, so this is done manually.
static void dir_format_entry(char *out, const char *name, const char *typestr)
{
    uint8_t i;
    for (i = 0; i < 32 && name[i]; i++) out[i] = name[i];
    for (; i < 32; i++) out[i] = ' ';
    out[32] = ' ';
    out[33] = typestr[0];
    out[34] = typestr[1];
    out[35] = typestr[2];
    out[36] = '\0';
}

/**
 * Format v as 4 uppercase hex digits ("%04X"), not NUL-terminated.
 *
 * @param out Destination buffer, at least 4 bytes.
 * @param v   Value to format.
 * @return (none) -- result is written to out.
 */
static void dir_hex4(char *out, uint16_t v)
{
    static const char hexdig[16] = "0123456789ABCDEF";
    out[0] = hexdig[(v >> 12) & 0x0F];
    out[1] = hexdig[(v >> 8)  & 0x0F];
    out[2] = hexdig[(v >> 4)  & 0x0F];
    out[3] = hexdig[v & 0x0F];
}

/**
 * Format v as a 6-character space-padded, right-justified decimal number
 * ("%6u"), not NUL-terminated.
 *
 * @param out Destination buffer, at least 6 bytes.
 * @param v   Value to format.
 * @return (none) -- result is written to out.
 */
static void dir_dec6(char *out, uint16_t v)
{
    char    tmp[6];
    uint8_t i = 6;
    uint8_t j;

    if (v == 0)
    {
        tmp[--i] = '0';
    }
    else
    {
        while (v)
        {
            tmp[--i] = (char)('0' + v % 10);
            v /= 10;
        }
    }
    for (j = 0; j < i; j++) out[j] = ' ';
    for (j = i; j < 6; j++) out[j] = tmp[j];
}

/**
 * Format v as a 10-character space-padded, right-justified decimal number
 * ("%10lu"), NUL-terminated. Used in place of cwin_printf(), which has no
 * uint32 support (see dir_dec6() for the 16-bit equivalent).
 *
 * @param out Destination buffer, at least 11 bytes.
 * @param v   Value to format.
 * @return (none) -- result is written to out.
 */
static void dir_dec10(char *out, uint32_t v)
{
    char    tmp[10];
    uint8_t i = 10;
    uint8_t j;

    if (v == 0)
    {
        tmp[--i] = '0';
    }
    else
    {
        while (v)
        {
            tmp[--i] = (char)('0' + v % 10);
            v /= 10;
        }
    }
    for (j = 0; j < i; j++) out[j] = ' ';
    for (j = i; j < 10; j++) out[j] = tmp[j];
    out[10] = '\0';
}

/**
 * Format a tape directory entry as "%-12.12s       $%04X %6db" (32 chars
 * plus NUL).
 *
 * @param out        Destination buffer, at least 33 bytes.
 * @param name       Tape file name (truncated to 12 chars if longer,
 *                     space-padded if shorter).
 * @param start_addr Load start address, shown as 4 hex digits.
 * @param size       File size in bytes, shown as a 6-digit decimal.
 * @return (none) -- result is written to out.
 */
static void dir_format_tape_entry(char *out, const char *name,
                                   uint16_t start_addr, uint16_t size)
{
    uint8_t i, j;

    for (i = 0; i < 12 && name[i]; i++) out[i] = name[i];
    for (; i < 12; i++) out[i] = ' ';
    for (j = 0; j < 7; j++) out[i++] = ' ';
    out[i++] = '$';
    dir_hex4(out + i, start_addr);
    i += 4;
    out[i++] = ' ';
    dir_dec6(out + i, size);
    i += 6;
    out[i++] = 'b';
    out[i] = '\0';
}

/**
 * Prepare the progress-bar row (pane row 1, the path row) before a directory
 * or tape read: writes the FWBLACK/bg attribute bytes, an A_ALT charset
 * switch at column 0, then clears the remaining 37 columns ready for
 * dir_progress_step().
 *
 * @param dirnr Pane number (0 or 1) whose progress row is prepared.
 * @return (none)
 */
static void dir_progress_init(uint8_t dirnr)
{
    uint8_t bg   = (activepane == dirnr) ? A_BGYELLOW : A_BGWHITE;
    uint8_t ypos = (dirnr ? PANE2_YPOS : PANE1_YPOS) + 1;

    dir_row_attr(ypos, A_FWBLACK, bg);
    cwin_putat_char(&pane[dirnr], 0, 1, A_ALT);
    cwin_fill_rect(&pane[dirnr], 1, 1, 37, 1, ' ');
}

/**
 * Advance the progress bar on pane dirnr's row 1 by one step, writing the
 * next animated character at *count's position; once the row is full,
 * clears it and resets *count to 0.
 *
 * @param dirnr Pane number (0 or 1) whose progress row is updated.
 * @param count In/out: progress-bar step counter, incremented each call
 *               (and reset to 0 when the bar wraps).
 * @return (none)
 */
static void dir_progress_step(uint8_t dirnr, uint8_t *count)
{
    if ((*count >> 2) > 36)
    {
        *count = 0;
        cwin_fill_rect(&pane[dirnr], 1, 1, 37, 1, ' ');
    }
    else
    {
        cwin_putat_char(&pane[dirnr], (uint8_t)(1 + (*count >> 2)), 1, dirProgressBar[*count & 3]);
        (*count)++;
    }
}

// -------------------------------------------------------------------------
// Path helpers
// -------------------------------------------------------------------------

/**
 * Concatenate dirpath and name into dest, e.g. a directory path plus an
 * entry name, truncating to fit destsize.
 *
 * @param dest     Destination buffer.
 * @param destsize Size of dest in bytes, including the NUL terminator.
 * @param dirpath  Directory path (copied first, truncated to destsize-1).
 * @param name     Entry name appended after dirpath (truncated to fit the
 *                   remaining space).
 * @return (none) -- result is written to dest.
 */
void dir_build_path(char *dest, uint16_t destsize, const char *dirpath, const char *name)
{
    strncpy(dest, dirpath, destsize);
    strncat(dest, name, destsize - strlen(dest) - 1);
}

// -------------------------------------------------------------------------
// Element storage
// -------------------------------------------------------------------------

/**
 * Load presentdirelement (meta data, then name) from XRAM at address.
 *
 * @param address XRAM address of a DirElement (meta data followed by name).
 * @return (none) -- result is written to the global presentdirelement.
 */
void dir_get_element(uint16_t address)
{
    uint16_t workaddress = address;
    xram_memcpy_from(&presentdirelement.meta, workaddress, sizeof(presentdirelement.meta));
    workaddress += sizeof(presentdirelement.meta);
    xram_memcpy_from(presentdirelement.name, workaddress, presentdirelement.meta.length);
}

/**
 * Store presentdirelement (meta data, then name) to XRAM at address.
 *
 * @param address XRAM address of a DirElement (meta data followed by name).
 * @return (none) -- writes presentdirelement to XRAM.
 */
void dir_save_element(uint16_t address)
{
    uint16_t workaddress = address;
    xram_memcpy_to(workaddress, &presentdirelement.meta, sizeof(presentdirelement.meta));
    workaddress += sizeof(presentdirelement.meta);
    xram_memcpy_to(workaddress, presentdirelement.name, presentdirelement.meta.length);
}

/**
 * Refresh the global `present` pointer and presentdirelement from the active
 * pane's current element (presentdir[activepane].present).
 *
 * @return (none) -- updates the globals `present` and presentdirelement.
 */
void dir_refresh_present(void)
{
    present = presentdir[activepane].present;
    if (present)
        dir_get_element(present);
}

// -------------------------------------------------------------------------
// Directory / tape reading
// -------------------------------------------------------------------------

/**
 * Read the directory at presentdir[dirnr].path into the XRAM linked list for
 * pane dirnr (DIR1BASE/DIR2BASE), classifying each entry by type (directory,
 * .DSK/.TAP/.ROM/.LCE image, or unknown), filtering by filterval and the
 * global namefilter, and inserting into the list in sort order if
 * settings.sort is set (or append order otherwise). Shows a progress bar on
 * pane dirnr's row 1 while reading, and refreshes the pane's
 * device/path header (dir_print_id_and_path()) when done. Stops early
 * (silently truncating the listing) if the per-pane XRAM region (DIRSIZE
 * bytes) would be exceeded.
 *
 * @param dirnr     Pane number (0 or 1) to read into.
 * @param filterval File-type filter: 0 = show all types, otherwise only
 *                    type (filterval + 1) plus directories are kept (see
 *                    dir_entry_types[]).
 * @return (none) -- populates presentdir[dirnr] and the pane's XRAM linked
 *         list; updates the global `present`/`previous`.
 */
void dir_read(uint8_t dirnr, uint8_t filterval)
{
    uint8_t  presenttype;
    uint8_t  datalength;
    uint8_t  count = 0;
    uint16_t element;
    uint16_t workaddress;
    uint8_t  inserted;
    struct DirElement bufferdir;

    dir_progress_init(dirnr);

    previous = 0;

    presentdir[dirnr].address = (dirnr) ? DIR2BASE : DIR1BASE;
    present = presentdir[dirnr].address;

    presentdir[dirnr].firstelement = 0;
    presentdir[dirnr].firstprint = 0;
    presentdir[dirnr].lastprint = 0;
    presentdir[dirnr].position = 0;
    presentdir[dirnr].present = 0;
    selection[dirnr] = 0;

    dir = loci_opendir(presentdir[dirnr].path);

    while (dir && (file = loci_readdir(dir)) != 0 && file->d_name[0] != '\0')
    {
        presenttype = 0;
        datalength = (uint8_t)strlen(file->d_name);

        dir_progress_step(dirnr, &count);

        // Check if entry is a dir by checking if bit 4 of the attribute byte is set
        if (file->d_attrib & DIR_ATTR_DIR)
        {
            presenttype = 1;

            // Add / to dir name if room is left
            if (datalength < 64)
            {
                datalength++;
                strcat(file->d_name, "/");
            }
        }

        // Check if file is a matching image type by extension
        if (!presenttype && datalength > 4)
        {
            if (file->d_name[datalength - 4] == '.')
            {
                if ((file->d_name[datalength - 3] == 'd' || file->d_name[datalength - 3] == 'D') &&
                    (file->d_name[datalength - 2] == 's' || file->d_name[datalength - 2] == 'S') &&
                    (file->d_name[datalength - 1] == 'k' || file->d_name[datalength - 1] == 'K'))
                {
                    presenttype = 2;
                }

                if ((file->d_name[datalength - 3] == 't' || file->d_name[datalength - 3] == 'T') &&
                    (file->d_name[datalength - 2] == 'a' || file->d_name[datalength - 2] == 'A') &&
                    (file->d_name[datalength - 1] == 'p' || file->d_name[datalength - 1] == 'P'))
                {
                    presenttype = 3;
                }

                if ((file->d_name[datalength - 3] == 'r' || file->d_name[datalength - 3] == 'R') &&
                    (file->d_name[datalength - 2] == 'o' || file->d_name[datalength - 2] == 'O') &&
                    (file->d_name[datalength - 1] == 'm' || file->d_name[datalength - 1] == 'M'))
                {
                    presenttype = 4;
                }

                if ((file->d_name[datalength - 3] == 'l' || file->d_name[datalength - 3] == 'L') &&
                    (file->d_name[datalength - 2] == 'c' || file->d_name[datalength - 2] == 'C') &&
                    (file->d_name[datalength - 1] == 'e' || file->d_name[datalength - 1] == 'E'))
                {
                    presenttype = 5;
                }
            }
        }

        // Set file type to unknown if not set yet
        if (!presenttype)
        {
            presenttype = 6;
        }

        // Filter out hidden files
        if (file->d_name[0] == '.')
        {
            presenttype = 0;
        }

        // Filter out non-matching file types if filter is set
        if (filterval && presenttype > 1)
        {
            if (presenttype != filterval + 1)
            {
                presenttype = 0;
            }
        }

        // Filter out non-matching file names if a name filter is set.
        // Directories (presenttype 1) are always shown so navigation isn't
        // blocked by the filter.
        if (namefilter[0] && presenttype > 1 && !dir_wildcard_match(namefilter, file->d_name))
        {
            presenttype = 0;
        }

        if (presenttype)
        {
            // Memory-full check: stop adding entries once the per-pane
            // XRAM region (DIR1BASE/DIR2BASE, each DIRSIZE bytes) would be
            // exceeded. Compare against the region's fixed end address, not
            // presentdir[dirnr].address itself (a prior version of this
            // check included presentdir[dirnr].address on both sides,
            // making it a permanent no-op).
            if (presentdir[dirnr].address + datalength + sizeof(presentdirelement.meta) > (dirnr ? DIR2BASE : DIR1BASE) + DIRSIZE - 1)
            {
                break;
            }

            // Is this the first entry?
            if (!previous)
            {
                presentdir[dirnr].firstelement = present;
                presentdir[dirnr].firstprint = present;
                presentdirelement.meta.prev = 0;
                previous = present;
                presentdirelement.meta.next = 0;
            }
            else
            {
                // Does the list need to be sorted?
                if (settings.sort)
                {
                    inserted = 0;
                    element = presentdir[dirnr].firstelement;
                    do
                    {
                        workaddress = element;
                        xram_memcpy_from(&bufferdir.meta, workaddress, sizeof(bufferdir.meta));
                        workaddress += sizeof(bufferdir.meta);
                        xram_memcpy_from(bufferdir.name, workaddress, bufferdir.meta.length);

                        if (dir_stricmp(bufferdir.name, file->d_name) > 0)
                        {
                            // Insert before the first one?
                            if (!bufferdir.meta.prev)
                            {
                                presentdirelement.meta.prev = 0;
                                presentdirelement.meta.next = element;
                                bufferdir.meta.prev = present;
                                presentdir[dirnr].firstelement = present;
                                presentdir[dirnr].firstprint = present;
                                xram_memcpy_to(element, &bufferdir.meta, sizeof(bufferdir.meta));
                            }
                            else
                            // Insert in between
                            {
                                presentdirelement.meta.prev = bufferdir.meta.prev;
                                presentdirelement.meta.next = element;
                                bufferdir.meta.prev = present;
                                xram_memcpy_to(element, &bufferdir.meta, sizeof(bufferdir.meta));

                                xram_memcpy_from(&bufferdir.meta, presentdirelement.meta.prev, sizeof(bufferdir.meta));
                                bufferdir.meta.next = present;
                                xram_memcpy_to(presentdirelement.meta.prev, &bufferdir.meta, sizeof(bufferdir.meta));
                            }
                            inserted = 1;
                            break;
                        }
                        element = bufferdir.meta.next;
                    } while (element);

                    // Insert at the end
                    if (!inserted)
                    {
                        presentdirelement.meta.prev = previous;
                        xram_memcpy_to(previous, &present, sizeof(present));
                        previous = present;
                        presentdirelement.meta.next = 0;
                    }
                }
                else
                {
                    presentdirelement.meta.prev = previous;
                    xram_memcpy_to(previous, &present, sizeof(present));
                    previous = present;
                    presentdirelement.meta.next = 0;
                }
            }

            presentdirelement.meta.select = 0;
            presentdirelement.meta.type = presenttype;
            datalength++;
            presentdirelement.meta.length = datalength;

            xram_memcpy_to(presentdir[dirnr].address, &presentdirelement.meta, sizeof(presentdirelement.meta));
            presentdir[dirnr].address += sizeof(presentdirelement.meta);

            xram_memcpy_to(presentdir[dirnr].address, file->d_name, datalength);
            presentdir[dirnr].address += datalength;

            present = presentdir[dirnr].address;
        }
    }
    if (dir)
    {
        loci_closedir(dir);
    }

    if (presentdir[dirnr].firstelement)
    {
        present = presentdir[dirnr].firstelement;
        dir_get_element(present);
    }
    else
    {
        present = 0;
    }

    dir_print_id_and_path(dirnr);
}

/**
 * Rewind the mounted tape image and parse its file headers into the XRAM
 * linked list for pane dirnr (DIR1BASE/DIR2BASE), formatting each entry via
 * dir_format_tape_entry() (type 7 = PRG/BIN, type 8 = BASIC) and inserting in
 * sort order if settings.sort is set (or append order otherwise). Shows a
 * progress bar on pane dirnr's row 1 while reading, and refreshes the pane's
 * device/path header (dir_print_id_and_path()) when done. Stops early
 * (rewinding the tape) if the per-pane XRAM region (DIRSIZE bytes) would be
 * exceeded.
 *
 * @param dirnr Pane number (0 or 1) to read into.
 * @return (none) -- populates presentdir[dirnr] and the pane's XRAM linked
 *         list; updates the global `present`/`previous`.
 */
void dir_tape_parse(uint8_t dirnr)
{
    uint8_t  presenttype;
    uint8_t  datalength;
    uint8_t  count = 0;
    uint16_t element;
    uint16_t workaddress;
    uint8_t  inserted;
    struct DirElement bufferdir;
    LociTapHdr hdr;
    int32_t  counter;
    uint16_t start_addr, end_addr, size;
    uint8_t  hi, lo;

    dir_progress_init(dirnr);

    previous = 0;

    presentdir[dirnr].address = (dirnr) ? DIR2BASE : DIR1BASE;
    present = presentdir[dirnr].address;

    presentdir[dirnr].firstelement = 0;
    presentdir[dirnr].firstprint = 0;
    presentdir[dirnr].lastprint = 0;
    presentdir[dirnr].position = 0;
    presentdir[dirnr].present = 0;
    selection[dirnr] = 0;

    // Rewind tape
    TAP.cmd = TAP_CMD_REW;

    while ((counter = tap_read_header(&hdr)) != -1)
    {
        // Get addresses and size
        hi = hdr.start_addr_hi;
        lo = hdr.start_addr_lo;
        start_addr = ((uint16_t)hi << 8) | lo;

        hi = hdr.end_addr_hi;
        lo = hdr.end_addr_lo;
        end_addr = ((uint16_t)hi << 8) | lo;

        if (start_addr > end_addr)
        {
            // Bad/unsupported header
            size = 0;
        }
        else
        {
            size = end_addr - start_addr;
        }

        // Misuse first four bytes of dirbuffer for the tape counter
        memcpy(dirbuffer, &counter, sizeof(counter));

        // Set name, type, start address and size
        dir_format_tape_entry(dirbuffer + 4,
                               hdr.filename[0] ? (const char *)hdr.filename : "<no name>",
                               start_addr, size);

        // Name length: formatted string plus 4 for counter, plus 1 trailing zero
        datalength = (uint8_t)strlen(dirbuffer + 4) + 5;

        dir_progress_step(dirnr, &count);

        // Set data type
        presenttype = (hdr.type == 0x80) ? 8 : 7;

        // Seek over file on tape
        counter += (int32_t)sizeof(LociTapHdr) + 4 + size;
        tap_seek(counter);

        // Memory-full check: stop adding entries once the per-pane XRAM
        // region (DIR1BASE/DIR2BASE, each DIRSIZE bytes) would be exceeded.
        // See the analogous check in dir_read() for why the comparison is
        // against the region's fixed end address, not presentdir[dirnr].address.
        if (presentdir[dirnr].address + datalength + sizeof(presentdirelement.meta) > (dirnr ? DIR2BASE : DIR1BASE) + DIRSIZE - 1)
        {
            TAP.cmd = TAP_CMD_REW;
            break;
        }

        // Is this the first entry?
        if (!previous)
        {
            presentdir[dirnr].firstelement = present;
            presentdir[dirnr].firstprint = present;
            presentdirelement.meta.prev = 0;
            previous = present;
            presentdirelement.meta.next = 0;
        }
        else
        {
            // Does the list need to be sorted?
            if (settings.sort)
            {
                inserted = 0;
                element = presentdir[dirnr].firstelement;
                do
                {
                    workaddress = element;
                    xram_memcpy_from(&bufferdir.meta, workaddress, sizeof(bufferdir.meta));
                    workaddress += sizeof(bufferdir.meta);
                    xram_memcpy_from(bufferdir.name, workaddress, bufferdir.meta.length);

                    if (dir_stricmp(bufferdir.name + 4, dirbuffer + 4) > 0)
                    {
                        if (!bufferdir.meta.prev)
                        {
                            presentdirelement.meta.prev = 0;
                            presentdirelement.meta.next = element;
                            bufferdir.meta.prev = present;
                            presentdir[dirnr].firstelement = present;
                            presentdir[dirnr].firstprint = present;
                            xram_memcpy_to(element, &bufferdir.meta, sizeof(bufferdir.meta));
                        }
                        else
                        {
                            presentdirelement.meta.prev = bufferdir.meta.prev;
                            presentdirelement.meta.next = element;
                            bufferdir.meta.prev = present;
                            xram_memcpy_to(element, &bufferdir.meta, sizeof(bufferdir.meta));

                            xram_memcpy_from(&bufferdir.meta, presentdirelement.meta.prev, sizeof(bufferdir.meta));
                            bufferdir.meta.next = present;
                            xram_memcpy_to(presentdirelement.meta.prev, &bufferdir.meta, sizeof(bufferdir.meta));
                        }
                        inserted = 1;
                        break;
                    }
                    element = bufferdir.meta.next;
                } while (element);

                if (!inserted)
                {
                    presentdirelement.meta.prev = previous;
                    xram_memcpy_to(previous, &present, sizeof(present));
                    previous = present;
                    presentdirelement.meta.next = 0;
                }
            }
            else
            {
                presentdirelement.meta.prev = previous;
                xram_memcpy_to(previous, &present, sizeof(present));
                previous = present;
                presentdirelement.meta.next = 0;
            }
        }

        presentdirelement.meta.select = 0;
        presentdirelement.meta.type = presenttype;
        datalength++;
        presentdirelement.meta.length = datalength;

        xram_memcpy_to(presentdir[dirnr].address, &presentdirelement.meta, sizeof(presentdirelement.meta));
        presentdir[dirnr].address += sizeof(presentdirelement.meta);

        xram_memcpy_to(presentdir[dirnr].address, dirbuffer, datalength);
        presentdir[dirnr].address += datalength;

        present = presentdir[dirnr].address;
    }

    if (presentdir[dirnr].firstelement)
    {
        present = presentdir[dirnr].firstelement;
        dir_get_element(present);
    }
    else
    {
        present = 0;
    }

    dir_print_id_and_path(dirnr);
}

// -------------------------------------------------------------------------
// Pane drawing
// -------------------------------------------------------------------------

/**
 * Redraw pane dirnr's two header rows: row 0 shows the device label (or, if
 * insidetape[dirnr], MSG_DIR_TAPE_PREFIX plus the mounted tape's filename),
 * and row 1 shows the current path (drive-number prefix in its own 3-char
 * field, then the rest of the path).
 *
 * @param dirnr Pane number (0 or 1) to redraw.
 * @return (none)
 */
void dir_print_id_and_path(uint8_t dirnr)
{
    uint8_t bg     = (activepane == dirnr) ? A_BGYELLOW : A_BGWHITE;
    uint8_t absrow = (dirnr ? PANE2_YPOS : PANE1_YPOS);
    char    namebuf[40];

    // Device or tape name on the first pane row
    dir_row_attr(absrow, A_FWBLACK, bg);
    cwin_fill_rect(&pane[dirnr], 0, 0, 38, 1, ' ');

    if (insidetape[dirnr])
    {
        sprintf(namebuf, MSG_DIR_TAPE_PREFIX "%s", mount_filename[4]);
        cwin_putat_string(&pane[dirnr], 0, 0, namebuf);
    }
    else
    {
        cwin_putat_string(&pane[dirnr], 0, 0, get_loci_devname(presentdir[dirnr].drive, 38));
    }

    // Path on the second pane row: drive id (3 chars) then rest of path
    dir_row_attr(absrow + 1, A_FWBLACK, bg);
    cwin_fill_rect(&pane[dirnr], 0, 1, 38, 1, ' ');
    cwin_putat_chars(&pane[dirnr], 0, 1, presentdir[dirnr].path, 3);
    cwin_putat_string(&pane[dirnr], 3, 1, presentdir[dirnr].path + 3);
}

/**
 * Redraw a single directory-listing row of pane dirnr at printpos (0-based,
 * below the two header rows): highlights it if it's both the active pane and
 * the cursor position, shows a '-' selection indicator in column 0 if
 * presentdirelement.meta.select is set, and formats the (possibly
 * tape-offset) name and type via dir_format_entry().
 *
 * @param dirnr   Pane number (0 or 1) being redrawn.
 * @param printpos Row index within the pane's listing area (0-based).
 * @return (none) -- assumes presentdirelement already holds the entry to
 *         print (see dir_get_element()).
 */
void dir_print_entry(uint8_t dirnr, uint8_t printpos)
{
    uint8_t fg, bg, offs, absrow;
    char    linebuf[37];

    if (activepane == dirnr)
    {
        fg = (printpos == presentdir[dirnr].position) ? A_FWBLACK : A_FWYELLOW;
        bg = (printpos == presentdir[dirnr].position) ? A_BGCYAN  : A_BGBLACK;
    }
    else
    {
        fg = A_FWWHITE;
        bg = A_BGBLACK;
    }

    absrow = (dirnr ? PANE2_YPOS : PANE1_YPOS) + printpos + 2;
    dir_row_attr(absrow, fg, bg);
    cwin_fill_rect(&pane[dirnr], 0, printpos + 2, 38, 1, ' ');

    // Print '-' indicator if selected
    cwin_putat_char(&pane[dirnr], 0, printpos + 2, presentdirelement.meta.select ? '-' : ' ');

    // Set offset if in tape, as the first four bytes are the tape counter
    offs = insidetape[dirnr] ? 4 : 0;

    dir_format_entry(linebuf, presentdirelement.name + offs,
                      dir_entry_types[presentdirelement.meta.type - 1]);
    cwin_putat_string(&pane[dirnr], 1, printpos + 2, linebuf);
}

/**
 * Fully redraw pane dirnr: clears the pane, redraws the header
 * (dir_print_id_and_path()), optionally re-reads the directory or tape
 * (dir_read()/dir_tape_parse()) if readdir is set, then prints either
 * MSG_DIR_EMPTY (if the listing is empty) or up to PANE_HEIGHT entries
 * starting from presentdir[dirnr].firstprint, tracking
 * presentdir[dirnr].present/lastprint and the global `present` as it goes.
 *
 * @param dirnr   Pane number (0 or 1) to redraw.
 * @param readdir Non-zero to re-read the directory/tape before drawing;
 *                  zero to redraw the existing in-memory listing.
 * @return (none)
 */
void dir_draw(uint8_t dirnr, uint8_t readdir)
{
    uint8_t  ypos = (dirnr) ? PANE2_YPOS : PANE1_YPOS;
    uint8_t  printpos = 0;
    uint16_t element;

    cwin_clear(&pane[dirnr]);

    dir_print_id_and_path(dirnr);

    if (readdir)
    {
        if (insidetape[dirnr])
        {
            dir_tape_parse(dirnr);
        }
        else
        {
            dir_read(dirnr, settings.filter);
        }

        presentdir[dirnr].present = presentdir[dirnr].firstprint;
    }

    // Print message if no valid entries in dir are found
    if (!presentdir[dirnr].firstprint)
    {
        dir_row_attr(ypos + 2, A_FWRED, A_BGBLACK);
        cwin_putat_string(&pane[dirnr], 0, 2, MSG_DIR_EMPTY);
    }
    else
    {
        element = presentdir[dirnr].firstprint;

        // Loop while area is not full and further direntries are still present
        do
        {
            dir_get_element(element);
            dir_print_entry(dirnr, printpos);
            presentdir[dirnr].lastprint = element;
            if (printpos == presentdir[dirnr].position)
            {
                presentdir[dirnr].present = element;
            }
            printpos++;

            if (!presentdirelement.meta.next)
            {
                break;
            }
            else
            {
                element = presentdirelement.meta.next;
            }
        } while (printpos < PANE_HEIGHT);
        present = presentdir[dirnr].present;
    }
}

// -------------------------------------------------------------------------
// Drive switching
// -------------------------------------------------------------------------

/**
 * Switch pane dirnr to the next valid LOCI device (wrapping past MAXDEV back
 * to 0, per locicfg.validdev[]), reset its path to that drive's root ("N:/"),
 * and redraw it (re-reading the directory). Persists the new drive/path via
 * config_save().
 *
 * @param dirnr Pane number (0 or 1) to switch.
 * @return (none)
 */
void dir_get_next_drive(uint8_t dirnr)
{
    uint8_t drive = presentdir[dirnr].drive;
    uint8_t valid = 0;

    do
    {
        drive++;
        if (drive > MAXDEV)
        {
            drive = 0;
        }
        if (locicfg.validdev[drive])
        {
            valid = 1;
        }
    } while (!valid);

    presentdir[dirnr].drive = drive;

    sprintf(dirbuffer, "%u:/", drive);
    strncpy(presentdir[dirnr].path, dirbuffer, 4);

    dir_draw(dirnr, 1);
    config_save();
}

/**
 * Switch pane dirnr to the previous valid LOCI device (wrapping below 0 to
 * MAXDEV, per locicfg.validdev[]), reset its path to that drive's root
 * ("N:/"), and redraw it (re-reading the directory). Persists the new
 * drive/path via config_save().
 *
 * @param dirnr Pane number (0 or 1) to switch.
 * @return (none)
 */
void dir_get_prev_drive(uint8_t dirnr)
{
    uint8_t drive = presentdir[dirnr].drive;
    uint8_t valid = 0;

    do
    {
        if (drive > 0)
        {
            drive--;
        }
        else
        {
            drive = MAXDEV;
        }

        if (locicfg.validdev[drive])
        {
            valid = 1;
        }
    } while (!valid);

    presentdir[dirnr].drive = drive;

    sprintf(dirbuffer, "%u:/", drive);
    strncpy(presentdir[dirnr].path, dirbuffer, 4);

    dir_draw(dirnr, 1);
    config_save();
}

// -------------------------------------------------------------------------
// Pane / cursor navigation
// -------------------------------------------------------------------------

/**
 * Toggle the active pane (0 <-> 1) and redraw both panes (without re-reading
 * their directories) so the highlight moves to the newly active pane.
 * Persists the new active pane via config_save().
 *
 * @return (none)
 */
void dir_switch_pane(void)
{
    activepane = !activepane;
    dir_draw(0, 0);
    dir_draw(1, 0);
    config_save();
}

/**
 * Move the cursor down one entry in the active pane, scrolling the listing
 * (re-reading the page via dir_draw()) if the cursor would move past the
 * last visible row, or otherwise redrawing just the affected two rows. No-op
 * if the listing is empty or the current entry has no next element.
 *
 * @return (none)
 */
void dir_go_down(void)
{
    if (presentdir[activepane].firstelement && presentdirelement.meta.next)
    {
        present = presentdirelement.meta.next;
        dir_get_element(present);
        presentdir[activepane].present = present;
        presentdir[activepane].position++;

        if (presentdir[activepane].position > PANE_HEIGHT - 1)
        {
            presentdir[activepane].position = 0;
            presentdir[activepane].firstprint = present;
            dir_draw(activepane, 0);
        }
        else
        {
            previous = presentdirelement.meta.prev;
            dir_get_element(previous);
            dir_print_entry(activepane, presentdir[activepane].position - 1);
            dir_get_element(present);
            dir_print_entry(activepane, presentdir[activepane].position);
        }
    }
}

/**
 * Move the cursor up one entry in the active pane, scrolling the listing
 * (recomputing firstprint and calling dir_draw()) if the cursor is already
 * on the first visible row, or otherwise redrawing just the affected two
 * rows. No-op if the listing is empty or the current entry has no previous
 * element.
 *
 * @return (none)
 */
void dir_go_up(void)
{
    uint8_t count;

    if (presentdir[activepane].firstelement && presentdirelement.meta.prev)
    {
        present = presentdirelement.meta.prev;
        dir_get_element(present);
        presentdir[activepane].present = present;

        if (!presentdir[activepane].position)
        {
            presentdir[activepane].position = PANE_HEIGHT - 1;
            for (count = 0; count < PANE_HEIGHT - 1; count++)
            {
                if (!presentdirelement.meta.prev)
                {
                    break;
                }
                present = presentdirelement.meta.prev;
                dir_get_element(present);
            }
            presentdir[activepane].firstprint = present;
            dir_draw(activepane, 0);
        }
        else
        {
            presentdir[activepane].position--;
            next = presentdirelement.meta.next;
            dir_get_element(next);
            dir_print_entry(activepane, presentdir[activepane].position + 1);
            dir_get_element(present);
            dir_print_entry(activepane, presentdir[activepane].position);
        }
    }
}

/**
 * Move the cursor to the last entry of the currently displayed page
 * (starting from presentdir[activepane].firstprint and advancing up to
 * PANE_HEIGHT-1 times), redrawing the old and new cursor rows. Used by
 * dir_bottom() and dir_pagedown() when the listing ends partway through a
 * page.
 *
 * @return (none) -- no-op if the active pane's listing is empty.
 */
void dir_last_of_page(void)
{
    uint16_t element;
    uint8_t  count;
    uint8_t  position = 0;
    uint8_t  oldpos = presentdir[activepane].position;

    if (presentdir[activepane].firstelement)
    {
        element = presentdir[activepane].firstprint;
        dir_get_element(element);
        for (count = 0; count < PANE_HEIGHT - 1; count++)
        {
            if (!presentdirelement.meta.next)
            {
                break;
            }
            position++;
            element = presentdirelement.meta.next;
            dir_get_element(element);
        }

        presentdir[activepane].position = position;
        dir_get_element(present);
        dir_print_entry(activepane, oldpos);
        presentdir[activepane].present = element;
        present = element;
        dir_get_element(present);
        dir_print_entry(activepane, position);
    }
}

/**
 * Scroll the active pane forward by one page: advances up to PANE_HEIGHT
 * elements past the current `present` element to become the new
 * firstprint/present (cursor at position 0), then redraws via dir_draw(). If
 * the current last-printed element has no next element (already on the last
 * page), instead calls dir_last_of_page() to move the cursor to the end of
 * the current page.
 *
 * @return (none) -- no-op if the active pane's listing is empty.
 */
void dir_pagedown(void)
{
    uint16_t element;
    uint8_t  count;

    if (presentdir[activepane].firstelement)
    {
        element = presentdir[activepane].lastprint;
        dir_get_element(element);

        if (presentdirelement.meta.next)
        {
            dir_get_element(present);
            for (count = 0; count < PANE_HEIGHT; count++)
            {
                if (!presentdirelement.meta.next)
                {
                    break;
                }
                present = presentdirelement.meta.next;
                dir_get_element(present);
            }

            presentdir[activepane].firstprint = present;
            presentdir[activepane].present = present;
            presentdir[activepane].position = 0;

            dir_draw(activepane, 0);
        }
        else
        {
            dir_last_of_page();
        }
    }
}

/**
 * Scroll the active pane back by one page: retreats up to PANE_HEIGHT
 * elements before the current `present` element to become the new
 * firstprint/present (cursor at position 0), then redraws via dir_draw().
 *
 * @return (none) -- no-op if the active pane's listing is empty or already
 *         at the first element.
 */
void dir_pageup(void)
{
    uint8_t count;

    if (presentdir[activepane].firstelement && presentdirelement.meta.prev)
    {
        for (count = 0; count < PANE_HEIGHT; count++)
        {
            if (!presentdirelement.meta.prev)
            {
                break;
            }
            present = presentdirelement.meta.prev;
            dir_get_element(present);
        }

        presentdir[activepane].firstprint = present;
        presentdir[activepane].present = present;
        presentdir[activepane].position = 0;

        dir_draw(activepane, 0);
    }
}

/**
 * Move the active pane's cursor to the first entry of the listing
 * (firstelement), redrawing the pane.
 *
 * @return (none) -- no-op if the active pane's listing is empty.
 */
void dir_top(void)
{
    if (presentdir[activepane].firstelement)
    {
        present = presentdir[activepane].firstelement;
        dir_get_element(present);
        presentdir[activepane].present = present;
        presentdir[activepane].position = 0;
        presentdir[activepane].firstprint = present;
        dir_draw(activepane, 0);
    }
}

/**
 * Move the active pane's cursor to the last entry of the listing: if the
 * current page already shows the last entry, delegates to
 * dir_last_of_page(); otherwise walks to the last element, computes a new
 * firstprint that places it at the bottom of a full page (PANE_HEIGHT-1
 * elements back), and redraws via dir_draw().
 *
 * @return (none) -- no-op if the active pane's listing is empty or the
 *         current entry has no next element.
 */
void dir_bottom(void)
{
    uint8_t count;

    if (presentdir[activepane].firstelement && presentdirelement.meta.next)
    {
        present = presentdir[activepane].lastprint;
        dir_get_element(present);
        if (!presentdirelement.meta.next)
        {
            dir_last_of_page();
            return;
        }

        present = presentdir[activepane].firstelement;
        do
        {
            dir_get_element(present);
            if (presentdirelement.meta.next)
            {
                present = presentdirelement.meta.next;
            }
            else
            {
                break;
            }
        } while (1);

        presentdir[activepane].present = present;
        presentdir[activepane].position = PANE_HEIGHT - 1;
        for (count = 0; count < PANE_HEIGHT - 1; count++)
        {
            present = presentdirelement.meta.prev;
            dir_get_element(present);
        }
        presentdir[activepane].firstprint = present;
        present = presentdir[activepane].lastprint;

        dir_draw(activepane, 0);
    }
}

// -------------------------------------------------------------------------
// Selection
// -------------------------------------------------------------------------

/**
 * Toggle the selection mark on the entry under the cursor in the active
 * pane, persist it (dir_save_element()), redraw that row, and update
 * selection[activepane]. No-op for directories, tape entries, or an empty
 * listing.
 *
 * @return (none)
 */
void dir_select_toggle(void)
{
    if (presentdir[activepane].firstelement && !insidetape[activepane] && presentdirelement.meta.type > 1)
    {
        presentdirelement.meta.select = !presentdirelement.meta.select;
        dir_save_element(present);
        dir_print_entry(activepane, presentdir[activepane].position);
        if (presentdirelement.meta.select)
        {
            selection[activepane]++;
        }
        else
        {
            selection[activepane]--;
        }
    }
}

/**
 * Set the selection mark on every non-directory entry in the active pane's
 * listing to select, persist each change (dir_save_element()), recompute
 * selection[activepane], and redraw the pane.
 *
 * @param select New selection state to apply (0 = deselect, non-zero =
 *                 select) to every selectable entry.
 * @return (none) -- no-op if the active pane's listing is empty or it is a
 *         tape listing.
 */
void dir_select_all(uint8_t select)
{
    uint16_t element;

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        element = presentdir[activepane].firstelement;
        selection[activepane] = 0;
        do
        {
            dir_get_element(element);
            if (presentdirelement.meta.type > 1)
            {
                presentdirelement.meta.select = select;
                selection[activepane]++;
                dir_save_element(element);
            }
            element = presentdirelement.meta.next;
        } while (element);
        dir_draw(activepane, 0);
    }
}

/**
 * Invert the selection mark on every non-directory entry in the active
 * pane's listing, persist each change (dir_save_element()), recompute
 * selection[activepane], and redraw the pane.
 *
 * @return (none) -- no-op if the active pane's listing is empty or it is a
 *         tape listing.
 */
void dir_select_inverse(void)
{
    uint16_t element;

    if (presentdir[activepane].firstelement && !insidetape[activepane])
    {
        element = presentdir[activepane].firstelement;
        selection[activepane] = 0;
        do
        {
            dir_get_element(element);
            if (presentdirelement.meta.type > 1)
            {
                presentdirelement.meta.select = !presentdirelement.meta.select;
                if (presentdirelement.meta.select)
                {
                    selection[activepane]++;
                }
                dir_save_element(element);
            }
            element = presentdirelement.meta.next;
        } while (element);
        dir_draw(activepane, 0);
    }
}

// -------------------------------------------------------------------------
// Path navigation
// -------------------------------------------------------------------------

/**
 * Reset the active pane's path to its drive's root ("N:/"), clear
 * insidetape[activepane], redraw the pane (re-reading the directory), and
 * persist the new path via config_save().
 *
 * @return (none)
 */
void dir_gotoroot(void)
{
    sprintf(dirbuffer, "%u:/", presentdir[activepane].drive);
    strncpy(presentdir[activepane].path, dirbuffer, 4);

    insidetape[activepane] = 0;

    dir_draw(activepane, 1);
    config_save();
}

/**
 * Navigate the active pane up one level: if currently inside a tape
 * container, leaves the tape view; otherwise truncates the path after the
 * second-to-last '/' (no-op at the drive root, path length <= 3). Redraws the
 * pane (re-reading the directory) and persists the new path via
 * config_save().
 *
 * @return (none)
 */
void dir_parentdir(void)
{
    uint8_t length;
    uint8_t i;

    // First check if inside tape; if yes go out of tape and return
    if (insidetape[activepane])
    {
        insidetape[activepane] = 0;
        dir_draw(activepane, 1);
        config_save();
        return;
    }

    length = (uint8_t)strlen(presentdir[activepane].path);
    if (length > 3)
    {
        // Find last /
        for (i = length - 2; i > 2; i--)
        {
            if (presentdir[activepane].path[i] == '/')
            {
                break;
            }
        }

        presentdir[activepane].path[i + 1] = 0;

        dir_draw(activepane, 1);
        config_save();
    }
}

// -------------------------------------------------------------------------
// Sort toggle
// -------------------------------------------------------------------------

/**
 * Toggle settings.sort, redraw both panes (re-reading their directories so
 * the new sort order takes effect), update the "Sort: On/Off" pulldown menu
 * label, and persist the setting via config_save().
 *
 * @return (none)
 */
void dir_togglesort(void)
{
    settings.sort = !settings.sort;
    dir_draw(0, 1);
    dir_draw(1, 1);

    sprintf(pulldown_titles[0][3], MSG_MENU_APP_SORT_FMT,
            settings.sort ? MSG_MENU_VAL_ON : MSG_MENU_VAL_OFF);
    config_save();
}

// -------------------------------------------------------------------------
// Persistent settings (FMCONFIG_PATH)
// -------------------------------------------------------------------------

/**
 * Persist settings (confirm/filter/enterchoice/sort), favourites, and each
 * pane's last path/drive plus the active pane to FMCONFIG_PATH, creating
 * FMCONFIG_DIR1/FMCONFIG_DIR2 first if needed.
 *
 * @return (none) -- writes FMCONFIG_PATH.
 */
void config_save(void)
{
    // static: struct FmConfig is too large for the 512-byte stack segment
    // (#pragma stacksize(0x0200), include/oric_crt.c) alongside its callers.
    static struct FmConfig cfg;
    uint8_t i;

    // Create the parent directories if they don't exist yet. Errors
    // (including "already exists") are ignored -- file_save() below fails
    // harmlessly if a directory is genuinely unavailable.
    loci_mkdir(FMCONFIG_DIR1);
    loci_mkdir(FMCONFIG_DIR2);

    cfg.magic       = FMCONFIG_MAGIC;
    cfg.confirm     = settings.confirm;
    cfg.filter      = settings.filter;
    cfg.enterchoice = settings.enterchoice;
    cfg.sort        = settings.sort;

    for (i = 0; i < FMCONFIG_FAV_SLOTS; i++)
        strncpy(cfg.favourites[i], favourites[i], FMCONFIG_FAV_PATHLEN);

    for (i = 0; i < 2; i++)
    {
        strncpy(cfg.lastpath[i], presentdir[i].path, FMCONFIG_FAV_PATHLEN - 1);
        cfg.lastpath[i][FMCONFIG_FAV_PATHLEN - 1] = '\0';
        cfg.lastdrive[i] = presentdir[i].drive;
    }
    cfg.lastactivepane = activepane;

    file_save(FMCONFIG_PATH, &cfg, sizeof(cfg));
}

/**
 * Load settings (confirm/filter/enterchoice/sort), favourites, and each
 * pane's last path/drive plus the active pane from FMCONFIG_PATH, if present
 * and valid (correct size and FMCONFIG_MAGIC). On any failure (missing file,
 * short read, bad magic), instead calls config_save() to write out the
 * compiled-in defaults already set by the caller as a new config file.
 *
 * @return (none) -- updates settings, favourites, presentdir[]/activepane,
 *         or writes FMCONFIG_PATH via config_save().
 */
void config_load(void)
{
    // static: see config_save() above.
    static struct FmConfig cfg;
    uint8_t i;

    if (file_load(FMCONFIG_PATH, &cfg, sizeof(cfg)) != sizeof(cfg))
    {
        config_save();
        return;
    }

    if (cfg.magic != FMCONFIG_MAGIC)
    {
        config_save();
        return;
    }

    settings.confirm     = cfg.confirm;
    settings.filter      = cfg.filter;
    settings.enterchoice = cfg.enterchoice;
    settings.sort        = cfg.sort;

    for (i = 0; i < FMCONFIG_FAV_SLOTS; i++)
    {
        strncpy(favourites[i], cfg.favourites[i], FMCONFIG_FAV_PATHLEN);
        favourites[i][FMCONFIG_FAV_PATHLEN - 1] = '\0';
    }

    for (i = 0; i < 2; i++)
    {
        strncpy(presentdir[i].path, cfg.lastpath[i], FMCONFIG_FAV_PATHLEN);
        presentdir[i].path[FMCONFIG_FAV_PATHLEN - 1] = '\0';
        presentdir[i].drive = cfg.lastdrive[i];
    }
    activepane = (cfg.lastactivepane == 1) ? 1 : 0;
}

// -------------------------------------------------------------------------
// Favourite directories (FMCONFIG_PATH)
// -------------------------------------------------------------------------

/**
 * Bookmark the active pane's current path into favourites[slot] and persist
 * via config_save().
 *
 * @param slot 0-based favourite slot index (0..FMCONFIG_FAV_SLOTS-1).
 * @return (none)
 */
void favourites_add(uint8_t slot)
{
    strncpy(favourites[slot], presentdir[activepane].path, FMCONFIG_FAV_PATHLEN - 1);
    favourites[slot][FMCONFIG_FAV_PATHLEN - 1] = '\0';
    config_save();
}

/**
 * Clear favourites[slot] and persist via config_save().
 *
 * @param slot 0-based favourite slot index (0..FMCONFIG_FAV_SLOTS-1).
 * @return (none)
 */
void favourites_delete(uint8_t slot)
{
    favourites[slot][0] = '\0';
    config_save();
}

/**
 * Jump the active pane to the path bookmarked in favourites[slot]: copies
 * the path, derives the drive number from its leading digit, clears
 * insidetape[activepane], and redraws the pane (re-reading the directory).
 *
 * @param slot 0-based favourite slot index (0..FMCONFIG_FAV_SLOTS-1).
 * @return (none) -- no-op if favourites[slot] is empty.
 */
void favourites_goto(uint8_t slot)
{
    if (!favourites[slot][0])
        return;

    strncpy(presentdir[activepane].path, favourites[slot], sizeof(presentdir[activepane].path) - 1);
    presentdir[activepane].path[sizeof(presentdir[activepane].path) - 1] = '\0';
    presentdir[activepane].drive = (uint8_t)(favourites[slot][0] - '0');
    insidetape[activepane] = 0;

    dir_draw(activepane, 1);
}

/**
 * Show a popup listing all FMCONFIG_FAV_SLOTS favourite slots (MSG_FAV_EMPTY
 * for unused ones). Digit keys 1..FMCONFIG_FAV_SLOTS jump the active pane to
 * a populated slot (closing the popup first) via favourites_goto(); [A]
 * prompts for a slot and bookmarks the active pane's current path into it
 * via favourites_add(); [D] prompts for a slot and clears it via
 * favourites_delete(); ESC closes the popup without navigating.
 *
 * @return (none)
 */
void favourites_show(void)
{
    OricCharWin popup;
    uint8_t key;
    uint8_t i;

    menu_popup_open(0, 6, 13);
    cwin_init(&popup, 2, 6, 38, 13, A_FWBLACK, A_BGWHITE);

    cwin_putat_string(&popup, 0, 0, MSG_FAV_TITLE);

    for (;;)
    {
        for (i = 0; i < FMCONFIG_FAV_SLOTS; i++)
        {
            cwin_fill_rect(&popup, 0, 2 + i, 36, 1, CH_SPACE);
            cwin_putat_printf(&popup, 0, 2 + i, MSG_FAV_SLOT_FMT, (uint16_t)(i + 1),
                              favourites[i][0] ? favourites[i] : MSG_FAV_EMPTY);
        }

        cwin_fill_rect(&popup, 0, 11, 36, 1, CH_SPACE);
        cwin_putat_string(&popup, 0, 11, MSG_FAV_HELP);

        key = cwin_getch();

        if (key == KEY_ESC)
            break;

        if (key >= '1' && key <= '0' + FMCONFIG_FAV_SLOTS)
        {
            i = key - '1';
            if (favourites[i][0])
            {
                // Close the popup before redrawing the panes -- see
                // select_namefilter() for why (menu_popup_close() restores
                // the pre-popup screen content for the rows it covers).
                menu_popup_close();
                favourites_goto(i);
                return;
            }
            continue;
        }

        if (key == 'a' || key == 'A')
        {
            cwin_fill_rect(&popup, 0, 11, 36, 1, CH_SPACE);
            cwin_putat_string(&popup, 0, 11, MSG_FAV_ADD_PROMPT);
            key = cwin_getch();
            if (key >= '1' && key <= '0' + FMCONFIG_FAV_SLOTS)
                favourites_add(key - '1');
            continue;
        }

        if (key == 'd' || key == 'D')
        {
            cwin_fill_rect(&popup, 0, 11, 36, 1, CH_SPACE);
            cwin_putat_string(&popup, 0, 11, MSG_FAV_DELETE_PROMPT);
            key = cwin_getch();
            if (key >= '1' && key <= '0' + FMCONFIG_FAV_SLOTS)
                favourites_delete(key - '1');
            continue;
        }
    }

    menu_popup_close();
}

// -------------------------------------------------------------------------
// Directory creation / deletion
// -------------------------------------------------------------------------

/**
 * Prompt for a new subdirectory name in the active pane's current path and
 * create it via loci_mkdir(), then redraw the pane to show the new entry.
 * Shows menu_fileerrormessage() if creation or the post-creation
 * loci_opendir() check fails. No-op (after the input prompt) if the user
 * cancels/enters an empty name, or if the active pane is a tape listing.
 *
 * @return (none)
 */
void dir_newdir(void)
{
    char input[64] = "";
    OricCharWin popup;

    // Note: deliberately NOT gated on presentdir[activepane].firstelement --
    // an empty directory (firstprint == 0, "Directory empty" shown) is still
    // a valid place to create a subdirectory. Requiring an existing entry
    // made it impossible to create a subdir inside a freshly-created (and
    // therefore empty) one.
    if (!insidetape[activepane])
    {
        menu_popup_open(0, 8, 15);
        cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);

        cwin_putat_string(&popup, 0, 1, MSG_DIR_CREATE_TITLE);
        cwin_putat_string(&popup, 0, 3, MSG_DIR_ENTER_NAME);

        // Input new name and check if not cancelled or empty string
        if (cwin_textinput(&popup, 0, 4, 35, input, sizeof(input) - 1, VINPUT_ALL) > 0)
        {
            dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, input);

            if (loci_mkdir(pathbuffer) < 0)
            {
                menu_fileerrormessage();
                menu_popup_close();
            }
            else
            {
                // loci_mkdir() is synchronous (mia_call_int blocks until the
                // firmware operation completes), so the new directory is
                // immediately visible -- a single loci_opendir() suffices.
                // A previous retry loop called loci_opendir() repeatedly on
                // failure; on real LOCI hardware each failed opendir appears
                // to consume a firmware directory-handle slot that is never
                // released (loci_closedir() can't be called without a valid
                // fd), so retrying exhausted the firmware's handle pool and
                // surfaced as "error #5" (EMFILE) on later, unrelated file
                // operations -- e.g. copying into a freshly-created
                // multi-level subdirectory tree.
                dir = loci_opendir(pathbuffer);

                if (!dir)
                {
                    menu_fileerrormessage();
                    menu_popup_close();
                }
                else
                {
                    loci_closedir(dir);

                    // Close the popup before redrawing the panes -- see the
                    // analogous comment in select_namefilter(): closing
                    // AFTER dir_draw() would restore the pre-popup screen
                    // content over the freshly-drawn pane, hiding the new
                    // entry until an unrelated redraw (e.g. switching panes)
                    // happened to repaint it.
                    menu_popup_close();
                    dir_draw(activepane, 1);
                }
            }
        }
        else
        {
            menu_popup_close();
        }
    }
}

/**
 * recurse_walk() callback for dir_delete_recursive(): for every event except
 * RECURSE_ENTER_DIR, shows the current name/path on the popup and deletes it
 * via loci_unlink() (a file on RECURSE_FILE, or a now-emptied directory on
 * RECURSE_LEAVE_DIR). Aborts the walk on a delete error (after showing
 * menu_fileerrormessage()) or if ESC is pressed.
 *
 * @param ev       Type of recursion event.
 * @param entry    Directory entry for the current file/directory (used for
 *                   its name on RECURSE_FILE).
 * @param fullpath Full drive-prefixed path of the current file/directory.
 * @param userdata Pointer to the OricCharWin popup used for progress
 *                   feedback.
 * @return RECURSE_CONTINUE to keep walking, or RECURSE_ABORT to stop (on
 *         delete error or ESC).
 */
// recurse_walk() callback for dir_delete_recursive(): removes files as they
// are visited, and now-empty subdirectories on RECURSE_LEAVE_DIR.
static int8_t dir_delete_cb(RecurseEvent ev, const LociDirent *entry,
                             const char *fullpath, void *userdata)
{
    OricCharWin *popup = (OricCharWin *)userdata;

    if (ev == RECURSE_ENTER_DIR)
        return RECURSE_CONTINUE;

    cwin_fill_rect(popup, 0, 3, 36, 1, CH_SPACE);
    cwin_putat_string(popup, 0, 3, (ev == RECURSE_FILE) ? entry->d_name : fullpath);

    if (loci_unlink(fullpath) < 0)
    {
        menu_fileerrormessage();
        return RECURSE_ABORT;
    }

    if (keyb_check() == KEY_ESC)
        return RECURSE_ABORT;

    return RECURSE_CONTINUE;
}

/**
 * Recursively delete everything inside path, then path itself, showing
 * progress in a popup via dir_delete_cb(). If the walk was truncated
 * (recurse_truncated), path itself is left in place and
 * MSG_DIR_RECURSE_TRUNCATED is shown instead of deleting it. Waits for a
 * keypress before closing the popup. Caller has already confirmed via
 * menu_confirm_file(MSG_DIR_DELETE_RECURSE_Q, ...).
 *
 * @param path Drive-prefixed path of the directory to delete recursively.
 * @return (none)
 */
static void dir_delete_recursive(const char *path)
{
    OricCharWin popup;

    menu_popup_open(0, 8, 15);
    cwin_init(&popup, 2, 8, 38, 15, A_FWBLACK, A_BGWHITE);
    cwin_putat_string(&popup, 0, 1, MSG_DIR_DELETING);
    cwin_putat_string(&popup, 0, 5, MSG_FILE_ESC_CANCEL);

    if (recurse_walk(path, dir_delete_cb, &popup) == RECURSE_CONTINUE)
    {
        if (recurse_truncated)
        {
            cwin_fill_rect(&popup, 0, 3, 36, 1, CH_SPACE);
            cwin_putat_string(&popup, 0, 3, MSG_DIR_RECURSE_TRUNCATED);
        }
        else if (loci_unlink(path) < 0)
        {
            menu_fileerrormessage();
        }
    }

    cwin_fill_rect(&popup, 0, 5, 36, 1, CH_SPACE);
    cwin_putat_string(&popup, 0, 7, MSG_MENU_PRESSAKEY);
    cwin_getch();

    menu_popup_close();
}

/**
 * Delete the directory under the cursor in the active pane. For non-internal
 * drives, first checks whether the directory is empty (loci_opendir()/
 * loci_readdir()); if not empty, prompts via
 * menu_confirm_file(MSG_DIR_DELETE_RECURSE_Q, ...) and, if confirmed, deletes
 * it recursively via dir_delete_recursive(). Otherwise (empty, or drive 0
 * where emptiness can't be pre-checked), prompts via
 * menu_confirm_file(MSG_DIR_DELETE_Q, ...) and deletes via loci_unlink();
 * on drive 0, an EACCES from loci_unlink() is treated as "directory not
 * empty" and re-prompts for recursive deletion. Redraws the pane on success.
 * Shows menu_fileerrormessage() on any other error.
 *
 * @return (none) -- no-op if the cursor entry is not a directory, the
 *         listing is empty, or the active pane is a tape listing.
 */
void dir_deletedir(void)
{
    if (presentdir[activepane].firstelement && !insidetape[activepane] && presentdirelement.meta.type == 1)
    {
        dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, presentdirelement.name);

        // Check if dir is empty (skip for internal storage, drive 0:
        // its filesystem returns no entries here, so rely on unlink's
        // own error handling instead)
        if (presentdir[activepane].drive)
        {
            dir = loci_opendir(pathbuffer);
            if (!dir)
            {
                menu_fileerrormessage();
                return;
            }

            file = loci_readdir(dir);
            if (file && file->d_name[0] != '\0')
            {
                loci_closedir(dir);

                if (menu_confirm_file(MSG_DIR_DELETE_RECURSE_Q, presentdirelement.name))
                {
                    dir_delete_recursive(pathbuffer);
                    dir_draw(activepane, 1);
                }
                return;
            }
            loci_closedir(dir);
        }

        if (!menu_confirm_file(MSG_DIR_DELETE_Q, presentdirelement.name))
        {
            return;
        }

        if (loci_unlink(pathbuffer) < 0)
        {
            // Drive 0 (internal storage) can't be pre-checked for emptiness
            // above -- a non-empty directory surfaces here as an unlink
            // failure instead.
            if (!presentdir[activepane].drive && loci_errno == LOCI_EACCES)
            {
                if (menu_confirm_file(MSG_DIR_DELETE_RECURSE_Q, presentdirelement.name))
                {
                    dir_delete_recursive(pathbuffer);
                    dir_draw(activepane, 1);
                }
                return;
            }
            menu_fileerrormessage();
        }
        else
        {
            dir_draw(activepane, 1);
        }
    }
}

// -------------------------------------------------------------------------
// Properties popup
// -------------------------------------------------------------------------

// recurse_walk() callback for dir_show_properties(): accumulates the total
// size of every file under a directory. Also shows the directory currently
// being entered and a running count, both as progress feedback for large
// trees and as a breadcrumb of how far the walk got if the LOCI stops
// responding partway through. ESC aborts the calculation early (the running
// total at that point is shown as MSG_PROP_CANCELLED instead).
typedef struct
{
    OricCharWin *popup;
    uint32_t     total;
    uint16_t     dirs;
} SizeCbData;

/**
 * recurse_walk() callback for dir_show_properties(): on RECURSE_FILE, adds
 * entry->d_size to data->total; on RECURSE_ENTER_DIR, increments data->dirs
 * and shows the directory name and running count on data->popup as progress
 * feedback. Aborts the walk if ESC is pressed.
 *
 * @param ev       Type of recursion event.
 * @param entry    Directory entry for the current file/directory (used for
 *                   d_size on RECURSE_FILE, d_name on RECURSE_ENTER_DIR).
 * @param fullpath Full drive-prefixed path of the current file/directory
 *                   (unused).
 * @param userdata Pointer to a SizeCbData accumulating the result.
 * @return RECURSE_CONTINUE to keep walking, or RECURSE_ABORT if ESC was
 *         pressed.
 */
static int8_t dir_size_cb(RecurseEvent ev, const LociDirent *entry,
                           const char *fullpath, void *userdata)
{
    SizeCbData *data = (SizeCbData *)userdata;

    if (ev == RECURSE_FILE)
        data->total += entry->d_size;

    if (ev == RECURSE_ENTER_DIR)
    {
        data->dirs++;
        cwin_fill_rect(data->popup, 0, 6, 36, 2, CH_SPACE);
        cwin_putat_string(data->popup, 0, 6, entry->d_name);
        cwin_putat_printf(data->popup, 0, 7, MSG_PROP_CALCULATING_FMT, data->dirs);
    }

    if (keyb_check() == KEY_ESC)
        return RECURSE_ABORT;

    return RECURSE_CONTINUE;
}

/**
 * Show a popup with the name, type, path, and attributes (R/S flags) of the
 * entry under the cursor in the active pane, plus its size in bytes -- for
 * directories, recursively computed via recurse_walk()/dir_size_cb() with
 * live progress feedback and ESC-to-cancel (showing MSG_PROP_CANCELLED if
 * aborted). The type string is a known extension's "EXT - Description"
 * label, MSG_PROP_TYPE_DIR for directories, or the entry's own extension as
 * a fallback. Waits for a keypress before closing.
 *
 * @return (none) -- no-op if the listing is empty or the active pane is a
 *         tape listing.
 */
void dir_show_properties(void)
{
    OricCharWin popup;
    char     namebuf[64];
    char     sizebuf[11];
    char     attrstr[3];
    char     typebuf[24];
    char    *ext;
    uint8_t  namelen;
    uint8_t  attrib = 0;
    uint32_t filesize = 0;

    if (!presentdir[activepane].firstelement || insidetape[activepane])
        return;

    // Strip the trailing '/' that dir_read() appends to directory names --
    // loci_readdir() returns names without it.
    strcpy(namebuf, presentdirelement.name);
    namelen = (uint8_t)strlen(namebuf);
    if (namelen && namebuf[namelen - 1] == '/')
        namebuf[--namelen] = '\0';

    // Re-fetch d_attrib/d_size for the selected entry -- not stored in
    // DirElement/DirMeta.
    dir = loci_opendir(presentdir[activepane].path);
    if (dir)
    {
        while ((file = loci_readdir(dir)) != 0 && file->d_name[0] != '\0')
        {
            if (strcmp(file->d_name, namebuf) == 0)
            {
                attrib   = file->d_attrib;
                filesize = file->d_size;
                break;
            }
        }
        loci_closedir(dir);
    }

    // Build a descriptive type string: known extensions get their full
    // "EXT - Description" label, directories get MSG_PROP_TYPE_DIR, and
    // anything else falls back to the file's own extension (if any).
    switch (presentdirelement.meta.type)
    {
        case 1:
            strcpy(typebuf, MSG_PROP_TYPE_DIR);
            break;
        case 2:
            strcpy(typebuf, MSG_PROP_TYPE_DSK);
            break;
        case 3:
            strcpy(typebuf, MSG_PROP_TYPE_TAP);
            break;
        case 4:
            strcpy(typebuf, MSG_PROP_TYPE_ROM);
            break;
        case 5:
            strcpy(typebuf, MSG_PROP_TYPE_LCE);
            break;
        default:
            ext = strrchr(namebuf, '.');
            if (ext && dir_stricmp(ext, ".cfg") == 0)
            {
                strcpy(typebuf, MSG_PROP_TYPE_CFG);
            }
            else if (ext && dir_stricmp(ext, ".sys") == 0)
            {
                strcpy(typebuf, MSG_PROP_TYPE_SYS);
            }
            else if (ext)
            {
                strncpy(typebuf, ext, sizeof(typebuf) - 1);
                typebuf[sizeof(typebuf) - 1] = '\0';
            }
            else
            {
                typebuf[0] = '\0';
            }
            break;
    }

    menu_popup_open(0, 8, 12);
    cwin_init(&popup, 2, 8, 38, 12, A_FWBLACK, A_BGWHITE);

    cwin_putat_string(&popup, 0, 0, MSG_PROP_TITLE);
    cwin_putat_printf(&popup, 0, 2, MSG_PROP_NAME_FMT, presentdirelement.name);
    cwin_putat_printf(&popup, 0, 3, MSG_PROP_TYPE_FMT, typebuf);
    cwin_putat_printf(&popup, 0, 4, MSG_PROP_PATH_FMT, presentdir[activepane].path);

    attrstr[0] = (attrib & DIR_ATTR_RDO) ? 'R' : '-';
    attrstr[1] = (attrib & DIR_ATTR_SYS) ? 'S' : '-';
    attrstr[2] = '\0';
    cwin_putat_printf(&popup, 0, 5, MSG_PROP_ATTR_FMT, attrstr);

    if (presentdirelement.meta.type == 1)
    {
        SizeCbData data;
        int8_t     walkresult;

        data.popup = &popup;
        data.total = 0;
        data.dirs  = 0;

        cwin_putat_printf(&popup, 0, 7, MSG_PROP_CALCULATING_FMT, data.dirs);
        cwin_putat_string(&popup, 0, 9, MSG_FILE_ESC_CANCEL);

        dir_build_path(pathbuffer, sizeof(pathbuffer), presentdir[activepane].path, presentdirelement.name);
        walkresult = recurse_walk(pathbuffer, dir_size_cb, &data);

        cwin_fill_rect(&popup, 0, 6, 36, 4, CH_SPACE);

        if (walkresult == RECURSE_ABORT)
        {
            cwin_putat_string(&popup, 0, 7, MSG_PROP_CANCELLED);
        }
        else
        {
            dir_dec10(sizebuf, data.total);
            cwin_putat_printf(&popup, 0, 7, MSG_PROP_SIZE_FMT, sizebuf,
                              recurse_truncated ? MSG_PROP_BYTES_APPROX : MSG_PROP_BYTES);
        }
    }
    else
    {
        dir_dec10(sizebuf, filesize);
        cwin_putat_printf(&popup, 0, 7, MSG_PROP_SIZE_FMT, sizebuf, MSG_PROP_BYTES);
    }

    cwin_putat_string(&popup, 0, 11, MSG_MAIN_PRESS_CONTINUE);
    cwin_getch();
    menu_popup_close();
}
