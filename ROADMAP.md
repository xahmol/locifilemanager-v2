# LOCI File Manager v2 — Roadmap

## Purpose

This document has two parts:

1. **Completed work** — a summary of all functionality and architectural
   changes delivered in locifilemanager-v2, described relative to the **v1
   (CC65)** baseline at https://github.com/xahmol/locifilemanager. This is a
   feature-level summary, not a commit-by-commit changelog or a record of
   every intermediate step taken to get from v1 to the current state — see
   `git log` for that level of detail. The detailed phase-by-phase plan and
   implementation notes that produced the "New file-management features"
   below are preserved in git history (`ROADMAP.md` as of commit `93ac053`
   and earlier).
2. **Planned / future work** — development phases or substantial features
   that have been designed but not yet started. Empty right now; see
   [Adding a new plan](#adding-a-new-plan) for the format to use when that
   changes.

---

## Completed: v1 to v2

v2 is a from-scratch rewrite of v1, replacing the CC65 toolchain with
Oscar64 and targeting the Oric Atmos bare-metal (no ROM calls). Everything
below is new or substantially changed relative to v1.

### 1. Platform rewrite (Oscar64, bare-metal)

- Complete rewrite from CC65 to **Oscar64**, native mode, bare-metal 6502 —
  no Oric ROM calls anywhere (screen, keyboard, sound and storage are all
  driven directly via memory-mapped registers).
- New custom runtime (`include/oric_crt.c`, `include/crt_math.c`) replacing
  CC65's runtime: defines memory regions, startup code, and integer/float
  math helpers (`-dNOFLOAT`).
- New **charwin** character-window library (`include/charwin.h/c`): windows,
  console mode with scrolling and word-wrap (`cwin_printwrap`),
  positional/cursor writes, double-height text, rectangle copy, a text-input
  widget (`cwin_textinput`), and overlay-RAM window save/restore
  (`cwin_push`/`cwin_pop`).
- New direct **VIA/AY keyboard scanner** (`include/keyboard.h/c`) — no ROM
  calls, with key-repeat and release-debounce handling.
- New **LOCI hardware API library** (`include/loci.h/c`) covering the
  MIA/TAP registers, XRAM and overlay RAM, and file/directory/mount/tape
  operations.
- **IJK joystick driver** split into its own independent library
  (`include/ijk.h/c`).
- **libdemo** — a full exerciser for charwin/keyboard/LOCI/IJK (lettered
  sections A–Q), used for manual smoke-testing and as the basis of automated
  library tests.
- New **headless automated test harness** via the
  [Phosphoric](https://github.com/xahmol/Phosphoric) emulator
  (`--loci-flash` sandbox, `--type-keys` auto-typing,
  `--dump-ram-at`/`tests/scripts/oric_screen.py` screen capture) — 8 suites
  (quick boot, menus, file ops, libdemo, recursive ops, name filter,
  copy-cancel, viewer) totalling 234 assertions, run via `make test`.
  v1 had no automated tests.

### 2. New file-management features

v1 could browse, copy single files (not directories), delete/rename files or
empty directories, create directories, mount/unmount images, and browse
inside tape images. v2 adds:

- **Recursive directory copy/move** — copies or moves a directory's full
  subtree (files and nested subdirectories), merging into an existing
  destination directory if one is present.
- **Recursive directory delete** — deleting a non-empty directory now asks
  "Directory not empty. Delete ALL contents?" and, if confirmed, removes the
  entire tree.
- **Mid-copy cancellation** — pressing ESC during a copy or move aborts
  immediately and removes the partially-written destination file.
- **Wildcard filename filter** (`*`/`?`, case-insensitive, Tools menu / `L`
  key) — in addition to v1's by-type filter; applies to both panes,
  directories are always shown so navigation is never blocked.
- **Full-screen text file viewer** (Tools menu / `J` key) — word-wrapped,
  paged viewer for any non-directory entry (forward-only paging in this
  version).
- **Properties popup** (Tools menu / `K` key) — shows name, type, path,
  attributes (read-only/system), and size; for directories the size is
  calculated recursively, with a "Calculating…" / ESC-cancel flow and a `+`
  suffix if the tree is deeper than the walk's 8-level limit.
- A new 6th menu-bar item, **Tools**, hosts the filter/viewer/properties
  features alongside their direct hotkeys.
- Shared infrastructure: an iterative (non-recursive, stack-safe) directory
  walker (`src/recurse.c/h`, `recurse_walk()`, max depth 8) underlies
  recursive copy/move, recursive delete, and the properties size
  calculation — needed because the software stack is only 512 bytes and
  shared by the whole call chain.

### 3. Localisation

- v1 was English-only. v2 builds **English and French** binaries from one
  codebase (`make`, `make LANG=FR`, `make all-langs`), with every
  user-visible string as an `MSG_*` macro in `src/strings_en.h` /
  `src/strings_fr.h`.

### 4. Branding & presentation

- New versioned splash screens (startup and Info menu): the "I Dream in 8
  Bits" logo with build version (`vMAJOR.MINOR.PATCH-YYYYMMDD-HHMMSS`) and
  credits, followed by a GitHub QR-code screen.

### 5. Build, testing & documentation infrastructure

- `make docs` — regenerates the README and library-manual PDFs via pandoc
  (EN+FR).
- `make zip` — packages all `.tap` images and docs into a release archive.
- `make usb` — copies built images to a USB stick path from `.env`,
  including WSL2 auto-mount/unmount of Windows drive letters.
- New project documentation: `ARCHITECTURE.md` (memory map, data structures,
  module layout, control flow), `CONTRIBUTING.md` (dev setup, coding/testing/
  documentation conventions, PR checklist), and `libmanual.md` /
  `libmanual_fr.md` (full API reference for charwin/LOCI/IJK/keyboard).

---

## Planned / Future Work

_No active plans at this time._

### Adding a new plan

When a new development phase or substantial feature is designed, add a
section below this heading using whichever of these fits the size of the
work:

**Small/contained feature** — a short entry is enough:

```markdown
## <Feature name>

### Context
What it is and why it's wanted, in 1-3 sentences.

### Plan
Files/functions to touch and the approach, in enough detail to start work.

### Test scenario
What `make test-<name>` (new or extended) will check.

### Progress
- [ ] Implemented
- [ ] Tests pass
- [ ] Docs updated (README/ARCHITECTURE/CLAUDE.md as relevant)
```

**Large, multi-session effort** — break it into numbered phases, each with
its own Context/Plan/Test scenario/Progress, plus a top-level "Hard
constraints" section for cross-cutting design decisions discovered during
research (the kind of structure used for the recursive-ops/filter/
viewer/properties work above is the model — see git history for that
document in full).

Once a plan's items are all complete, move its summary into the
[Completed: v1 to v2](#completed-v1-to-v2) section above (or a future
"Completed: vX → vY" section if a new major version line has started), and
remove the detailed plan from this section.
