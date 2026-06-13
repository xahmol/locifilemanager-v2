# Contributing to locifilemanager-v2

Thank you for your interest in contributing to locifilemanager-v2 — a
bare-metal file manager for the LOCI mass-storage device on the Oric Atmos,
built with the Oscar64 compiler. This document describes how to set up a
development environment, the coding and documentation conventions the
project follows, and what is required for a pull request to be accepted.

For a deep dive into the memory map, data structures, module layout and
application flow, see [ARCHITECTURE.md](ARCHITECTURE.md). For day-to-day
build/test commands and Oscar64-specific gotchas, see
[CLAUDE.md](CLAUDE.md) and [oscar64manual.md](oscar64manual.md).

---

## 1. Before you start

### Prerequisites

| Tool | Purpose |
|---|---|
| [Oscar64](https://github.com/drmortalwombat/oscar64) | C compiler for 6502, native mode |
| Python 3 | `tools/mktap.py` (tape image wrapper), test scripts |
| pandoc | Regenerating PDF docs (`make docs`) |
| [Oricutron](http://www.defence-force.org/index.php?page=articles&art=oricutron) | Manual testing in emulator (no LOCI support) |
| [Phosphoric](https://github.com/xahmol/Phosphoric) (`oric1-emu`) | Automated headless testing (LOCI emulated via host-fs sandbox) |

### Build and run

```sh
make              # build build/locifm.tap (English)
make LANG=FR      # build build/locifm_fr.tap (French)
make all-langs    # build all four .tap images (app + libdemo, EN + FR)
make run          # build + launch in Oricutron
make libdemo-run  # build + launch the library demo in Oricutron
```

LOCI-dependent features (file I/O, mount/unmount, overlay RAM) cannot be
exercised in Oricutron — use Phosphoric (`make test*`, §3) or real hardware.

### Reference implementation

The v1 CC65 implementation at
[locifilemanager](https://github.com/xahmol/locifilemanager) is the
functional reference for feature behaviour, screen layouts and UX — when in
doubt about *what* a feature should do, check how v1 does it. v2 is a
from-scratch Oscar64 rewrite, not a line-by-line port, so implementations may
differ where Oscar64/bare-metal constraints require it (see §2.3).

---

## 2. Coding conventions

### 2.1 Style

- C, `stdint.h`/`stdbool.h` types throughout (`uint8_t`, `uint16_t`,
  `int16_t`, `uint32_t`, `bool`) — no plain `int`/`unsigned`/`char` for
  anything that crosses a LOCI wire-protocol or struct boundary.
- Include guards: `#ifndef FOO_H` / `#define FOO_H` / `#endif // FOO_H`.
- Each module header ends with `#pragma compile("module.c")` so including
  the header pulls in the implementation (Oscar64 whole-program build) —
  follow this pattern for any new module.
- Match the existing module's formatting (brace style, spacing, comment
  banners `// ---...---`) rather than introducing a new style within a file.

### 2.2 Localisation — `MSG_*` strings

**All user-visible strings must be `MSG_*` macros**, defined in both
`src/strings_en.h` and `src/strings_fr.h` (or `strings_demo_en.h`/
`strings_demo_fr.h` for `libdemo.c`). No raw string literals in logic files.
Retroactively extracting literals later is painful — add both languages in
the same commit. French strings are **unaccented** (the Oric ROM charset has
no é/è/à/ç) — follow existing conventions ("Deplacer", "Effacer", "Oui",
"Non").

### 2.3 Oscar64 / bare-metal constraints

These are hard constraints, not style preferences — code that violates them
may build successfully but fail or hang on real hardware:

- **No C-level recursion.** The software stack is 512 bytes
  (`#pragma stacksize(0x0200)`, `include/oric_crt.c`) and shared by the whole
  call chain. Any tree-walk must use `recurse_walk()`
  (`src/recurse.c/h`, explicit frame stack, `RECURSE_MAX_DEPTH=8`) or a
  similar iterative pattern.
- **No heap.** `malloc` returns `NULL` in this runtime configuration. Use
  static/global fixed-size buffers.
- **`$B400` ceiling.** Code/data/BSS/stack must not extend into charset RAM
  (`$B400+`) — watch `build/*.map` after adding significant static data.
- **No floats** (`-dNOFLOAT`). `cwin_printf` supports only
  `%d/%u/%x/%s/%c/%%`; for `uint32_t` decimal output follow the
  `dir_dec10()` pattern (`src/dir.c`).
- **IRQ policy:** the program runs permanently under `SEI` with no IRQ
  handler. Code that must transiently enable interrupts (overlay-RAM access,
  VIA Port A access in `ijk.c`) **must** use `PHP`/`SEI` ... `PLP`, never
  `SEI`/`CLI` — an unconditional `CLI` permanently re-enables IRQs and lets
  the stock ROM handler corrupt zero page / screen RAM. See
  `ARCHITECTURE.md` §2.6.
- Consult `oscar64manual.md` before relying on `<stdarg.h>`, casts on struct
  members, ternaries returning pointers, or macros expanding to volatile
  reads inside braces-free loop bodies — these have known Oscar64 `-n -O2`
  miscompilation gotchas already documented there.

### 2.4 Code attribution

When code is derived from, adapted from, or inspired by an external source
(reference manual, third-party library, open-source project, forum post,
assembly listing, the v1 CC65 implementation, etc.), add a credit comment at
the top of the function or block naming:
- the original author or project,
- where it can be found (file, URL, or "local reference at `<path>`"),
- what was changed or adapted.

Existing examples are throughout `include/loci.c`, `include/ijk.c` and
`src/main.c` — follow the same format.

### 2.5 Comments

Default to no comments. Only add one when the *why* is non-obvious — a
hidden hardware constraint, a workaround for a specific bug, a subtle
invariant. Don't restate what well-named identifiers already say.

---

## 3. Testing requirements

### 3.1 All existing tests must pass

```sh
make test
```

runs the full automated suite under Phosphoric:
`test-quick`, `test-menus`, `test-fileops`, `test-libdemo`, `test-recurse`,
`test-namefilter`, `test-copycancel`, `test-viewer`.

**A pull request will not be merged if `make test` does not pass cleanly.**
If a change legitimately alters expected behaviour that an existing test
checks for, update that test in the same PR and explain why in the PR
description.

### 3.2 New features and bug fixes need test coverage

- **New features**: add a new `tests/scripts/test_<name>.sh` (or extend an
  existing one if the feature is part of an existing area, e.g. an
  additional Tools-pulldown item extending `test-recurse`), and wire it into
  the `test:`/`test-<name>:` targets in the `Makefile` following the
  existing pattern (`check-phosphoric sandbox-reset`, then the script).
- **Bug fixes**: add a test that fails without your fix and passes with it —
  this is the best evidence the fix is correct and prevents regressions.
- **Library additions** (`include/charwin.c`, `include/loci.c`,
  `include/ijk.c`): exercise the new function from `src/libdemo.c` (next
  available lettered section) so `make test-libdemo` covers it.

How a test script works (see any `tests/scripts/test_*.sh` for a full
example):
1. Uses `--type-keys` to drive the built `.tap` under `oric1-emu`
   (`tests/sandbox/`, seeded from `tests/fixtures/` by `sandbox-reset`).
2. Takes `--dump-ram-at` RAM snapshots at calibrated cycle counts.
3. Uses `tests/scripts/oric_screen.py --find`/`--row` to assert on the
   decoded 40×28 text screen.
4. Reports `[PASS]`/`[FAIL]` per assertion and a pass/fail/skip summary,
   exiting non-zero on any failure.

Use `make test-capture CYCLES=N TYPEKEYS='...'` to calibrate cycle counts and
inspect the resulting screen (`tests/out/capture.png`/`.bin`) while writing a
new script.

### 3.3 Things that can't be tested under Phosphoric/Oricutron

Overlay-RAM features (`cwin_push`/`cwin_pop`, menu window-save/restore) and
some MIA edge cases require real LOCI hardware. If your change touches these
paths, say so explicitly in the PR description and describe what manual
hardware testing (if any) you were able to do — this is fine, but reviewers
need to know automated coverage is incomplete for that path.

---

## 4. Documentation requirements

Update documentation in the same PR as the code change it describes:

- **`README.md` and `README_fr.md`** — keep both in sync for any
  user-visible feature, keyboard shortcut, menu item, or requirement change.
  Run `make docs` to confirm the PDFs still regenerate cleanly (requires
  `pandoc`).
- **`libmanual.md` / `libmanual_fr.md`** — update for any change to the
  public API of `charwin.h`, `loci.h`, `ijk.h`, or `keyboard.h`.
- **`ARCHITECTURE.md`** — update if you change the memory map, XRAM/overlay
  RAM layout, a core data structure, or the application's control flow.
- **`ROADMAP.md`** — if your change is part of a tracked phase/feature, tick
  it off or add a note; new substantial features should get a brief entry.
- **`CLAUDE.md`** — update if you discover a new Oscar64 gotcha, hardware
  quirk, or change a convention described there (and mirror durable Oscar64
  findings into `oscar64manual.md` per that file's own instructions).

---

## 5. Commit messages and workflow

1. Create a feature/fix branch from `main`.
2. Implement the change, following §2.
3. Add/extend tests (§3) and documentation (§4).
4. Run `make test` — all suites must pass.
5. Commit using [Conventional Commits](https://www.conventionalcommits.org/):
   ```
   feat: add wildcard name filter to Tools menu
   fix: correct off-by-one in dir_pagedown at last page
   test: add coverage for mid-copy cancellation
   docs: document XRAM directory-list layout in ARCHITECTURE.md
   ```
6. Push your branch and open a pull request.

---

## 6. Pull request checklist

Before submitting, confirm:

- [ ] `make test` passes (all 8 suites).
- [ ] New features/fixes have new or extended tests under `tests/scripts/`.
- [ ] All user-visible strings are `MSG_*` macros, added to both
      `strings_en.h` and `strings_fr.h`.
- [ ] No C-level recursion, no `malloc`, no code that could push
      code/data/stack past `$B400`.
- [ ] Any transient-IRQ-enable code uses `PHP`/`SEI`/`PLP`, not `SEI`/`CLI`.
- [ ] Adapted/derived code carries an attribution comment (§2.4).
- [ ] `README.md`/`README_fr.md`, `libmanual.md`/`libmanual_fr.md`,
      `ARCHITECTURE.md` and `ROADMAP.md` updated where relevant.
- [ ] Commit messages follow Conventional Commits.
- [ ] If the change touches overlay-RAM/XRAM/real-hardware-only paths, the
      PR description says what (if anything) was tested on real LOCI
      hardware.

## 7. Reporting issues

When reporting a bug, please include:
- Steps to reproduce (keys pressed, menu path).
- Expected vs. actual behaviour.
- Whether it reproduces in Oricutron, under `make test`/Phosphoric, on real
  LOCI hardware, or some combination — and the LOCI firmware version if
  hardware is involved (`get_locicfg()`/`locicfg.uname.release`).
- Build: `make` vs `make LANG=FR`, and the commit/tag you built from.
