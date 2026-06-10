// strings_fr.h - Main application French user-visible strings
// All MSG_* macros for the main locifilemanager application (French).
// No accented characters - Oric ROM charset has no e-acute, e-grave, etc.
// Phase 3: placeholder — MSG_* macros will be populated in Phase 4.
// Do not use MSG_DEMO_* macros from here; use strings_demo_fr.h instead.

#ifndef STRINGS_FR_H
#define STRINGS_FR_H

// ── Phase 3: LOCI library error / status messages used in get_locicfg() ──────
#define MSG_LOCI_NOT_FOUND  "Aucun LOCI detecte ou micrologiciel trop ancien."
#define MSG_PRESS_KEY_EXIT  "Appuyer sur une touche pour quitter."

// ── Phase 4+: menu system strings ─────────────────────────────────────────────

// Application header / window title
#define MSG_MENU_HEADER         "LOCI Gestionnaire"

// Menu bar titles
#define MSG_MENU_BAR_APP        "App"
#define MSG_MENU_BAR_FILE       "Fich"
#define MSG_MENU_BAR_DIR        "Rep"
#define MSG_MENU_BAR_MOUNTS     "Monts"
#define MSG_MENU_BAR_INFO       "Info"

// Pulldown 0 — App (items 0-3 overwritten at runtime via snprintf + _FMT macros)
#define MSG_MENU_APP0           "Confirm.: 1x"
#define MSG_MENU_APP1           "Retour:  Select"
#define MSG_MENU_APP2           "[F]iltrer: Aucun"
#define MSG_MENU_APP3           "T[R]i:    Non"
#define MSG_MENU_APP4           "[ESC] Quitter"

// Pulldown 1 — File
#define MSG_MENU_FILE0          "[S]el. basculer"
#define MSG_MENU_FILE1          "Sel. [T]out"
#define MSG_MENU_FILE2          "Sel. [A]ucun"
#define MSG_MENU_FILE3          "[I]nverser sel."
#define MSG_MENU_FILE4          "[DEL]ete"
#define MSG_MENU_FILE5          "[R]enommer"
#define MSG_MENU_FILE6          "[C]opier"
#define MSG_MENU_FILE7          "Dep[L]acer"
#define MSG_MENU_FILE8          "Lire band[E]"

// Pulldown 2 — Dir
#define MSG_MENU_DIR0           "[\\] Racine"
#define MSG_MENU_DIR1           "[<] Retour"
#define MSG_MENU_DIR2           "Page suivante[D]"
#define MSG_MENU_DIR3           "Page prec[E]."
#define MSG_MENU_DIR4           "Pre[M]ier"
#define MSG_MENU_DIR5           "[D]ernier"
#define MSG_MENU_DIR6           "N[O]uv. rep."

// Pulldown 3 — Mounts (item 5 overwritten at runtime)
#define MSG_MENU_MNT0           "[/] Autre pan."
#define MSG_MENU_MNT1           "[.] Suiv. unité"
#define MSG_MENU_MNT2           "[,] Prec. unité"
#define MSG_MENU_MNT3           "[M]onter"
#define MSG_MENU_MNT4           "[D]emonter"
#define MSG_MENU_MNT5           "Cib[L]e: A"
#define MSG_MENU_MNT6           "Voir montages"

// Pulldown 4 — Info
#define MSG_MENU_INFO0          "Version/credits"
#define MSG_MENU_INFO1          "Aide"

// Pulldown 5 — Enter-action sub-menu
#define MSG_MENU_ENT0           "Selectionner"
#define MSG_MENU_ENT1           "Monter"
#define MSG_MENU_ENT2           "Lancer"

// Pulldown 6 — Filter sub-menu
#define MSG_MENU_FLT0           "Aucun"
#define MSG_MENU_FLT1           ".DSK"
#define MSG_MENU_FLT2           ".TAP"
#define MSG_MENU_FLT3           ".ROM"
#define MSG_MENU_FLT4           ".LCE"

// Pulldown 7 — Target drive A-D
#define MSG_MENU_DRV0           "A"
#define MSG_MENU_DRV1           "B"
#define MSG_MENU_DRV2           "C"
#define MSG_MENU_DRV3           "D"

// Pulldown 8 — Mount source (A/B/C/D/Tape/ROM/None)
#define MSG_MENU_SRC0           "A"
#define MSG_MENU_SRC1           "B"
#define MSG_MENU_SRC2           "C"
#define MSG_MENU_SRC3           "D"
#define MSG_MENU_SRC4           "Ruban"
#define MSG_MENU_SRC5           "ROM"
#define MSG_MENU_SRC6           "Aucun"

// Pulldown 9 — Yes/No
#define MSG_MENU_YN0            "Oui"
#define MSG_MENU_YN1            "Non"

// Format macros for dynamic App pulldown entries (snprintf label + value → 16 chars max)
#define MSG_MENU_APP_CONFIRM_FMT    "Confirm.: %s"
#define MSG_MENU_APP_RETURN_FMT     "Retour: %s"
#define MSG_MENU_APP_FILTER_FMT     "[F]iltre: %s"
#define MSG_MENU_APP_SORT_FMT       "T[R]i: %s"
#define MSG_MENU_MNT_TARGET_FMT     "Cib[L]e: %s"

// Value strings for dynamic entries (padded so total fits in 16 chars)
#define MSG_MENU_VAL_ONCE       "1x"
#define MSG_MENU_VAL_ALL        "Tous"
#define MSG_MENU_VAL_SELECT     "Select"
#define MSG_MENU_VAL_ENTER      "Entrer"
#define MSG_MENU_VAL_LAUNCH     "Lancr."
#define MSG_MENU_VAL_OFF        "Non"
#define MSG_MENU_VAL_NAME       "Nom"
#define MSG_MENU_VAL_NONE       "Aucun"
#define MSG_MENU_VAL_DSK        ".DSK"
#define MSG_MENU_VAL_TAP        ".TAP"
#define MSG_MENU_VAL_ROM        ".ROM"
#define MSG_MENU_VAL_LCE        ".LCE"

// Popup dialog strings
#define MSG_MENU_AREYOUSURE     "Etes vous sur?"
#define MSG_MENU_PRESSAKEY      "Appuyer touche."
#define MSG_MENU_FILE_ERROR     "Erreur fichier!"
#define MSG_MENU_ERR_LABEL      "Erreur# : "
#define MSG_MENU_SELECT_OPT     "Choisir option:"

#endif
