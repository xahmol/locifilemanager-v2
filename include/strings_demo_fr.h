// strings_demo_fr.h - libdemo French user-visible strings
// All MSG_DEMO_* macros for src/libdemo.c (French).
// No accented characters - Oric ROM charset has no e-acute, e-grave, etc.
// Key bindings stay identical to English; if the key letter is absent from
// French text, a [key] hint is appended rather than changing the binding.

#ifndef STRINGS_DEMO_FR_H
#define STRINGS_DEMO_FR_H

// ── Intro / title bar ─────────────────────────────────────────────────────────
#define MSG_DEMO_TITLE          "LOCI FILE MANAGER V2 - TEST PHASE 2"
#define MSG_DEMO_SUBTITLE       "Compilation bare-metal Oric Oscar64"

// ── Build-status window ───────────────────────────────────────────────────────
#define MSG_DEMO_STATUS_CRT     "[ OK ] oric_crt.c demarrage + regions"
#define MSG_DEMO_STATUS_CHARWIN "[ OK ] bibliotheque charwin initialisee"
#define MSG_DEMO_STATUS_KEYB    "[ OK ] scanner clavier (VIA/AY)"
#define MSG_DEMO_STATUS_ORAM    "[ -- ] RAM overlay (necessite LOCI)"

// ── Text-input section ────────────────────────────────────────────────────────
#define MSG_DEMO_INPUT_PROMPT   "Saisie (ENTREE=valider, ECHAP=annuler):"
#define MSG_DEMO_INPUT_ENTERED  "Saisi:"
#define MSG_DEMO_INPUT_CANCEL   "ECHAP: saisie annulee"
#define MSG_DEMO_KEY_ECHO_NEXT  "Appuyer sur une touche pour l'echo..."
#define MSG_DEMO_KEY_ECHO_HDR   "Echo touche (hex). ENTREE/ECHAP halt:"

// ── Color quick-test labels (pre-section-A) ───────────────────────────────────
#define MSG_DEMO_COLOR_TEST     "Test couleurs:"
#define MSG_DEMO_COLOR_RED      "ENCRE ROU"
#define MSG_DEMO_COLOR_CYAN     "ENCRE CYA"
#define MSG_DEMO_COLOR_YELLOW   "ENCRE JAU"

// ── Section headers ───────────────────────────────────────────────────────────
#define MSG_DEMO_SECTION_A      "A: Palette couleurs (8 encre x 8 fond)"
#define MSG_DEMO_SECTION_A_HDR  "    NOI ROU VER JAU BLE MAG CYA BLA"
#define MSG_DEMO_SECTION_B      "B: Macros attributs inline (ASTR)"
#define MSG_DEMO_SECTION_C      "C: Remplissage rect (bordures conc.)"
#define MSG_DEMO_SECTION_D      "D: Deplacement curseur anime"
#define MSG_DEMO_SECTION_E      "E: Test relecture caractere getat"
#define MSG_DEMO_SECTION_F      "F: Banc defilement (500 scrolls)"
#define MSG_DEMO_SECTION_G      "G: Balle rebondissante (ECHAP/ENTREE)"
#define MSG_DEMO_SECTION_H      "H: Demo bibliotheque LOCI"

// ── Generic prompts ───────────────────────────────────────────────────────────
#define MSG_DEMO_PRESS_KEY      "Appuyer sur une touche..."

// ── Section E: getat read-back ────────────────────────────────────────────────
#define MSG_DEMO_GETAT_PASS     "GETAT: OK"
#define MSG_DEMO_GETAT_FAIL     "GETAT: ECHEC col %u"

// ── Section F: scroll benchmark ───────────────────────────────────────────────
#define MSG_DEMO_SCROLL_DONE    "500 scrolls: termine. (rapide!)"

// ── Section G: bouncing ball ──────────────────────────────────────────────────
#define MSG_DEMO_FRAME          "Image: %u"
#define MSG_DEMO_FRAMES_DONE    "Termine: %u images.    "

// ── Section H: LOCI demo ──────────────────────────────────────────────────────
#define MSG_DEMO_LOCI_DETECTED  "LOCI: Detecte"
#define MSG_DEMO_LOCI_NOT_FOUND "LOCI: Introuvable - materiel requis"
#define MSG_DEMO_LOCI_FW_VER    "Micrologiciel: "
#define MSG_DEMO_LOCI_CPU_KHZ   "CPU: "
#define MSG_DEMO_LOCI_KHZ_UNIT  " kHz"
#define MSG_DEMO_LOCI_DEV_COUNT "Appareils: "
#define MSG_DEMO_LOCI_DIR_HDR   "Dossier racine:"
#define MSG_DEMO_LOCI_TYPE_DIR  "[REP]"
#define MSG_DEMO_LOCI_TYPE_FILE "[FIC]"
#define MSG_DEMO_LOCI_IJK_FOUND "Joystick IJK: trouve"
#define MSG_DEMO_LOCI_IJK_NONE  "Joystick IJK: absent"
#define MSG_DEMO_ERROR          "Erreur: "

// ── Completion ────────────────────────────────────────────────────────────────
#define MSG_DEMO_DONE           "TEST PHASE 2 TERMINE. ARRETE."

#endif
