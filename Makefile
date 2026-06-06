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
  MKDIR   = mkdir
else
  NULLDEV = /dev/null
  DEL     = $(RM)
  MKDIR   = mkdir -p
endif

# -------------------------------------------------------------------------
# Toolchain
# -------------------------------------------------------------------------

CC    = /home/xahmol/oscar64/bin/oscar64
PY    = python3
EMUL  = /home/xahmol/oricutron/oricutron

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
LOAD_ADDR = 0x0500
VERSION   = 2.0.0

# -------------------------------------------------------------------------
# Compiler flags
# -------------------------------------------------------------------------

CFLAGS = \
  -n              \
  -tf=bin         \
  -rt=include/oric_crt.c \
  -i=include      \
  -O2             \
  -dNOFLOAT       \
  $(LANGFLAG)

# -------------------------------------------------------------------------
# Source dependency lists
# Oscar64 follows #pragma compile chains internally; make does not —
# list everything here so rebuilds trigger correctly.
# -------------------------------------------------------------------------

LOCI_SRCS = \
  include/loci.c        \
  include/loci.h        \
  include/strings.h     \
  include/strings_en.h  \
  include/strings_fr.h

MAIN_SRCS = \
  src/main.c            \
  include/oric_crt.c    \
  include/oric.h        \
  include/keyboard.c    \
  include/keyboard.h    \
  include/charwin.c     \
  include/charwin.h     \
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
  include/strings_demo.h     \
  include/strings_demo_en.h  \
  include/strings_demo_fr.h

# -------------------------------------------------------------------------
# Emulator flags
# -------------------------------------------------------------------------

EMUFLAG = -ma --serial none --vsynchack off --turbotape on

# -------------------------------------------------------------------------
# Targets
# -------------------------------------------------------------------------

DEMO      = libdemo
DEMONAME  = LIBDEMO

.PHONY: all all-langs clean run libdemo libdemo-run

all: build/$(MAIN)$(LANGSUFFIX).tap

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
# Demo library targets
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

# Launch libdemo (EN by default) in Oricutron
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
