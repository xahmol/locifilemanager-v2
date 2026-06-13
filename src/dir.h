// dir.h - Directory engine for locifilemanager v2 (two-pane browser)
//
// Based on v1 locifilemanager dir.h/dir.c by Xander Mol, 2025 — local
// reference at locifilemanager/include/dir.h, locifilemanager/src/dir.c.
//
// Adapted: stdint types throughout (unsigned -> uint16_t, unsigned char ->
// uint8_t); DIR*/struct dirent -> LociDir*/LociDirent (loci.h). Panes are
// rendered via OricCharWin/cwin_* (dir.c), not raw conio.

#ifndef DIR_H
#define DIR_H

#include <stdint.h>
#include "loci.h"

// -------------------------------------------------------------------------
// XRAM layout for directory linked lists
// -------------------------------------------------------------------------

#define DIR1BASE  (COPYBUF_XRAM_ADDR + COPYBUF_XRAM_SIZE)  // 0x8800
#define DIRSIZE   0x0C00
#define DIR2BASE  (DIR1BASE + DIRSIZE)                     // 0x9400

// -------------------------------------------------------------------------
// Pane location and height
// -------------------------------------------------------------------------

#define PANE1_YPOS   3
#define PANE2_YPOS  15
#define PANE_HEIGHT 10

// -------------------------------------------------------------------------
// Structs and variables
// -------------------------------------------------------------------------

// Directory entry meta data, stored alongside the name in XRAM
struct DirMeta
{
    uint16_t next;    // XRAM pointer to next element (0 = none)
    uint16_t prev;    // XRAM pointer to previous element (0 = none)
    uint8_t  type;    // Type: 1=dir, 2=DSK, 3=TAP, 4=ROM, 5=LCE
    uint8_t  select;  // Select: 0=not selected, 1=selected
    uint8_t  length;  // Length of name
};

// Directory element: name + meta data
struct DirElement
{
    char           name[64]; // Entry name
    struct DirMeta meta;     // Meta data
};
extern struct DirElement presentdirelement;

// Active directory state for one pane
struct Directory
{
    uint16_t firstelement;  // XRAM pointer to first element
    uint16_t firstprint;    // XRAM pointer to first element to print
    uint16_t lastprint;     // XRAM pointer to last element to print
    uint16_t present;       // XRAM pointer to active element
    uint8_t  drive;         // Drive number
    uint8_t  position;      // Position in directory
    char     path[256];     // Path
    uint16_t address;       // Address in XRAM for next entry
};
extern struct Directory presentdir[2];

// Directory reading variables
extern LociDir    *dir;
extern LociDirent *file;
extern char dir_entry_types[8][4]; // Directory entry type name text strings

// User-configurable app settings, grouped in one struct so that persistent
// storage (if reintroduced) can save/load it in a single read/write.
struct AppSettings
{
    uint8_t confirm;     // Confirm once (0) or all (1)
    uint8_t filter;      // Filter for file type, 0: None, 1: DSK, 2: TAP, 3: ROM, 4: LCE
    uint8_t enterchoice; // Choice for enter action: 0: Select, 1: Mount or 2: Launch
    uint8_t sort;        // Sort on (1) or off (0)
};
extern struct AppSettings settings;

// Persistent settings, saved to/loaded from FMCONFIG_PATH (config_save()/
// config_load()). magic guards against a missing/foreign/corrupt file --
// on mismatch, config_load() falls back to writing the compiled-in defaults.
#define FMCONFIG_DIR1  "0:/idi8b"
#define FMCONFIG_DIR2  "0:/idi8b/locifm"
#define FMCONFIG_PATH  "0:/idi8b/locifm/locifm.cfg"
#define FMCONFIG_MAGIC 0xA5

// Bookmarked directory paths (global, shared by both panes).
#define FMCONFIG_FAV_SLOTS   8
#define FMCONFIG_FAV_PATHLEN 48

struct FmConfig
{
    uint8_t magic;
    uint8_t confirm;
    uint8_t filter;
    uint8_t enterchoice;
    uint8_t sort;
    char    favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN];
};

// Application variables
extern uint8_t  activepane;    // Number of active pane: 0 is upper, 1 is lower
extern uint16_t present;       // Present element (XRAM pointer)
extern uint16_t previous;      // Previous element (XRAM pointer)
extern uint16_t next;          // Next element (XRAM pointer)
extern uint8_t  targetdrive;   // Target drive for mount: 0: A, 1: B, 2: C, 3: D
extern uint16_t selection[2];  // Number of selected entries per pane
extern uint8_t  insidetape[2]; // Browser is inside a tape .TAP container file

// Name filter pattern ('*'/'?' wildcards, case-insensitive, dir_read() only).
// Empty string = no name filter.
extern char namefilter[32];

// Bookmarked directory paths, persisted via config_save()/config_load().
// favourites[i][0] == '\0' marks an unused slot.
extern char favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN];

// Buffers for full paths
extern char pathbuffer[256];
extern char pathbuffer2[256];
extern char pathbuffer3[256];

// -------------------------------------------------------------------------
// Function prototypes
// -------------------------------------------------------------------------

void dir_build_path(char *dest, uint16_t destsize, const char *dirpath, const char *name);
void dir_get_element(uint16_t address);
void dir_save_element(uint16_t address);
void dir_refresh_present(void);
void dir_read(uint8_t dirnr, uint8_t filterval);
void dir_tape_parse(uint8_t dirnr);
void dir_print_id_and_path(uint8_t dirnr);
void dir_print_entry(uint8_t dirnr, uint8_t printpos);
void dir_draw(uint8_t dirnr, uint8_t readdir);
void dir_get_next_drive(uint8_t dirnr);
void dir_get_prev_drive(uint8_t dirnr);
void dir_switch_pane(void);
void dir_go_down(void);
void dir_go_up(void);
void dir_pagedown(void);
void dir_pageup(void);
void dir_top(void);
void dir_bottom(void);
void dir_last_of_page(void);
void dir_select_toggle(void);
void dir_select_all(uint8_t select);
void dir_select_inverse(void);
void dir_gotoroot(void);
void dir_parentdir(void);
void dir_togglesort(void);
void config_load(void);
void config_save(void);
void favourites_add(uint8_t slot);
void favourites_delete(uint8_t slot);
void favourites_goto(uint8_t slot);
void favourites_show(void);
void dir_newdir(void);
void dir_deletedir(void);
void dir_show_properties(void);

#pragma compile("dir.c")

#endif  // DIR_H
