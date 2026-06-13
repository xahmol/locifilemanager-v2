# LOCI File Manager v2 — Implement Priorities 1–7 (Phased Plan)

## Context

Following the earlier feature-suggestion analysis, the user asked to implement
the top 7 prioritized features for LOCI File Manager v2
(`/home/xahmol/git/locifilemanager-v2`), in this order:

1. Recursive directory copy/move
2. Recursive directory delete
3. Persistent settings
4. Filename search/filter by name (wildcard)
5. Mid-file copy cancellation
6. Text file viewer
7. Properties popup + recursive directory size

This is a substantial multi-session effort touching `dir.c`, `file.c`,
`main.c`, `menu.c`, `include/loci.c`, and the Makefile/test harness. The plan
below is organized as **8 phases (0–7)**: Phase 0 builds shared
infrastructure (a recursive directory-walk helper + menu/keyboard scaffolding
+ test fixtures) used by multiple later phases; Phases 1–7 implement the
features in the user's requested order. **Every phase ends with extending
`tests/scripts/` and running `make test` before moving on** — no phase is
considered done until its tests pass.

---

## Hard constraints established during research

- **Stack is only 512 bytes** (`#pragma stacksize(0x0200)`, `oric_crt.c`),
  shared by the whole call chain. → The directory-walk helper **must be
  iterative** (one function, one `while` loop, explicit stack array), not
  C-level recursive — avoids any risk of stack overflow at depth 8.
- **`malloc` currently returns NULL** in this project's custom runtime
  (`crt_malloc` stub, `include/crt_math.c:1747-1773`) — this is a
  configuration choice of `include/oric_crt.c` (no `heap` region /
  `#pragma heapsize()`), not an Oscar64-wide limitation; a real heap could be
  enabled later if needed. For this plan, every new buffer (recurse walk
  state ~280 bytes, viewer chunk buffer 4KB, settings struct, name-filter
  pattern) is small and fixed-size, so **static/global arrays** remain the
  simplest choice — ~21.6KB free static RAM (`build/locifm.map`) is more than
  enough.
- **`cwin_printf`/`_cwin_vformat` has no `%lu`/uint32 support** (`%d/%u/%x`
  only, `include/charwin.c:169`). Directory recursive sizes (uint32) need a
  manual decimal formatter — model on the existing `dir_dec6()`
  (`src/dir.c:110`, formats uint16 as 6-char padded decimal); add a
  `dir_dec10()` uint32 variant using the same divide-by-10 loop (10 digits
  max for `0xFFFFFFFF`).
- **Menu capacity constants are adjustable** — `MENUBAR_MAXOPTIONS`,
  `PULLDOWN_NUMBER`, `PULLDOWN_MAXOPTIONS` (`src/menu.h:33-37`) are plain
  `#define`s; `menu_placebar()`/`menu_main()` (`src/menu.c:253-275, 432-498`)
  already loop on `MENUBAR_MAXOPTIONS` generically, so raising it is
  mechanical (limited only by screen width / popup height, not by code
  structure). Currently `MENUBAR_MAXOPTIONS=5` (App/File/Dir/Mounts/Info) and
  `PULLDOWN_NUMBER=10` (indices 0-9 all used). Phase 0 adds a **6th bar item
  ("Tools")** rather than cramming new items into the Dir pulldown's 2 free
  slots — see Phase 0 below. **Implementation note:** `menu_main()` calls
  `menu_pulldown(..., menubarchoice - 1, ...)`, so `pulldown_titles`/
  `pulldown_options` indices 0-5 must be the 6 top-bar pulldowns in bar
  order. Tools therefore occupies **pulldown index 5** (not 10 as the wording
  below might suggest), with the 5 existing popup submenus (Enter-action,
  Filter, Target drive, Mount source, Yes/No) shifted from indices 5-9 to
  6-10 and `MENU_YESNO` updated 9→10. The menu **codes** `61/62/63` for the
  Tools items are unaffected (`menubarchoice(6) * 10 + menuoptionchoice`).
- **Free keyboard letters**: `j, k, l, q, x, y, z` (confirmed against the
  full set used in `src/main.c`'s main loop: s,a,n,i,o,c,v,f,g,r,m,u,w,e,h,
  d,p,t,b plus `.`,`,`,`/`,`\`,DEL,ENTER,ESC,arrows — `i` is taken by inverse-select).
  `KEY_F1`-`F10` require the Atmos FUNCT key and are **avoided** — unverified
  whether Phosphoric's `--type-keys` can send FUNCT combos, so all new
  shortcuts use plain letters.
- **Help table** (`helpinfo[22][2]`, `src/main.c:35`, loops at lines 140/149
  bounded to 20/22) is **full** — must grow to `[25][2]` with adjusted loop
  bounds to document 3 new shortcuts.

---

## Phase 0 — Foundation: recursive walk helper + menu/keyboard scaffolding + fixtures

### New module: `src/recurse.h` / `src/recurse.c`

Iterative depth-first directory walker, shared by Phases 1, 2, and 7.

```
#define RECURSE_MAX_DEPTH 8

typedef enum { RECURSE_ENTER_DIR, RECURSE_FILE, RECURSE_LEAVE_DIR } RecurseEvent;
#define RECURSE_CONTINUE 0
#define RECURSE_ABORT    1

// entry is NULL for RECURSE_LEAVE_DIR (only fullpath is meaningful there)
typedef int8_t (*RecurseCallback)(RecurseEvent ev, const LociDirent *entry,
                                   const char *fullpath, void *userdata);

extern uint8_t recurse_truncated;  // set if RECURSE_MAX_DEPTH reached and a subtree was skipped

// Walks the CONTENTS of rootpath (rootpath itself is not reported).
// Returns 0 = completed, RECURSE_ABORT = cb requested stop, -1 = opendir error.
int8_t recurse_walk(const char *rootpath, RecurseCallback cb, void *userdata);
```

**Implementation** (single function, no C recursion):
- Static state: `static RecurseFrame stack[RECURSE_MAX_DEPTH]` where
  `RecurseFrame = { LociDir *dir; uint8_t pathlen; }` (~24 bytes total), plus
  a single shared `static char walkpath[256]`.
- `depth` index into `stack[]`. Push: `loci_opendir`, record `pathlen =
  strlen(walkpath)`. On each `loci_readdir`:
  - End-of-dir (`d_name[0]==0`): `loci_closedir`, truncate `walkpath` to
    `stack[depth].pathlen`; if `depth==0` return `RECURSE_CONTINUE`; else
    `depth--`, call `cb(RECURSE_LEAVE_DIR, NULL, walkpath, userdata)`,
    truncate `walkpath` to `stack[depth].pathlen`.
  - Directory entry (`d_attrib & DIR_ATTR_DIR`): append name to `walkpath` via
    `dir_build_path(walkpath, sizeof(walkpath), walkpath, file->d_name)`
    (in-place truncate+append, safe per existing helper); call
    `cb(RECURSE_ENTER_DIR, file, walkpath, userdata)` (check abort); if
    `depth+1 < RECURSE_MAX_DEPTH`, `loci_opendir(walkpath)` and push;
    else set `recurse_truncated=1`, immediately call
    `cb(RECURSE_LEAVE_DIR, NULL, walkpath, userdata)` and truncate back.
  - File entry: append name to `walkpath`, call
    `cb(RECURSE_FILE, file, walkpath, userdata)` (check abort), truncate back.
- Any `RECURSE_ABORT` from `cb` closes all open `LociDir*` in `stack[0..depth]`
  and returns `RECURSE_ABORT` immediately.

### Menu/keyboard scaffolding (decided now, filled in later phases)

New 6th menu bar item **"Tools"** (bar position 6, pulldown index 5 — see
implementation note above), giving menu codes 61-69. Each item also gets a
single-letter hotkey shown via the `[X]` bracket-hint convention:

| New feature | Tools menu item (case) | Hotkey | Help table |
|---|---|---|---|
| Properties popup (Phase 7) | `MSG_MENU_TOOLS0` = `"[K] Properties"` (case 61) | `k` | new row |
| Name-filter wildcard (Phase 4) | `MSG_MENU_TOOLS1` = `"Fi[l]ter by name"` (case 62) | `l` | new row |
| Text viewer (Phase 6) | `MSG_MENU_TOOLS2` = `"[J] View text file"` (case 63) | `j` | new row |

Changes in this phase:
- `src/menu.h:33,35` — `MENUBAR_MAXOPTIONS` 5 → 6, `PULLDOWN_NUMBER` 10 → 11.
- `src/menu.c:194-195` — add `MSG_MENU_BAR_TOOLS` to the bar-titles array.
- `src/menu.c:200` — extend `pulldown_options[PULLDOWN_NUMBER]` with a new
  entry (e.g. `3`, room to grow up to `PULLDOWN_MAXOPTIONS=9` for future tools).
- `src/menu.c:202` — extend `pulldown_titles[][][]` with a new row:
  `{ MSG_MENU_TOOLS0, MSG_MENU_TOOLS1, MSG_MENU_TOOLS2 }`.
- `src/strings_en.h` / `src/strings_fr.h` — add `MSG_MENU_BAR_TOOLS` ("Tools"
  / "Outils"), `MSG_MENU_TOOLS0..2`; grow `MSG_HELP_TABLE`/`helpinfo` from
  `[22][2]` to `[25][2]` with 3 placeholder rows ("K","Properties (TBD)"),
  ("L","Filter by name (TBD)"), ("J","View text file (TBD)").
- `src/main.c` — adjust `help()` loop bounds (`src/main.c:140,149`, 20/22 →
  22/25); add `case 61:`, `case 62:`, `case 63:` to `mainmenuloop()`'s switch
  and `case 'k':`, `case 'l':`, `case 'j':` to the main loop's switch, all as
  `menu_messagepopup("TODO")`-style stubs.
- `Makefile` — add `src/recurse.h`/`src/recurse.c` to `MAIN_SRCS`.

`menu_placebar()`/`menu_main()` (`src/menu.c:253-275, 432-498`) already loop
generically on `MENUBAR_MAXOPTIONS`, so the 6th bar item and its pulldown
should work without further structural changes — verify the bar still fits
within 38 columns (current 5 titles ≈ 20 chars; "Tools" adds ~5-6 more,
comfortably under budget) and that `menu_pulldown()`'s popup sizing
(`src/menu.c:557` comment re: `PULLDOWN_MAXOPTIONS=9` rows) renders correctly
for a 3-item pulldown.

### Test fixtures

`tests/fixtures/DEEP/` — 3-level nested tree for recursive-op tests:
- `DEEP/FILEA.TXT`, `DEEP/SUB1/FILEB.TXT`, `DEEP/SUB1/SUB2/FILEC.TXT`
- `DEEP/EMPTY/` (empty subdir, regression check for plain empty-dir delete)
- Give each file a **fixed, known size** (e.g. exactly 100/200/300 bytes) so
  Phase 7's recursive-size test can assert an exact total (600).

### Test scenario (new Scenario 5: `tests/scripts/test_recurse.sh` + `make test-recurse`)

Phase 0 assertions (structural only — extended in later phases):
1. `check_host "build succeeded" "[ -f build/locifm.tap ]"`
2. Boot smoke: `check_found "main interface intact" "LOCI File Manager"`
3. Open the new Tools pulldown (6th bar item), `check_found "Properties item visible"`,
   `check_found "Filter by name item visible"`, `check_found "View text file item visible"`
4. `check_host "DEEP fixture tree present" "[ -d '$SANDBOX/DEEP/SUB1/SUB2' ] && [ -f '$SANDBOX/DEEP/FILEA.TXT' ]"`

Add `test-recurse` to the `test:` aggregate target and `.PHONY`.

**Done when:** `make test` (incl. `test-recurse`) passes; new Dir items and
stub keys render/respond without corruption; `DEEP/` fixture present.

---

## Phase 1 — Recursive directory copy/move

**File:** `src/file.c`, function `file_copy_move_selected(uint8_t move)`
(currently lines ~16-142).

- In the per-element loop, if `presentdirelement.meta.type == 1` (directory),
  call new static helper `file_copy_move_dir(srcpath, dstpath, move)` instead
  of the existing single-file copy logic:
  1. `loci_mkdir(dstpath)` — if it already exists, proceed (merge semantics).
  2. `recurse_walk(srcpath, file_copy_move_cb, &ctx)` where `ctx` holds
     `srcroot`, `dstroot`, `move` flag, and the progress `OricCharWin`.
  3. `file_copy_move_cb`:
     - `RECURSE_ENTER_DIR`: build corresponding dest subdir path (replace
       `srcroot` prefix with `dstroot`), `loci_mkdir()`.
     - `RECURSE_FILE`: build dest file path the same way; reuse existing
       overwrite-confirm logic (`confirm`/`menu_confirm_file`,
       `src/file.c:77-91`); call `file_copy_progress(dst, src, 2, 14, 32)`
       (same coords as today). Check `keyb_check()==KEY_ESC` → `RECURSE_ABORT`.
     - `RECURSE_LEAVE_DIR`: if `move`, `loci_unlink(srcsubdir)` (now empty);
       non-fatal if it fails.
  4. After `recurse_walk`, if `move` and not aborted/truncated,
     `loci_unlink(srcpath)` (remove now-empty source root, mirrors existing
     single-file move at `src/file.c:106`).
  5. If `recurse_truncated`, show new `MSG_FILE_RECURSE_TRUNCATED`.
- Path-length guard: before each `dir_build_path` inside the callback, check
  resulting length against `sizeof(pathbuffer)` (256); on overflow, skip that
  one entry with `MSG_FILE_PATH_TOO_LONG` (existing message, existing pattern
  at `src/file.c:67-71`) rather than aborting the whole walk.

**New strings:** `MSG_FILE_RECURSE_TRUNCATED`, `MSG_FILE_MKDIR_FAILED` (EN+FR).

### Test scenario (extend `test_recurse.sh`)

1. **Recursive copy**: select `DEEP/` in pane 0, switch pane 1 to `SUBDIR/`,
   press `c`. Assert (`check_host`): `SUBDIR/DEEP/FILEA.TXT`,
   `SUBDIR/DEEP/SUB1/FILEB.TXT`, `SUBDIR/DEEP/SUB1/SUB2/FILEC.TXT`,
   `SUBDIR/DEEP/EMPTY/` all exist; original `DEEP/FILEA.TXT` still exists.
2. **Recursive move** (fresh `reset_sandbox`): same setup, press `v`. Same
   destination assertions, plus `check_host "original DEEP removed" "[ ! -e '$SANDBOX/DEEP' ]"`.
3. Both: `check_found "LOCI File Manager"`, `check_not_found "File error"`.
4. Calibrate `--type-keys` cycle counts via `make test-capture` (cursor-down
   counts for selecting `DEEP/` among the fixture entries).

**Done when:** `make test-recurse` passes with full-tree copy and move both
verified on the host filesystem.

### Implementation notes (deviations/discoveries during Phase 1)

1. **Directory selection was architecturally unreachable.** `dir_select_toggle`,
   `dir_select_all`, `dir_select_inverse`, and `file_copy_move_selected`'s
   original single-cursor auto-select all gated on `presentdirelement.meta.type
   > 1`, excluding directories (`type==1`) from ever getting
   `meta.select=1` — so the new `if (presentdirelement.meta.type == 1)`
   recursive-copy branch could never execute via the UI. Fixed minimally by
   relaxing ONLY `file_copy_move_selected`'s auto-select condition to
   `if (!selection[activepane])` (no type restriction); the select-toggle/
   select-all/inverse functions and `file_delete()`'s auto-select are
   unchanged (Phase 2's `dir_deletedir()` is a separate cursor-based path that
   doesn't need this).
2. **`recurse_walk()` trailing-slash fix** (`src/recurse.c`): `loci_readdir()`
   returns directory names WITHOUT a trailing `/` (only `dir_read()` appends
   one for its own display purposes). `recurse_walk` now appends `/` to
   `walkpath` for `DIR_ATTR_DIR` entries before calling
   `cb(RECURSE_ENTER_DIR, ...)` and `loci_opendir()`, so child paths built via
   `dir_build_path(walkpath, sizeof(walkpath), walkpath, childname)` get the
   correct separator.
3. **Phosphoric emulator bug found and fixed**: `loci_unlink()` on an empty
   directory failed in the `--loci-flash` sandbox, because Phosphoric's
   `op_unlink` (hostfs backend, `src/io/loci_fs.c`) called bare `unlink()`,
   which returns `EISDIR` for directories on Linux. Real LOCI firmware's
   UNLINK (FatFs `f_unlink()`) removes both files AND empty directories.
   Fixed in the local `Phosphoric` checkout
   (`/home/xahmol/git/Phosphoric/src/io/loci_fs.c`, `op_unlink`): if the
   resolved host path is a directory, call `rmdir()` instead of `unlink()`
   (added `ENOTEMPTY -> LOCI_EACCES` to `map_errno`). Rebuilt with
   `SDL2=1 make`. **Without this fix, recursive move (and Phase 2's recursive
   delete) cannot remove now-empty subdirectories or the source root** — this
   directly resolves Open Risk #1 below (drive-0 unlink-on-directory now
   succeeds for empty dirs, fails with `LOCI_EACCES`/errno 3 for non-empty
   ones, matching FatFs `f_unlink` semantics). The sdimg (`.dsk`-image)
   backend's `loci_sdimg_unlink()` still unconditionally returns `-EISDIR` for
   directory entries and was NOT fixed (out of scope — the test sandbox uses
   `--loci-flash`, not a `.dsk` image); revisit if a future test mounts a
   `.dsk` and needs directory removal on it.

---

## Phase 2 — Recursive directory delete

**File:** `src/dir.c`, function `dir_deletedir()` (lines 1201-1243).

Current behavior (verified): for drive≠0, if `loci_opendir`+`loci_readdir`
finds any entry, shows `MSG_DIR_NOT_EMPTY` and returns — no deletion. For
drive 0, skips the check (drive 0's `opendir` returns no entries) and calls
`loci_unlink(pathbuffer)` directly.

New behavior:
1. Where the non-empty check currently shows `MSG_DIR_NOT_EMPTY` and returns,
   instead show `menu_confirm_file(MSG_DIR_DELETE_RECURSE_Q, presentdirelement.name)`
   ("Directory not empty. Delete ALL contents?"). If declined, return as before.
2. If confirmed (or dir was empty — existing fast path), call new
   `dir_delete_recursive(pathbuffer)`:
   - `recurse_walk(pathbuffer, dir_delete_cb, NULL)`:
     - `RECURSE_FILE`: `loci_unlink(fullpath)`; on error → `RECURSE_ABORT`.
     - `RECURSE_ENTER_DIR`: no-op.
     - `RECURSE_LEAVE_DIR`: `loci_unlink(fullpath)` (now-empty subdir).
     - Show a simple "Deleting: <name>" popup line (model on
       `file_delete()`'s popup, `src/file.c:154-165`); poll
       `keyb_check()==KEY_ESC` → `RECURSE_ABORT`.
   - On success (not aborted/truncated), `loci_unlink(pathbuffer)` removes the
     root dir itself (same final call as today, line 1234).
   - If `recurse_truncated`, show `MSG_FILE_RECURSE_TRUNCATED` and do **not**
     delete the root (subtree incomplete).
3. Drive-0 path: keep existing direct `loci_unlink` as the fast path; if it
   fails, fall back to `dir_delete_recursive()` (covers the case where drive 0
   *can* contain non-empty dirs and `loci_unlink` errors accordingly — verify
   actual LOCI errno for "not empty" during implementation via
   `make test-capture` on a non-empty drive-0 dir).

**Design decision:** only ONE top-level "delete all?" confirm — no per-file
confirms during the walk (keeps it testable with a fixed key sequence and
matches copy/move's single-ESC-to-cancel model).

**New strings:** `MSG_DIR_DELETE_RECURSE_Q` ("Dir not empty. Delete ALL?").

### Test scenario (extend `test_recurse.sh`, fresh `reset_sandbox`)

1. Select `DEEP/`, press DEL, confirm "delete all?" with Yes. Assert
   `check_host "DEEP fully removed" "[ ! -e '$SANDBOX/DEEP' ]"`,
   `check_found "LOCI File Manager"`, `check_not_found "File error"`.
2. Regression: select `DEEP/EMPTY/` alone, delete, assert
   `[ ! -d '$SANDBOX/DEEP/EMPTY' ]` and `[ -d '$SANDBOX/DEEP' ]` (parent intact;
   plain empty-dir delete still works as before).

**Done when:** non-empty `DEEP/` is fully removed with a single confirm; plain
empty-dir delete is unaffected.

### Implementation notes (deviations/discoveries during Phase 2)

1. **`dir_deletedir()` rewrite** (`src/dir.c`): split into the original
   `dir_deletedir()` plus two new static helpers, `dir_delete_cb()`
   (`recurse_walk` callback — `loci_unlink()`s files on `RECURSE_FILE` and
   now-empty subdirs on `RECURSE_LEAVE_DIR`, shows a "Deleting: <name>" popup
   line via `cwin_putat_string`, polls `keyb_check()==KEY_ESC` to abort) and
   `dir_delete_recursive()` (opens the popup, runs `recurse_walk`, then
   `loci_unlink()`s the now-empty root; handles `recurse_truncated` via
   `MSG_DIR_RECURSE_TRUNCATED`). For drive≠0, the existing
   `loci_opendir`/`loci_readdir` pre-check now shows
   `MSG_DIR_DELETE_RECURSE_Q` (instead of the old `MSG_DIR_NOT_EMPTY`
   dead-end) when the directory is non-empty. For drive 0 (no pre-check
   possible), the existing direct `loci_unlink(pathbuffer)` is tried first;
   if it fails with `loci_errno == LOCI_EACCES`, the same
   `MSG_DIR_DELETE_RECURSE_Q` confirm + `dir_delete_recursive()` fallback
   runs — this is the actual `LOCI_EACCES` (3, `ENOTEMPTY`) value confirmed
   during Phase 1's Phosphoric `op_unlink`/`map_errno` fix.
2. **New `LOCI_EACCES` constant** added to `include/loci.h` (value 3,
   documented as the Phosphoric-mapped `ENOTEMPTY`/FatFs "not empty" code —
   revisit against real LOCI firmware if it differs). `MSG_DIR_NOT_EMPTY`
   (now unused, superseded by `MSG_DIR_DELETE_RECURSE_Q`) was removed from
   both `strings_en.h` and `strings_fr.h`.
3. **Test-fixture/sandbox bug found and fixed** (NOT an application bug):
   sub-test 2 (plain `DEEP/EMPTY/` delete) initially failed —
   `loci_unlink("0:/DEEP/EMPTY/")` returned `LOCI_EACCES` even though the
   directory appeared empty in the app's listing. Root cause:
   `tests/fixtures/DEEP/EMPTY/.gitkeep` (the placeholder needed for git to
   track an otherwise-empty directory) was being copied into
   `tests/sandbox/DEEP/EMPTY/` by `reset_sandbox`/`sandbox-reset`, making the
   directory genuinely non-empty on the host filesystem — `dir_read()` hides
   dotfiles from the listing, so the UI showed it as empty while `rmdir()`
   correctly refused. `dir_deletedir()`'s `LOCI_EACCES` → "Delete ALL?"
   fallback was working exactly as designed for that (test-only) state.
   Fixed by adding `find tests/sandbox -name '.gitkeep' -delete` to both the
   Makefile's `sandbox-reset` target and `test_recurse.sh`'s
   `reset_sandbox()`, after the `cp -r tests/fixtures/.` step.
4. `make test-recurse` now passes 31/31 (8 new Phase 2 assertions); full
   `make test` passes 182/182 (11+45+15+80+31). Both EN and FR builds clean.

---

## Phase 3 — Persistent settings

**New struct + functions in `src/dir.c`/`src/dir.h`** (where `confirm`,
`filter`, `enterchoice`, `sort` already live as module globals,
`src/dir.c:41-44`):

```
struct FmConfig { uint8_t magic; uint8_t confirm; uint8_t filter;
                   uint8_t enterchoice; uint8_t sort; };
#define FMCONFIG_MAGIC 0xA5
```

- `config_load(void)` — `file_load("0:/LOCIFM.CFG", &cfg, sizeof(cfg))`
  (`include/loci.h:316`); if load fails or `cfg.magic != FMCONFIG_MAGIC`,
  leave compiled-in defaults untouched (silent fallback, no popup — common on
  first run).
- `config_save(void)` — populate `cfg` from current globals,
  `file_save("0:/LOCIFM.CFG", &cfg, sizeof(cfg))` (`include/loci.h:317`).

**Wiring in `src/main.c`:**
- In `main()`: keep the existing hardcoded defaults block
  (`src/main.c:354-363`) as the true fallback, then call `config_load()`
  immediately after — it overwrites `confirm/filter/enterchoice/sort` if a
  valid config exists. Then run the `sprintf(pulldown_titles[0][...])` calls
  (lines 371-374) **after** `config_load()` so App-menu labels reflect loaded
  values.
- Call `config_save()` from each of the 4 mutators: `confirm_toggle()`
  (`src/main.c:41`), `select_enter_choice()` (`:49`, only if a selection was
  made), `select_filter()` (`:62`, only if a selection was made), and
  `dir_togglesort()` (in `src/dir.c` — export `config_save()` via `dir.h`
  since `dir.c` owns the 4 settings globals; `main.c`'s 3 mutators call the
  same exported function).

### Test scenario (new Scenario 6: `tests/scripts/test_settings.sh` + `make test-settings`)

1. **Save**: fresh sandbox (no `LOCIFM.CFG`), toggle sort on (`o`) and filter
   to DSK (`f`→DSK). Assert `check_host "config created" "[ -f '$SANDBOX/LOCIFM.CFG' ]"`.
2. **Load**: same sandbox (don't reset), reboot emulator with no toggles.
   Open App pulldown, assert `check_found "Sort: On"` and
   `check_found "...DSK..."` (filter label).
3. **Fallback**: fresh sandbox (no config file), boot, assert default labels
   (`check_found "Sort: Off"`, `check_found "Once"` for confirm).

**Done when:** toggled settings persist to `0:/LOCIFM.CFG`, reload across a
reboot, and a missing config falls back cleanly to defaults.

### Implementation notes (deviations/discoveries during Phase 3)

1. Implemented exactly as designed — `struct FmConfig`/`FMCONFIG_MAGIC` in
   `src/dir.h`, `config_load()`/`config_save()` in `src/dir.c`
   (`file_load`/`file_save` against `"0:/LOCIFM.CFG"`, magic-byte guard, silent
   fallback to compiled-in defaults on any failure).
2. `dir_togglesort()` (`src/dir.c`) calls `config_save()` directly at the end.
   `confirm_toggle()`, `select_enter_choice()`, and `select_filter()`
   (`src/main.c`) call `config_save()` after updating their respective global
   (the latter two only inside their `if (select)` branch, i.e. only when the
   popup wasn't cancelled).
3. `main()` calls `config_load()` immediately after the compiled-in defaults
   block, before the App-pulldown label `sprintf`s — so a valid
   `0:/LOCIFM.CFG` is reflected in the pulldown labels from first paint.
4. `make test-settings` passes 8/8 (Save/Reload/Fallback sub-tests) on the
   first run — no cycle-count calibration needed (`BOOT_CYCLES=8000000`,
   `OPEN_CYCLES=10000000`, `SAVE_CYCLES=14000000` worked as drafted). Full
   `make test` (now 6 scenarios: test-quick + test-menus + test-fileops +
   test-libdemo + test-recurse + test-settings) passes 190/190. Both EN and FR
   builds clean.

---

## Phase 4 — Filename search/filter by name (wildcard)

**`src/dir.h`/`src/dir.c`:**
- New global `char namefilter[32] = ""` (empty = no name filter; **not**
  persisted — separate from `FmConfig`).
- New static `dir_wildcard_match(const char *pattern, const char *name)` —
  classic `*`/`?` glob matcher, case-insensitive (reuse the existing
  case-fold approach used for sorting). Pattern ≤31 chars, name ≤63 chars —
  small bounded recursion here is fine (not directory-depth recursion).
- In `dir_read()` (`src/dir.c:222-438`), alongside the existing type-filter
  check (~line 316-322): if `namefilter[0]` set, `presenttype > 1` (not a
  directory — **directories always shown** so navigation isn't blocked), and
  `!dir_wildcard_match(namefilter, file->d_name)`, set `presenttype = 0`
  (excluded from listing).

**`src/main.c`:** implement the Phase-0 `case 'l':`/`case 62:` stub as new
`select_namefilter()`:
- Popup with `cwin_textinput(..., namefilter, 31, VINPUT_WILD | VINPUT_ALPHA)`
  pre-filled with current value (model popup on `dir_newdir()`,
  `src/dir.c:1160-1167`).
- Empty input clears the filter. On change, `dir_draw(0,1)` + `dir_draw(1,1)`.
- Update `MSG_MENU_TOOLS1` label dynamically to show the active pattern (e.g.
  `"Name: *.TXT"`) vs `"Filter by name"` when empty.

**New strings:** `MSG_DIR_NAMEFILTER_PROMPT`, `MSG_MENU_TOOLS1_FMT`.

### Test scenario (new Scenario 7: `tests/scripts/test_namefilter.sh` + `make test-namefilter`)

1. At root (`DEMO.TAP, FIRM.ROM, GAME.DSK, NOTES.TXT, SAVE.LCE, SUBDIR/, DEEP/`),
   press `l`, type `*.TXT`, ENTER.
2. Assert `check_found "NOTES.TXT"`, `check_not_found "DEMO.TAP"`,
   `check_found "SUBDIR"` (dirs always shown).
3. Clear filter (empty input) → assert `DEMO.TAP` reappears.

**Done when:** `*.TXT` hides non-matching files (not dirs) in both panes;
clearing restores full listing.

### Implementation notes (deviations/discoveries during Phase 4)

1. `dir_wildcard_match()` implemented as designed: case-insensitive `*`/`?`
   glob matcher with small bounded recursion (only on `*`), using
   `tolower((unsigned char)c)` for case-folding. `dir_read()` applies it only
   when `presenttype > 1` (directories always shown).
2. `select_namefilter()` deviates from the ROADMAP sketch:
   - `VINPUT_WILD | VINPUT_ALPHA` does **not** allow `.` (period) in
     `cwin_textinput()` (`include/charwin.c` validation tables) — switched to
     `VINPUT_ALL` (all printable 0x20-0x7E), which is needed for patterns
     like `*.TXT`.
   - The textinput is **not** pre-filled with the current `namefilter`.
     Instead the popup shows a read-only `MSG_DIR_NAMEFILTER_CURRENT_FMT`
     ("Current: %s" / "Actuel: %s", via `cwin_putat_printf`) line above an
     empty input. Typing a pattern + ENTER sets it; ENTER on the empty input
     clears it; ESC cancels with no change. This avoids needing to delete
     pre-filled text (the DEL key cannot be sent by Phosphoric's
     `--type-keys`, so a pre-fill design would be untestable).
   - `MSG_MENU_TOOLS1_FMT` was not needed — the Tools pulldown label
     (`pulldown_titles[5][1]`) is updated in place via a small bounded copy
     loop (`MSG_MENU_VAL_NAME` + ":" + pattern, capped to fit
     `PULLDOWN_MAXLENGTH=17`), falling back to the static `MSG_MENU_TOOLS1`
     string when the filter is cleared.
3. **Real bug found and fixed** (general, not filter-specific):
   `menu_popup_close()` (→ `menu_winrestore()`) restores screen rows 8-22
   from overlay RAM, which overlaps both directory panes (rows 3-14 and
   15-26). Any popup handler that calls `dir_draw()` after a text input must
   call `menu_popup_close()` **before** `dir_draw()`, not after — otherwise
   the restore clobbers the freshly-redrawn pane content with stale
   pre-popup data. `select_namefilter()` follows this order. (`dir_newdir()`
   in `src/dir.c` has the old/wrong order and may have the same latent bug —
   not fixed here, out of scope for Phase 4.)
4. **Testing technique discovery**: Phosphoric's `--type-keys` needs a `\p1`
   (>=1s) pause **before every distinct keypress** in a sequence, not just
   between repeats of the same key — otherwise the keyboard scanner's
   release debounce swallows the next key. Typing `*.TXT` requires
   `l\p1*\p1.\p1T\p1X\p1T\p1\n`.
5. `make test-namefilter` passes 15/15 first try after the popup-restore
   fix; full `make test` (7 scenarios) passes 205/205, EN+FR build clean.

---

## Phase 5 — Mid-file copy cancellation

**File:** `include/loci.c`, function `file_copy_progress()` (lines 587-634).

- Inside the chunked copy loop (lines 606-626), at the top of each iteration,
  check `keyb_check() == KEY_ESC` (include `keyboard.h` if not already
  included).
- On ESC: break the loop with a new return value `-2` (distinct from the
  existing `-1` I/O-error path, which sets `loci_errno`).
- After the loop, before the existing `loci_close` calls (lines 628-629): if
  result `== -2`, after closing both fds, `loci_unlink(dst)` to remove the
  partial destination file.

**Callers** (`src/file.c` `file_copy_move_selected`, line ~97, and Phase 1's
`file_copy_move_dir`/`file_copy_move_cb`): distinguish `-2` from other
errors — show `MSG_FILE_COPY_CANCELLED` (not an error popup) and stop
processing further selected files / propagate `RECURSE_ABORT`.

**New strings:** `MSG_FILE_COPY_CANCELLED` ("Copy cancelled.").

### Test scenario (new Scenario 8: `tests/scripts/test_copycancel.sh` + `make test-copycancel`)

- New fixture `tests/fixtures/BIGFILE.BIN` — large enough (start at 64KB =
  32 chunks at `COPYBUF_XRAM_SIZE`=2048B) to give a wide window for ESC to
  land mid-copy; tune size via `make test-capture` if needed.
1. Select `BIGFILE.BIN`, switch pane to `SUBDIR/`, press `c`, then send ESC
   timed mid-copy (calibrate cycle offset via `make test-capture`).
2. Assert `check_found "LOCI File Manager"`, `check_not_found "File error"`,
   and **`check_host "partial dest removed" "[ ! -e '$SANDBOX/SUBDIR/BIGFILE.BIN' ]"`**.
3. Regression sub-test (no ESC): full copy of `BIGFILE.BIN` completes with
   correct size — `check_host "[ \$(stat -c%s '$SANDBOX/SUBDIR/BIGFILE.BIN') -eq \$(stat -c%s '$SANDBOX/BIGFILE.BIN') ]"`.

**Done when:** ESC mid-copy aborts and removes the partial file; an
uninterrupted copy of the same file still completes correctly.

*Note: ESC-timing calibration is the main risk here — budget extra
`make test-capture` iteration.*

### Implementation notes (deviations/discoveries during Phase 5)

1. `file_copy_progress()` (`include/loci.c`) implemented as designed: the
   ESC check is the first statement of each chunk-loop iteration, sets
   `result = -2` and `break`s on `KEY_ESC`, and `loci_unlink(dst)` runs after
   both `loci_close()` calls when `result == -2`.
2. Both callers (`file_copy_move_selected()` and `file_copy_move_cb()` in
   `src/file.c`) distinguish `-2` from other non-zero results and show
   `MSG_FILE_COPY_CANCELLED` instead of the generic file-error popup.
   `file_copy_move_selected()`'s check uses `if`/`else if` (not a `switch`)
   because its `break` must exit the enclosing per-file `do {...} while`
   loop — a `switch`'s `break` only exits the switch, a real bug introduced
   and reverted during implementation.
3. **Major testing discovery — Phosphoric keyboard-debounce timing for
   ESC.** `include/keyboard.c`'s `RELEASE_DEBOUNCE=20` only counts down on
   polls where the matrix scan reports `ch==0` (truly no key pressed). While
   *any* key is continuously held — even a different one — `release_count`
   never reaches 0, so `keyb_check()` keeps returning `KEY_NONE` and a new
   key (ESC) cannot register, no matter how Phosphoric's `--type-keys`
   presses it (`\e\e` etc.). The fix is a `\p1` (1,000,000-cycle true no-key
   gap) immediately before `\e`: this both releases the prior key and lets
   `release_count` reach 0 *while the copy loop is still polling*, so the
   very next `keyb_check()` after `\e` is pressed sees `KEY_ESC`.
4. **Fixture-size threshold derived from (3)**: counting from the moment the
   prior key ('c') is pressed, `\p1\e` presses ESC ~1,079,872 cycles later
   (79872-cycle hold of 'c' + 1,000,000-cycle `\p1` gap). For ESC to land
   *during* the copy (not after the copy loop has already exited), the copy
   must still be running at that point — i.e. `copy_duration_cycles >
   ~1,079,872`. Measured rate is ~1100-1700 cycles per 2KB
   (`COPYBUF_XRAM_SIZE`) chunk, so files ≤512KB (≤256 chunks, ≤~450K cycles)
   complete too fast for ESC to land mid-copy; **2MB (1024 chunks, ~1.1-1.7M
   cycles) was chosen** as comfortably straddling the threshold.
5. **Fixture placement**: the 2MB `BIGFILE.BIN` lives in `tests/assets/`, not
   `tests/fixtures/` — `tests/fixtures/` is copied into the sandbox by every
   test target's shared `sandbox-reset`, and a 9th root-level entry there
   shifts the alphabetically-sorted root-listing cursor positions that
   `test_fileops.sh` and `test_recurse.sh` are calibrated against (confirmed:
   adding it to `tests/fixtures/` broke 4 `test-fileops` and 11 `test-recurse`
   assertions). `test_copycancel.sh`'s own `reset_sandbox()` copies
   `tests/assets/BIGFILE.BIN` into the sandbox root after the shared-fixture
   copy, keeping the change isolated to this scenario.
6. `make test-copycancel` passes 9/9 (ESC-cancel: popup shows "Copy
   cancelled.", partial dest removed, source untouched; regression:
   uninterrupted 2MB copy completes with matching size). Full `make test` (8
   scenarios) passes 214/214, EN+FR build clean.

*Relevance to Phase 7*: the "Calculating..."/ESC-cancel design for recursive
directory size also polls `keyb_check()==KEY_ESC` in a tight loop, so the
same `\p1\e` technique (point 3) applies if that path needs testing — but
unlike a file copy, a directory walk has no natural "fixture size" knob, so
timing may need a deep fixture tree instead.

---

## Phase 6 — Text file viewer

**New module:** `src/viewer.h` / `src/viewer.c`
- `void viewer_show_text(const char *path);`
- `static char viewer_chunk[4096];` (static buffer, well within free RAM).
- Logic: `loci_open(path, O_RDONLY)` → `menu_popup_open(0,0,28)` full-screen
  console window (model on `versioninfo()`, `src/main.c:90`) → loop
  `loci_read(fd, viewer_chunk, sizeof(viewer_chunk))` then
  `cwin_printwrap(&win, viewer_chunk)` (handles word-wrap/partial lines across
  chunks) until EOF → pause for keypress per screen (ENTER/SPACE = next page,
  ESC = exit) → `loci_close`, `menu_popup_close()`.
- **Forward-only paging** for v1 (no scroll-back) — documented limitation.

**Wiring (`src/main.c`):** replace Phase-0 `case 'j':` and `case 63:` stubs:
`if (presentdir[activepane].firstelement && !insidetape[activepane] &&
presentdirelement.meta.type != 1) { dir_build_path(...); viewer_show_text(pathbuffer); }`
— any non-directory entry. Add `#include "viewer.h"`; add `src/viewer.h/.c`
to `Makefile` `MAIN_SRCS`. Fill in the Phase-0 help-table row for `J`.

**New strings:** `MSG_HELP_J` ("View text file"), `MSG_VIEWER_PRESS_KEY`
("SPACE: next page  ESC: exit").

### Test scenario (new Scenario 9: `tests/scripts/test_viewer.sh` + `make test-viewer`)

- Use existing `tests/fixtures/NOTES.TXT` (read its actual content first to
  pick a search string). Add `tests/fixtures/LONGTEXT.TXT` (>1 screen, >28
  wrapped lines) for the paging test.
1. Select `NOTES.TXT`, press `j`. Assert known substring from `NOTES.TXT`
   visible. Press ESC, assert `check_found "LOCI File Manager"`.
2. Select `LONGTEXT.TXT`, press `j`, press SPACE/ENTER, assert a string only
   present on page 2 becomes visible.

**Done when:** `j` opens a full-screen paged, word-wrapped viewer; paging
works for multi-page files; ESC returns cleanly.

### Implementation notes (deviations/discoveries during Phase 6)

1. `src/viewer.h`/`src/viewer.c` implemented largely as designed, with two
   sizing deviations from the original plan:
   - `viewer_chunk[512]` (not `[4096]`) — plenty for `loci_read()`, which
     internally chunks in ≤256-byte XSTACK blocks anyway; no benefit to a
     larger buffer.
   - A separate `viewer_line[128]` accumulates one logical line (split on
     `\n`, `\r` ignored) before calling `cwin_printwrap()`, so a single
     `cwin_printwrap()` call never spans a chunk boundary mid-line. Lines
     longer than 127 chars are flushed early (still word-wrapped correctly by
     `cwin_printwrap()`, just in two calls).
   - `viewer_show_text()` uses a 38x27 content window (rows 0-26) plus a 1-row
     footer window (row 27) for the "next page"/"press a key" prompts, rather
     than a single 0,0,28 window with prompts overwriting content — keeps the
     pause messages from disturbing already-wrapped text.
2. Pagination trigger: after each line is wrapped, if `content->cy >= wy-1`
   (last row reached), show `MSG_VIEWER_PRESS_KEY` ("SPACE: next page  ESC:
   exit") in the footer; ESC exits immediately, any other key clears both
   windows and starts a fresh page. At EOF (not aborted), `MSG_MAIN_PRESS_CONTINUE`
   ("Press a key to continue") is shown instead — reusing the existing
   string rather than adding a new `MSG_HELP_J`/EOF-specific message (the
   plan's `MSG_HELP_J` was folded into the existing `MSG_HELP_TABLE` "J" row
   update instead of a separate macro).
3. Wiring matches the plan exactly: `case 63:` (Tools pulldown) and
   `case 'j':` (direct hotkey) both guard on
   `presentdir[activepane].firstelement && !insidetape[activepane] &&
   presentdirelement.meta.type != 1` (any non-directory entry), build the
   full path via `dir_build_path()`, and call `viewer_show_text()`.
   `loci_open()` failure shows `menu_fileerrormessage()` (self-contained, no
   extra cleanup needed since the popup window hasn't been opened yet).
4. **New testing discovery — `config_save()`/`LOCIFM.CFG` cross-run sandbox
   pollution.** Every test script presses `o` (sort toggle) for predictable
   navigation order. Since Phase 3, `dir_togglesort()` calls `config_save()`,
   which writes `0:/LOCIFM.CFG` (5 bytes) to the sandbox root via LOCI
   hostfs. This file does **not** appear in the listing of the run that
   creates it (the directory listing is read once at boot, not rescanned
   after the write) — but it **persists on the host filesystem**, so any
   *later* `run_emu` invocation sharing the same sandbox boots with a 10th
   root-level entry, shifting the alphabetically-sorted cursor positions
   (`NOTES.TXT` 6→7, `SUBDIR/` 8→9) that later sub-tests' `--type-keys`
   sequences were calibrated against. Fix: `test_viewer.sh` gained its own
   `reset_sandbox()` (mirroring `test_copycancel.sh`, without `BIGFILE.BIN`),
   called before *each* of its 5 `run_emu` invocations. **This applies to any
   future multi-run test script that presses `o`** — including Phase 7's
   `test_recurse.sh` extensions.
5. `tests/fixtures/SUBDIR/LONGTEXT.TXT` (30 lines, "LINE NN OF LONGTEXT
   FIXTURE FILE", 990 bytes) added inside `SUBDIR/` rather than the fixtures
   root, per the Phase 5 fixture-placement precedent — `SUBDIR/` is not
   navigated by position in other scenarios, so adding a file there doesn't
   shift any root-listing cursor positions.
6. `make test-viewer` passes 14/14 (NOTES.TXT content + EOF prompt; ESC
   restores main interface; LONGTEXT.TXT page 1 shows LINE 01-27 + "next
   page" prompt, not LINE 28; SPACE advances to page 2 showing LINE 28-30 +
   EOF prompt, LINE 01 no longer visible; ESC from page 2 restores main
   interface). Full `make test` (9 scenarios) passes 228/228, EN+FR build
   clean.

---

## Phase 7 — Properties popup + recursive directory size

**File:** `src/dir.c`, new `void dir_show_properties(void)`:

1. Guard: `presentdir[activepane].firstelement && !insidetape[activepane]`.
2. `menu_popup_open(0, 8, 12)` (model on `help()`, `src/main.c:129-162`).
3. Fields:
   - **Name**: `presentdirelement.name`.
   - **Type**: `dir_entry_types[presentdirelement.meta.type - 1]`.
   - **Size/Attributes**: `DirElement`/`DirMeta` (`src/dir.h:37-44`) does
     **not** store `d_attrib`/`d_size`. Re-fetch: `loci_opendir(presentdir[activepane].path)`,
     loop `loci_readdir` until `d_name` matches `presentdirelement.name`,
     capture `d_attrib` (decode `DIR_ATTR_RDO`/`DIR_ATTR_SYS`) and `d_size`,
     `loci_closedir`.
   - **Path**: `presentdir[activepane].path`.
   - **Size for directories**: recursive total via
     `recurse_walk(fullpath, dir_size_cb, &total)` where `dir_size_cb` on
     `RECURSE_FILE` does `*(uint32_t*)userdata += entry->d_size`. Show
     "Calculating..." during the walk; poll `keyb_check()==KEY_ESC` in the
     callback to allow cancellation (`RECURSE_ABORT` → show "cancelled").
     If `recurse_truncated`, append "+" to indicate approximate total.
   - Format uint32 size with new `dir_dec10()` (model on `dir_dec6()`,
     `src/dir.c:110`, same divide-by-10 loop extended to 10 digits).
4. `cwin_getch()`, `menu_popup_close()`.

**Wiring:** replace Phase-0 `case 'k':`/`case 61:` stubs with
`dir_show_properties()`; prototype in `dir.h`; fill in help-table row for `K`.

**New strings:** `MSG_PROP_TITLE`, `MSG_PROP_NAME`, `MSG_PROP_TYPE`,
`MSG_PROP_SIZE`, `MSG_PROP_PATH`, `MSG_PROP_ATTR`, `MSG_PROP_CALCULATING`,
`MSG_PROP_BYTES`.

### Test scenario (extend `test_recurse.sh`, fresh `reset_sandbox`)

1. **File properties**: select `NOTES.TXT`, press `k`. Assert popup shows
   `NOTES.TXT`, a byte count, and correct type.
2. **Directory recursive size**: select `DEEP/` (fixture files sized exactly
   100/200/300 bytes per Phase 0 note → total 600), press `k`, wait for
   "Calculating..." to resolve, assert `600` appears on screen.

**Done when:** Properties popup (`k`) shows name/type/size/path/attributes
for files, and a correct recursively-computed total size (with
"Calculating..."/ESC-cancel) for directories.

### Implementation notes (deviations/discoveries during Phase 7)

1. `dir_dec10()` (`src/dir.c`, right after `dir_dec6()`) follows the plan
   exactly: same divide-by-10 loop as `dir_dec6()` but with a 10-char
   buffer/output (`"%10lu"`-equivalent), space-padded and NUL-terminated.
2. `dir_show_properties()` (`src/dir.c`, new section at end of file) matches
   the plan's logic, with naming/string deviations from the literal names
   listed in the plan:
   - String macros are all `_FMT`-suffixed (`MSG_PROP_NAME_FMT`,
     `MSG_PROP_TYPE_FMT`, `MSG_PROP_PATH_FMT`, `MSG_PROP_ATTR_FMT`,
     `MSG_PROP_SIZE_FMT`), each a one-arg (or two-arg for size) printf format
     — consistent with the existing `_FMT` convention used elsewhere in
     `strings_en.h`/`strings_fr.h` rather than the plan's unsuffixed names.
   - Two strings beyond the plan's list were needed: `MSG_PROP_BYTES_APPROX`
     ("bytes+", shown instead of `MSG_PROP_BYTES` when `recurse_truncated` is
     set after the recursive walk) and `MSG_PROP_CANCELLED` ("Cancelled.",
     shown in the size row if the walk is ESC-aborted via `dir_size_cb`).
   - `presentdirelement.name` has dir_read()'s appended trailing `/` stripped
     into a local `namebuf[64]` before re-fetching `d_attrib`/`d_size` via
     `loci_opendir`/`loci_readdir`/`strcmp`/`loci_closedir` on
     `presentdir[activepane].path` — `loci_readdir()` names never carry the
     trailing `/`.
   - Attribute string is 2 chars (`R`/`-` then `S`/`-`) built directly into a
     `char attrstr[3]`, passed as `%s` to `MSG_PROP_ATTR_FMT` — no separate
     per-flag strings needed.
   - Popup is `menu_popup_open(0, 8, 12)` / `cwin_init(&popup, 2, 8, 38, 12,
     A_FWBLACK, A_BGWHITE)`, matching the plan's coordinates exactly. Layout:
     row 0 title, row 2 name, row 3 type, row 4 path, row 5 attr, row 7
     size/calculating/cancelled, row 11 `MSG_MAIN_PRESS_CONTINUE` (reused, no
     new "press a key" string needed).
   - `dir_size_cb` checks `keyb_check() == KEY_ESC` every callback invocation
     (every file/dir/leave-dir event), not just on files — slightly more
     responsive cancellation than the plan's "poll in the RECURSE_FILE
     callback" wording, with no behavioural difference in practice.
3. Wiring matches the plan exactly: `case 61:` (Tools pulldown) and
   `case 'k':` (direct hotkey) both call `dir_show_properties()` directly
   (the function's own guard handles the empty-pane/inside-tape checks, so no
   extra guard needed at the call sites). Help-table row for `K` updated from
   "Properties (TBD)" to "Properties" (EN) / "Proprietes (TBD)" to
   "Proprietes" (FR).
4. **Test pollution fix applied as flagged in Phase 6**: both new
   `test_recurse.sh` sub-tests press `o` (sort toggle) for predictable cursor
   positions, so each gets its own `reset_sandbox()` first — consistent with
   the `LOCIFM.CFG`/`config_save()` cross-run pollution fix documented in
   Phase 6's notes.
5. Extended `test_recurse.sh` (no new scenario number — Phase 7 adds to
   Scenario 5) with two sub-tests, `PROPS_CYCLES=14000000`:
   - File properties on `NOTES.TXT` (sorted pos 6, `o\p1\d\d\d\d\d\d\p1k`):
     asserts "Properties", "NOTES.TXT", "78 bytes", "Path: 0:/" all visible.
   - Directory recursive size on `DEEP/` (sorted pos 0, `o\p1k`): asserts
     "Properties", "DEEP/", "Type: DIR", "600 bytes" all visible — the
     recursive walk over `DEEP/`'s 3 files (100+200+300=600 bytes) completes
     fast enough that "Calculating..." is already replaced by the final total
     within `PROPS_CYCLES`.
6. `make test-recurse` passes 39/39 (8 new checks). Full `make test` (9
   scenarios) passes 236/236, EN+FR build clean (only the pre-existing
   `include/charwin.c` "Unreachable code" warning, not a regression).

---

## Summary Table

| Phase | Feature | New files | Key functions | Test target |
|---|---|---|---|---|
| 0 | Foundation | `src/recurse.c/h` | `recurse_walk()` | `test-recurse` (structural) |
| 1 | Recursive copy/move | — | `file_copy_move_dir()`, `file_copy_move_cb()` | extend `test-recurse` |
| 2 | Recursive delete | — | `dir_delete_recursive()`, `dir_delete_cb()` | extend `test-recurse` |
| 3 | Persistent settings | — | `config_load()`, `config_save()` | `test-settings` |
| 4 | Name-filter wildcard | — | `dir_wildcard_match()`, `select_namefilter()` | `test-namefilter` |
| 5 | Mid-copy cancellation | — | `file_copy_progress()` (modified) | `test-copycancel` |
| 6 | Text viewer | `src/viewer.c/h` | `viewer_show_text()` | `test-viewer` |
| 7 | Properties + recursive size | — | `dir_show_properties()`, `dir_size_cb()`, `dir_dec10()` | extend `test-recurse` |

## Open risks (to verify during implementation, not blocking start)

1. ~~Drive-0 "not empty" errno for Phase 2's fallback path~~ — RESOLVED in
   Phase 1: `loci_unlink()` on an empty directory now succeeds (after the
   Phosphoric `op_unlink` rmdir fix, see Phase 1 implementation notes); on a
   non-empty directory it fails with errno 3 (`LOCI_EACCES`, from `rmdir()`'s
   `ENOTEMPTY`).
2. Phase 5 ESC-timing calibration may need iteration with `make test-capture`.
3. ~~`recurse_walk`'s in-place `dir_build_path(...)` self-truncate-and-append
   usage~~ — RESOLVED in Phase 0/1: confirmed safe (`dir_build_path` is
   `strncpy`+`strncat` into the same buffer, used throughout `recurse_walk`
   without issue).

## Progress

- [x] Phase 0 — Foundation (recurse module, Tools pulldown, fixtures, Scenario 5 structural)
- [x] Phase 1 — Recursive directory copy/move
- [x] Phase 2 — Recursive directory delete
- [x] Phase 3 — Persistent settings
- [x] Phase 4 — Filename search/filter by name (wildcard)
- [x] Phase 5 — Mid-file copy cancellation
- [x] Phase 6 — Text file viewer
- [x] Phase 7 — Properties popup + recursive directory size
