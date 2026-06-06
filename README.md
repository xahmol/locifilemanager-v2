# locifilemanager-v2

File manager for the LOCI mass storage device on Oric Atmos — v2 rebuild using Oscar64.

## Contents

1. [Introduction](#introduction)
2. [Requirements](#requirements)
3. [Building from source](#building-from-source)
4. [Installation](#installation)
5. [Usage](#usage)
6. [Version history and download](#version-history-and-download)
7. [License](#license)

---

## Introduction

locifilemanager is a full-screen file manager for the
[LOCI](https://github.com/sodiumlb/loci-rom) mass storage device on the
Oric Atmos home computer. It lets you browse, copy, move, delete, and rename
files on LOCI-connected storage; mount and unmount disk, tape, and ROM images;
and boot from mounted media.

This is version 2 — a complete rewrite using the
[Oscar64](https://github.com/drmortalwombat/oscar64) C compiler targeting the
6502A bare-metal (no ROM calls). The v1 CC65 implementation is at
[locifilemanager](https://github.com/xahmol/locifilemanager).

Available in English and French.

**Current status:** Library infrastructure (charwin window library, keyboard
scanner, full LOCI API) is complete and tested. Phase 4 — application
rebuild — is in progress.

---

## Requirements

### Hardware

- Oric Atmos
- [LOCI](https://github.com/sodiumlb/loci-rom) mass storage device
- USB storage connected to LOCI

### For emulator testing

- [Oricutron](http://www.defence-force.org/index.php?page=articles&art=oricutron)
  emulator

> **Note:** LOCI hardware features (file access, directory listing, mount/unmount)
> require a real LOCI device. Oricutron does not emulate LOCI.

### For building from source

| Tool | Purpose | Install |
|---|---|---|
| [Oscar64](https://github.com/drmortalwombat/oscar64) | C compiler for 6502 | Build from source |
| Python 3 | Tape image wrapper (`mktap.py`) | `sudo apt install python3` |
| pandoc | PDF documentation generation | `sudo apt install pandoc` |
| zip | Release archive | `sudo apt install zip` |

---

## Building from source

```sh
git clone https://github.com/xahmol/locifilemanager-v2
cd locifilemanager-v2
```

| Target | Action |
|---|---|
| `make` | Build English app (`build/locifm.tap`) |
| `make LANG=FR` | Build French app (`build/locifm_fr.tap`) |
| `make all-langs` | Build all four tap images (EN+FR for app and demo) |
| `make run` | Build + launch EN app in Oricutron |
| `make libdemo` | Build library demo (`build/libdemo.tap`) |
| `make libdemo-run` | Build + launch EN demo in Oricutron |
| `make docs` | Regenerate PDF documentation from Markdown |
| `make zip` | Build all images + docs + release ZIP |
| `make usb` | Copy all tap images to USB stick (see below) |
| `make clean` | Remove build artefacts |

### USB transfer to real hardware

Copy tap images directly to a mounted USB stick:

1. Copy `.env.example` to `.env` (`.env` is gitignored):
   ```sh
   cp .env.example .env
   ```
2. Edit `.env` and set `USBPATH` to the directory on your mounted USB stick
   where the tap files should land:
   ```
   USBPATH = /media/yourname/USBSTICK/oric
   ```
3. Mount the USB stick, then run:
   ```sh
   make usb
   ```
   This builds all four tap images and copies them to `USBPATH`.

---

## Installation

1. Copy `locifm.tap` (English) or `locifm_fr.tap` (French) to your LOCI
   USB storage.
2. On the Oric, load from the LOCI prompt or via BASIC:
   ```
   CLOAD""
   ```

---

## Usage

*(Application rebuild in progress — this section will be completed in Phase 4.)*

The application provides:

- Two-pane directory browser
- File operations: copy, move, delete, rename
- Tape image browser
- Drive management: mount/unmount disk, tape, and ROM images
- Boot from active mounts (disk priority over tape over ROM)
- IJK joystick support (Raxiss IJK interface)
- English and French interface

---

## Version history and download

| Version | Date | Notes |
|---|---|---|
| 2.0.0 | 2026 | Oscar64 rebuild — in development |
| 1.x | 2025 | [CC65 version](https://github.com/xahmol/locifilemanager) |

---

## License

Copyright (C) 2025-2026 Xander Mol.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3.

See the [LICENSE](LICENSE) file for the full licence text.
