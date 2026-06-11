// drive.h - LOCI drive enumeration / mount / unmount for locifilemanager v2
//
// Based on v1 locifilemanager drive.h/drive.c by Xander Mol, 2025 — local
// reference at locifilemanager/include/drive.h, locifilemanager/src/drive.c.
//
// Adapted: stdint types; mount()/umount() -> loci_mount()/loci_umount().

#ifndef DRIVE_H
#define DRIVE_H

#include <stdint.h>

// Mounted filenames per drive slot: A, B, C, D, Tape, ROM
extern char mount_filename[6][21];

// Boot-mode flags: which devices should be active on exit (disk > tape > ROM)
extern uint8_t fdc_on;  // FDC (disk drives A-D) active
extern uint8_t tap_on;  // Tape deck active
extern uint8_t b11_on;  // ROM box active
extern uint8_t bit_on;  // (reserved, mirrors v1)
extern uint8_t ald_on;  // Tape auto-load on boot

// -------------------------------------------------------------------------
// Function prototypes
// -------------------------------------------------------------------------

void drive_targetdrive(void);
void drive_unmount_all(void);
void drive_showmounts(void);
void drive_mount(void);
void drive_unmount(void);

#pragma compile("drive.c")

#endif  // DRIVE_H
