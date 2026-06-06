# Makefile - locifilemanager-v2 (Oric Atmos, Oscar64)
#
# Two-step build:
#   1. Oscar64 (-n -tf=bin -rt=include/oric_crt.c) → build/locifm.bin
#   2. tools/mktap.py → build/locifm.tap
#
# make run requires oricutron in /home/xahmol/oricutron/
# Overlay RAM features (cwin_push/cwin_pop) require LOCI — test on real hardware only.

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
  -dNOFLOAT

# -------------------------------------------------------------------------
# Source dependency list (Oscar64 follows #pragma compile chains internally;
# make does not — list everything here so rebuilds trigger correctly)
# -------------------------------------------------------------------------

MAIN_SRCS = \
  src/main.c           \
  include/oric_crt.c   \
  include/oric.h       \
  include/keyboard.c   \
  include/keyboard.h   \
  include/charwin.c    \
  include/charwin.h

# -------------------------------------------------------------------------
# Emulator flags
# -------------------------------------------------------------------------

EMUFLAG = -ma --serial none --vsynchack off --turbotape on

# -------------------------------------------------------------------------
# Targets
# -------------------------------------------------------------------------

DEMO      = libdemo
DEMONAME  = LIBDEMO

DEMO_SRCS = \
  src/libdemo.c         \
  include/oric_crt.c    \
  include/oric.h        \
  include/keyboard.c    \
  include/keyboard.h    \
  include/charwin.c     \
  include/charwin.h

.PHONY: all clean run libdemo libdemo-run

all: build/$(MAIN).tap

# Step 1: compile to raw binary
build/$(MAIN).bin: $(MAIN_SRCS)
	@$(MKDIR) build 2>$(NULLDEV) ; true
	$(CC) $(CFLAGS) -o=build/$(MAIN).bin src/main.c

# Step 2: wrap binary in Oric tape header
build/$(MAIN).tap: build/$(MAIN).bin
	$(PY) tools/mktap.py \
	    build/$(MAIN).bin \
	    build/$(MAIN).tap \
	    $(PROGNAME) \
	    $(LOAD_ADDR)

# Launch locifm in Oricutron (must cd to oricutron dir — it loads ROMs from cwd)
run: build/$(MAIN).tap
	cd /home/xahmol/oricutron && \
	    $(EMUL) $(EMUFLAG) "$(CURDIR)/build/$(MAIN).tap"

# -------------------------------------------------------------------------
# Demo library targets
# -------------------------------------------------------------------------

libdemo: build/$(DEMO).tap

# Step 1: compile demo to raw binary
build/$(DEMO).bin: $(DEMO_SRCS)
	@$(MKDIR) build 2>$(NULLDEV) ; true
	$(CC) $(CFLAGS) -o=build/$(DEMO).bin src/libdemo.c

# Step 2: wrap demo binary in Oric tape header
build/$(DEMO).tap: build/$(DEMO).bin
	$(PY) tools/mktap.py \
	    build/$(DEMO).bin \
	    build/$(DEMO).tap \
	    $(DEMONAME) \
	    $(LOAD_ADDR)

# Launch libdemo in Oricutron
libdemo-run: libdemo
	cd /home/xahmol/oricutron && \
	    $(EMUL) $(EMUFLAG) "$(CURDIR)/build/$(DEMO).tap"

clean:
	$(DEL) build/$(MAIN).bin 2>$(NULLDEV) ; true
	$(DEL) build/$(MAIN).tap 2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO).bin 2>$(NULLDEV) ; true
	$(DEL) build/$(DEMO).tap 2>$(NULLDEV) ; true
