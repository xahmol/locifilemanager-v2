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
  `--dump-ram-at`/`tests/scripts/oric_screen.py` screen capture) — 9 suites
  (quick boot, menus, file ops, libdemo, recursive ops, name filter,
  copy-cancel, viewer, persistent settings) totalling 257 assertions, run via
  `make test`. v1 had no automated tests.
- App settings (`confirm`/`filter`/`enterchoice`/`sort`) consolidated from 4
  standalone globals into one `struct AppSettings settings` (`src/dir.h`/
  `src/dir.c`), used throughout `main.c`/`dir.c`/`file.c`.
- **Persistent settings** (`src/dir.c` `config_load()`/`config_save()`,
  `struct FmConfig`) — `settings` is saved to `0:/idi8b/locifm/locifm.cfg`
  (directory and file created on first run) and loaded at startup, with a
  magic byte to detect and recover from a missing or corrupt file. Every
  change to sort/confirm/filter/enterchoice — via direct hotkeys or the App
  pulldown menu — immediately writes the updated config, so settings persist
  across power cycles with no explicit "save" step. Confirmed on real LOCI
  hardware: the boot-time load/fallback-save round trip and repeated runtime
  saves both work without issue. (An earlier flat-file attempt,
  `0:/LOCIFM.CFG`, wedged the LOCI firmware on writes and was removed — see
  git history; the nested `0:/idi8b/locifm/` path does not have this
  problem.)

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
  version). Non-printable bytes are shown as `.` placeholders, so binary
  files display safely. Pressing `X` at any pause point toggles to a hex
  dump of the same file (offset, hex bytes, ASCII column) and back,
  restarting the file from the beginning in the new view.
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

Two contained features are planned below, building on the persistent-settings
mechanism completed above. See [Adding a new plan](#adding-a-new-plan) for the
format used for these and any future entries.

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

---

## Favourite directories

### Context

Users navigate to deeply-nested directories repeatedly. A bounded, global
list of bookmarked directory paths lets either pane jump straight to a
favourite, persisted via the existing `config_save()`/`config_load()`
mechanism (`0:/idi8b/locifm/locifm.cfg`).

### Plan

- `src/dir.h`: add `FMCONFIG_FAV_SLOTS=8`, `FMCONFIG_FAV_PATHLEN=48`, and
  `extern char favourites[FMCONFIG_FAV_SLOTS][FMCONFIG_FAV_PATHLEN]` (defined
  in `dir.c` next to `settings`; empty string = unused slot). Add the same
  array to `struct FmConfig` (+384 bytes, total 389 bytes). The existing
  `file_load(...) != sizeof(cfg)` size check in `config_load()` already gives
  old (5-byte) config files a safe fallback to compiled-in defaults — no
  magic-byte bump required.
- `src/dir.c` `config_save()`/`config_load()` (~lines 1224/1247): loop over
  `FMCONFIG_FAV_SLOTS`, `strncpy` each slot to/from `cfg.favourites[i]`,
  NUL-terminating on load.
- New functions in `dir.c` (prototypes in `dir.h`): `favourites_add(slot)`
  (copies `presentdir[activepane].path` into `favourites[slot]`, calls
  `config_save()`), `favourites_delete(slot)` (zeroes the slot, calls
  `config_save()`), `favourites_goto(slot)` (sets
  `presentdir[activepane].path`/`.drive` from the slot, clears
  `insidetape[activepane]`, calls `dir_draw(activepane, 1)`), and
  `favourites_show(void)` — the popup entry point, modeled on
  `dir_show_properties()` (`dir.c:1481`).
- UI: a 4th Tools-menu item `MSG_MENU_TOOLS3` ("[Y] Favourites"),
  `pulldown_options[4]` 3→4 (`menu.c:205`, within `PULLDOWN_MAXOPTIONS=9`).
  New global hotkey `'y'` (unused in `main.c`'s key-loop switch) plus a
  matching `mainmenuloop()` dispatch case (`54`), both calling
  `favourites_show()`.
- `favourites_show()`: `menu_popup_open()` + `OricCharWin`, lists all 8 slots
  (`"N: path"` or `"(empty)"`). `cwin_getch()` loop: digits `1`-`8` jump via
  `favourites_goto()` (ignored if the slot is empty), `'a'`/`'A'` prompts for
  a slot then `favourites_add()`, `'d'`/`'D'` prompts for a slot then
  `favourites_delete()`, `KEY_ESC` closes via `menu_popup_close()`.
- New `MSG_*` macros (both `strings_en.h`/`strings_fr.h`): `MSG_MENU_TOOLS3`,
  `MSG_FAV_TITLE`, `MSG_FAV_SLOT_FMT`, `MSG_FAV_EMPTY`, `MSG_FAV_HELP`,
  `MSG_FAV_ADD_PROMPT`, `MSG_FAV_DELETE_PROMPT`.

### Test scenario

New `tests/scripts/test_favourites.sh` / `make test-favourites` (wired into
the aggregate `test:` target):

1. Fresh boot, open Favourites (`y`, then ESC) → screen shows the title and 8
   "(empty)" rows; `locifm.cfg` is 389 bytes, favourites region all zero.
2. Boot, navigate into a subdirectory, open Favourites, `a` then `1` → slot 1
   redraws with the current path; `locifm.cfg` bytes for slot 1 match the
   path + NUL padding.
3. Boot with a pre-seeded `locifm.cfg` containing one populated slot → open
   Favourites → slot shows the seeded path → press `1` → active pane's
   path/listing updates to that directory.
4. Delete a populated slot (`d` then slot number) → slot shows "(empty)"
   again; `locifm.cfg` bytes for that slot are zeroed, rest unchanged.

### Progress

- [x] Implemented
- [x] Tests pass (`make test-favourites`, 19/19; `make test` 276/276)
- [x] Docs updated (README.md "Tools: ... favourites" section + keyboard
      table, in-app Help (`H`) screen)

---

## Remember last path/pane state

### Context

Every boot currently starts both panes at the drive root (`0:/`). Persisting
each pane's current path/drive and the active pane lets the app reopen
exactly where the user left off, building on the now-confirmed
`config_save()`/`config_load()` mechanism. This is **automatic** — saved on
every navigation, not via an explicit "remember" action — a deliberate choice
accepting higher write frequency than the current toggle-only saves.

### Plan

- `src/dir.h`: add to `struct FmConfig`: `char lastpath[2][FMCONFIG_FAV_PATHLEN]`,
  `uint8_t lastdrive[2]`, `uint8_t lastactivepane` (+99 bytes; 488 bytes total
  combined with Favourites above — reuses `FMCONFIG_FAV_PATHLEN`, same
  size-check fallback applies, no magic bump required).
- `src/dir.c` `config_save()`: `strncpy` `presentdir[0/1].path` into
  `cfg.lastpath[0/1]` (NUL-terminate at `FMCONFIG_FAV_PATHLEN-1`), copy
  `presentdir[0/1].drive` into `cfg.lastdrive[0/1]`, copy `activepane` into
  `cfg.lastactivepane`.
- `src/dir.c` `config_load()`: reverse — restore `presentdir[0/1].path`/
  `.drive` and `activepane` (clamp `lastactivepane` to 0/1).
- `src/main.c` boot sequence: move the `presentdir[0/1].path = "0:/"` /
  `.drive = 0` defaults into the existing "Reset application state" block
  (~line 412), i.e. *before* `config_load()` (line 431) — so a fresh/fallback
  config saves and reloads these defaults correctly, while an existing config
  overwrites them with the restored values. Replace the current
  post-`config_load()` block (lines 471-475: `strcpy(presentdir[0].path,
  "0:/")`, the two drive resets, and `dir_get_next_drive(1)`) with just
  `dir_draw(0, 1); dir_draw(1, 1);`. **Decide during implementation**: should
  a brand-new config still default pane 2 to drive `1:` for the "out of the
  box" feel like today (a one-line special case, not a blocker either way)?
- New `config_save()` call sites (one extra line after the existing
  `dir_draw(...)` call in each): `dir_gotoroot()` (`dir.c:1170`),
  `dir_parentdir()` — both the inside-tape branch (`dir.c:1182`) and the
  normal path-trim branch (`dir.c:1200`), `dir_get_next_drive()`
  (`dir.c:834`), `dir_get_prev_drive()` (`dir.c:864`), `dir_switch_pane()`
  (`dir.c:876`, persists `lastactivepane`), and the ENTER-key directory
  descent in `main.c` (`main.c:495`).
- No new `MSG_*` macros — fully automatic/silent, no new UI.

### Test scenario

New `tests/scripts/test_laststate.sh` / `make test-laststate` (wired into the
aggregate `test:` target):

1. Fresh boot, navigate pane 0 into a subdirectory (one ENTER keypress), then
   re-launch the emulator from the *same* sandbox (no `reset_sandbox`) →
   second boot's screen shows the subdirectory's path/listing in pane 0, not
   `0:/`.
2. Switch the active pane and change pane 1's drive, re-launch → second boot
   restores pane 1's drive and `activepane`.
3. `od -An -tx1` on `locifm.cfg` after step 1's first run confirms
   `lastpath[0]` bytes match the subdirectory path + NUL padding, confirming
   the write happens on navigation itself.
4. Update `test_config.sh`'s `DEFAULT_BYTES`/expected-byte constants for the
   new (larger) `FmConfig` layout so its existing assertions still pass.

### Progress

- [x] Implemented
- [x] Tests pass (`make test-laststate`, 8/8; `make test` 284/284)
- [x] Real-HW verification of per-navigation `config_save()` write frequency
      — confirmed working on real LOCI hardware (2026-06-14): the increased
      write frequency (every directory/drive change, not just toggles) does
      not wedge or cause issues
- [x] Docs updated (no user-visible UI/MSG_* changes -- feature is fully
      automatic/silent, no README/Help updates needed)

---

## Backlog: other persistence ideas (raise with user first)

- **Persist `namefilter[32]`** — the active wildcard filter resets every
  boot; a 32-byte `FmConfig` addition plus a `config_save()` hook in the
  name-filter handler would be straightforward, but a filter silently
  surviving reboot could surprise a user who forgets it's set — confirm the
  desired UX first.
- **Favourites with custom labels** — the planned design above shows the raw
  path per slot; per-favourite labels would need `char label[N]` per slot
  (≈+128 bytes for 8×16), only worth it if raw-path display proves
  insufficient in practice.
- **Persist `targetdrive`** (mount target A-D) — resets to drive A on boot
  today; a 1-byte addition like the existing scalar settings, but low value
  unless reported as friction.
- **Per-pane sort/filter overrides** — `settings.sort`/`settings.filter` are
  currently global across both panes; would need `[2]` arrays if/when
  per-pane settings are ever introduced.
