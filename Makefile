# Makefile - locifilemanager-v2 (Oric Atmos, Oscar64)
#
# Two-step build:
#   1. Oscar64 (-n -tf=bin -rt=include/oric_crt.c) -> build/*.bin
#   2. tools/mktap.py -> build/*.tap
#
# make run requires oricutron in /home/xahmol/oricutron/
# Overlay RAM features (cwin_push/cwin_pop) require LOCI — test on real hardware only.
# LOCI section in libdemo also requires real LOCI hardware (degrades gracefully without).

# -------------------------------------------------------------------------
# Cross-platform portability (Windows CMD vs POSIX)
# -------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
  NULLDEV = nul:
  DEL     = -del /f
  RMDIR   = rmdir /s /q
  MKDIR   = mkdir
else
  NULLDEV = /dev/null
  DEL     = $(RM)
  RMDIR   = $(RM) -r
  MKDIR   = mkdir -p
endif

# -------------------------------------------------------------------------
# Toolchain
# -------------------------------------------------------------------------

CC    = /home/xahmol/oscar64/bin/oscar64
PY    = python3
EMUL  = /home/xahmol/oricutron/oricutron

# -------------------------------------------------------------------------
# Build versioning
# -------------------------------------------------------------------------

VERSION_MAJOR     = 2
VERSION_MINOR     = 0
VERSION_PATCH     = 0
VERSION_TIMESTAMP = $(shell date "+%Y%m%d-%H%M")
VERSION           = v$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)-$(VERSION_TIMESTAMP)

# -------------------------------------------------------------------------
# Localisation
# -------------------------------------------------------------------------
# LANG=EN (default) or LANG=FR
# LANG=EN builds build/locifm.tap and build/libdemo.tap
# LANG=FR builds build/locifm_fr.tap and build/libdemo_fr.tap
# make all-langs builds all four

LANG ?= EN
ifeq ($(LANG),FR)
  LANGFLAG   = -dLANG_FR
  LANGSUFFIX = _fr
else
  LANGFLAG   =
  LANGSUFFIX =
endif

# -------------------------------------------------------------------------
# Project
# -------------------------------------------------------------------------

MAIN      = locifm
PROGNAME  = LOCIFM
DEMO      = libdemo
DEMONAME  = LIBDEMO
LOAD_ADDR = 0x0500
ZIPNAME   = $(MAIN)-v$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# -------------------------------------------------------------------------
# Compiler flags
# -------------------------------------------------------------------------

CFLAGS = \
  -n              \
  -tf=bin         \
  -rt=include/oric_crt.c \
  -i=include      \
  -i=src          \
  -O2             \
  -dNOFLOAT       \
  -dVERSION_MAJOR=$(VERSION_MAJOR) \
  -dVERSION_MINOR=$(VERSION_MINOR) \
  -dVERSION_PATCH=$(VERSION_PATCH) \
  $(LANGFLAG)

# -------------------------------------------------------------------------
# Source dependency lists
# Oscar64 follows #pragma compile chains internally; make does not —
# list everything here so rebuilds trigger correctly.
# -------------------------------------------------------------------------

LOCI_SRCS = \
  include/loci.c        \
  include/loci.h        \
  src/strings.h         \
  src/strings_en.h      \
  src/strings_fr.h

IJK_SRCS = \
  include/ijk.c         \
  include/ijk.h

MAIN_SRCS = \
  src/main.c            \
  include/oric_crt.c    \
  include/oric.h        \
  include/keyboard.c    \
  include/keyboard.h    \
  include/charwin.c     \
  include/charwin.h     \
  src/menu.h            \
  src/menu.c            \
  src/input.h           \
  src/input.c           \
  src/dir.h             \
  src/dir.c             \
  src/file.h            \
  src/file.c            \
  src/drive.h           \
  src/drive.c           \
  $(IJK_SRCS)           \
  $(LOCI_SRCS)

DEMO_SRCS = \
  src/libdemo.c              \
  include/oric_crt.c         \
  include/oric.h             \
  include/keyboard.c         \
  include/keyboard.h         \
  include/charwin.c          \
  include/charwin.h          \
  include/loci.c             \
  include/loci.h             \
  include/ijk.c              \
  include/ijk.h              \
  src/menu.h                 \
  src/menu.c                 \
  src/strings.h              \
  src/strings_en.h           \
  src/strings_fr.h           \
  include/strings_demo.h     \
  include/strings_demo_en.h  \
  include/strings_demo_fr.h

# -------------------------------------------------------------------------
# USB stick transfer — variable declarations
# -------------------------------------------------------------------------
# Set USBPATH in .env (gitignored) — path to the directory on the USB stick.
# Native Linux: /media/yourname/USBSTICK/oric
# WSL2: Windows drives auto-mount at /mnt/<letter>; USB stick on E: → /mnt/e/DEV/LFM2
# See .env.example for a template.

-include .env
USBPATH  ?= NOT_SET

# Derived from USBPATH — used for WSL2 auto-mount.
# Assumes USBPATH starts with /mnt/<letter>/... (standard WSL2 drvfs layout).
# Example: USBPATH=/mnt/e/DEV/LFM2 → USBMOUNT=/mnt/e, USBDRIVE=E:
USBMOUNT := $(shell echo "$(USBPATH)" | cut -d/ -f1-3)
USBDRIVE := $(shell echo "$(USBPATH)" | cut -d/ -f3 | tr a-z A-Z):

# Detect WSL2 at parse time so check-usb can branch without a shell function.
IS_WSL2  := $(shell grep -qi microsoft /proc/version 2>/dev/null && echo 1 || echo 0)

# -------------------------------------------------------------------------
# Emulator flags
# -------------------------------------------------------------------------

EMUFLAG = -ma --serial none --vsynchack off --turbotape on

# -------------------------------------------------------------------------
# Phosphoric automated testing
# -------------------------------------------------------------------------
# PHOSDIR is set in .env (see .env.example) -- checkout of
# https://github.com/benedictemarty/Phosphoric, providing oric1-emu and
# roms/basic11b.rom. Phosphoric emulates the LOCI MIA peripheral
# (--loci-flash DIR), so locifm can be fast-loaded (-t ... -f) under Atmos
# BASIC 1.1 and tested headless against a sandbox directory standing in
# for the LOCI's USB storage.

PHOSDIR  ?= NOT_SET
PHOS      = $(PHOSDIR)/oric1-emu
ATMOSROM  = $(PHOSDIR)/roms/basic11b.rom

CYCLES   ?= 8000000

# =========================================================================
# Targets
# all: must appear first so it is the default goal
# =========================================================================

.PHONY: all all-langs clean run libdemo libdemo-run docs zip check-usb usb \
        check-phosphoric sandbox-reset test test-quick test-capture

all: build/$(MAIN)$(LANGSUFFIX).tap

# -------------------------------------------------------------------------
# Main app
# -------------------------------------------------------------------------

# Step 1: compile main app to raw binary
build/$(MAIN)$(LANGSUFFIX).bin: $(MAIN_SRCS)
	@$(MKDIR) build 2>$(NULLDEV) ; true
	$(CC) $(CFLAGS) -o=build/$(MAIN)$(LANGSUFFIX).bin src/main.c

# Step 2: wrap binary in Oric tape header
build/$(MAIN)$(LANGSUFFIX).tap: build/$(MAIN)$(LANGSUFFIX).bin
	$(PY) tools/mktap.py \
	    build/$(MAIN)$(LANGSUFFIX).bin \
	    build/$(MAIN)$(LANGSUFFIX).tap \
	    $(PROGNAME) \
	    $(LOAD_ADDR)

# Launch locifm in Oricutron (must cd to oricutron dir — it loads ROMs from cwd)
run: build/$(MAIN)$(LANGSUFFIX).tap
	cd /home/xahmol/oricutron && \
	    $(EMUL) $(EMUFLAG) "$(CURDIR)/build/$(MAIN)$(LANGSUFFIX).tap"

# -------------------------------------------------------------------------
# Demo library
# -------------------------------------------------------------------------

libdemo: build/$(DEMO)$(LANGSUFFIX).tap

# Step 1: compile demo to raw binary
build/$(DEMO)$(LANGSUFFIX).bin: $(DEMO_SRCS)
	@$(MKDIR) build 2>$(NULLDEV) ; true
	$(CC) $(CFLAGS) -o=build/$(DEMO)$(LANGSUFFIX).bin src/libdemo.c

# Step 2: wrap demo binary in Oric tape header
build/$(DEMO)$(LANGSUFFIX).tap: build/$(DEMO)$(LANGSUFFIX).bin
	$(PY) tools/mktap.py \
	    build/$(DEMO)$(LANGSUFFIX).bin \
	    build/$(DEMO)$(LANGSUFFIX).tap \
	    $(DEMONAME) \
	    $(LOAD_ADDR)

# Launch libdemo in Oricutron
libdemo-run: libdemo
	cd /home/xahmol/oricutron && \
	    $(EMUL) $(EMUFLAG) "$(CURDIR)/build/$(DEMO)$(LANGSUFFIX).tap"

# -------------------------------------------------------------------------
# Build both language variants
# -------------------------------------------------------------------------

all-langs:
	$(MAKE) LANG=EN
	$(MAKE) LANG=FR
	$(MAKE) libdemo LANG=EN
	$(MAKE) libdemo LANG=FR

# -------------------------------------------------------------------------
# Documentation — generate PDF from Markdown (requires pandoc)
# README.pdf and README_fr.pdf are committed to git so they ship in the
# release ZIP even if the recipient does not have pandoc installed.
# -------------------------------------------------------------------------

docs: README.pdf README_fr.pdf

README.pdf: README.md pandoc-defaults.yaml pandoc-header.tex
	@if which pandoc >/dev/null 2>&1; then \
	    pandoc --defaults=pandoc-defaults.yaml README.md -o README.pdf; \
	else \
	    echo "WARNING: pandoc not found -- README.pdf not updated (install: sudo apt install pandoc texlive-xetex)"; \
	fi

README_fr.pdf: README_fr.md pandoc-defaults.yaml pandoc-header.tex
	@if which pandoc >/dev/null 2>&1; then \
	    pandoc --defaults=pandoc-defaults.yaml README_fr.md -o README_fr.pdf; \
	else \
	    echo "WARNING: pandoc not found -- README_fr.pdf not updated (install: sudo apt install pandoc texlive-xetex)"; \
	fi

# -------------------------------------------------------------------------
# Release ZIP — all tap images + documentation
# -------------------------------------------------------------------------

zip: all-langs docs
	$(MKDIR) build/$(ZIPNAME) 2>$(NULLDEV) ; true
	cp build/$(MAIN).tap      build/$(ZIPNAME)/
	cp build/$(MAIN)_fr.tap   build/$(ZIPNAME)/
	cp build/$(DEMO).tap      build/$(ZIPNAME)/
	cp build/$(DEMO)_fr.tap   build/$(ZIPNAME)/
	cp README.md              build/$(ZIPNAME)/
	cp README_fr.md           build/$(ZIPNAME)/
	cp README.pdf             build/$(ZIPNAME)/
	cp README_fr.pdf          build/$(ZIPNAME)/
	cd build && zip -r $(ZIPNAME).zip $(ZIPNAME)/
	$(RMDIR) build/$(ZIPNAME) 2>$(NULLDEV) ; true

# -------------------------------------------------------------------------
# USB stick transfer — copies all tap images to mounted USB stick
# -------------------------------------------------------------------------

check-usb:
	@test "$(USBPATH)" != "NOT_SET" || \
	    (echo "ERROR: USBPATH not set -- copy .env.example to .env and set USBPATH" && false)
	@if ! test -d "$(USBPATH)"; then \
	    if [ "$(IS_WSL2)" = "1" ]; then \
	        echo "WSL2: mounting $(USBDRIVE) at $(USBMOUNT) via drvfs..."; \
	        sudo mount -t drvfs $(USBDRIVE) $(USBMOUNT); \
	    fi; \
	fi
	@test -d "$(USBPATH)" || \
	    (echo "ERROR: USB path '$(USBPATH)' not found -- plug in USB stick and retry" && false)

usb: check-usb all-langs
	cp build/$(MAIN).tap      "$(USBPATH)/"
	cp build/$(MAIN)_fr.tap   "$(USBPATH)/"
	cp build/$(DEMO).tap      "$(USBPATH)/"
	cp build/$(DEMO)_fr.tap   "$(USBPATH)/"
	@if [ "$(IS_WSL2)" = "1" ]; then \
	    echo "WSL2: unmounting $(USBMOUNT)..."; \
	    sudo umount $(USBMOUNT); \
	    echo "Done -- USB stick can now be ejected in Windows."; \
	fi

# -------------------------------------------------------------------------
# Phosphoric automated testing
# -------------------------------------------------------------------------
# make test-quick   -- boot smoke test (LOCI detection + main interface)
# make test         -- full automated suite (currently == test-quick)
# make test-capture CYCLES=N TYPEKEYS='...'
#                   -- calibration helper: fast-loads locifm.tap (-t ... -f)
#                      under Atmos BASIC 1.1 with --loci-flash tests/sandbox
#                      as the LOCI device, runs for CYCLES, dumps
#                      tests/out/capture.bin (RAM) and tests/out/capture.png
#                      (screenshot). No assertions -- used to find the right
#                      cycle counts and --type-keys sequences (e.g. for menu
#                      navigation) for new test scenarios.

check-phosphoric:
	@test "$(PHOSDIR)" != "NOT_SET" || \
	    (echo "ERROR: PHOSDIR not set -- copy .env.example to .env and set PHOSDIR" && false)
	@test -x "$(PHOS)" || \
	    (echo "ERROR: oric1-emu not found/executable at $(PHOS) -- check PHOSDIR in .env" && false)
	@test -f "$(ATMOSROM)" || \
	    (echo "ERROR: Atmos ROM not found at $(ATMOSROM)" && false)

# Reset the LOCI sandbox from checked-in fixtures + freshly built tap, so
# every test run (including file-operation tests) starts from a known state.
sandbox-reset: build/$(MAIN)$(LANGSUFFIX).tap
	$(RMDIR) tests/sandbox 2>$(NULLDEV) ; true
	$(MKDIR) tests/sandbox 2>$(NULLDEV) ; true
	cp -r tests/fixtures/. tests/sandbox/
	cp build/$(MAIN)$(LANGSUFFIX).tap tests/sandbox/

test-capture: check-phosphoric sandbox-reset
	$(MKDIR) tests/out 2>$(NULLDEV) ; true
	$(PHOS) -r $(ATMOSROM) --loci-flash tests/sandbox \
	    -t tests/sandbox/$(MAIN)$(LANGSUFFIX).tap -f \
	    --headless -c $(CYCLES) \
	    $(if $(TYPEKEYS),--type-keys '$(TYPEKEYS)') \
	    --dump-ram-at $(CYCLES):tests/out/capture.bin \
	    --screenshot-at $(CYCLES):tests/out/capture.ppm
	@which pnmtopng >$(NULLDEV) 2>&1 && pnmtopng tests/out/capture.ppm > tests/out/capture.png || true
	python3 tests/scripts/oric_screen.py tests/out/capture.bin

test-quick: check-phosphoric sandbox-reset
	$(MKDIR) tests/out 2>$(NULLDEV) ; true
	PHOS=$(PHOS) ATMOSROM=$(ATMOSROM) SANDBOX=tests/sandbox OUT=tests/out \
	    TAPFILE=$(MAIN)$(LANGSUFFIX).tap \
	    bash tests/scripts/test_boot.sh

test: test-quick

# -------------------------------------------------------------------------
# Clean
# -------------------------------------------------------------------------

clean:
	$(DEL) build/$(MAIN).bin      2>$(NULLDEV) ; true
	$(DEL) build/$(MAIN).tap      2>$(NULLDEV) ; true
	$(DEL) build/$(MAIN)_fr.bin   2>$(NULLDEV) ; true
	$(DEL) build/$(MAIN)_fr.tap   2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO).bin      2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO).tap      2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO)_fr.bin   2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO)_fr.tap   2>$(NULLDEV) ; true
	$(DEL) build/*.zip            2>$(NULLDEV) ; true
	$(RMDIR) tests/sandbox        2>$(NULLDEV) ; true
	$(RMDIR) tests/out            2>$(NULLDEV) ; true
