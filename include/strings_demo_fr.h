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
#define MSG_DEMO_VERSION        "Version: %u.%u.%u"

// ── Build-status window ───────────────────────────────────────────────────────
#define MSG_DEMO_STATUS_CRT     "[ OK ] oric_crt.c demarrage + regions"
#define MSG_DEMO_STATUS_CHARWIN "[ OK ] bibliotheque charwin initialisee"
#define MSG_DEMO_STATUS_KEYB    "[ OK ] scanner clavier (VIA/AY)"
#define MSG_DEMO_STATUS_ORAM    "[ -- ] RAM overlay (necessite LOCI)"
#define MSG_DEMO_STATUS_ORAM_OK "[ OK ] RAM overlay (push/pop OK)"
#define MSG_DEMO_STATUS_ORAM_ERR "[ERR] RAM overlay test ECHEC"

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
#define MSG_DEMO_SECTION_F      "F: Edition texte (inserer/suppr/ligne)"
#define MSG_DEMO_SECTION_G      "G: Banc defilement (500 scrolls)"
#define MSG_DEMO_SECTION_H      "H: Defilement bas (cwin_scroll_down)"
#define MSG_DEMO_SECTION_I      "I: Balle rebondissante (ECHAP/ENTREE)"
#define MSG_DEMO_SECTION_J      "J: Viewport defilement (fleches, ECHAP)"
#define MSG_DEMO_SECTION_K      "K: Demo systeme de menus"
#define MSG_DEMO_SECTION_L      "L: Demo bibliotheque LOCI"

// ── Generic prompts ───────────────────────────────────────────────────────────
#define MSG_DEMO_PRESS_KEY      "Appuyer sur une touche..."

// ── Section E: getat read-back ────────────────────────────────────────────────
#define MSG_DEMO_GETAT_PASS     "GETAT: OK"
#define MSG_DEMO_GETAT_FAIL     "GETAT: ECHEC col %u"

// ── Section F: text editing ───────────────────────────────────────────────────
#define MSG_DEMO_EDIT_INIT      "Curseur col 5. Insertion 5 espaces..."
#define MSG_DEMO_EDIT_DELETE    "Suppression 5 car. a la col 5..."
#define MSG_DEMO_EDIT_PRINTLINE "cwin_printline: 8 lignes auto-scroll:"
#define MSG_DEMO_EDIT_LINE_FMT  "Ligne printline %u sur 8"

// ── Section G: scroll benchmark ───────────────────────────────────────────────
#define MSG_DEMO_SCROLL_DONE    "500 scrolls: termine. (rapide!)"

// ── Section H: scroll down ────────────────────────────────────────────────────
#define MSG_DEMO_SCROLLDN_DONE  "60 scrolls bas: termine."

// ── Section I: bouncing ball ──────────────────────────────────────────────────
#define MSG_DEMO_FRAME          "Image: %u"
#define MSG_DEMO_FRAMES_DONE    "Termine: %u images.    "

// ── Section J: viewport ───────────────────────────────────────────────────────
#define MSG_DEMO_VP_ARROWS      "Fleches defilent. ECHAP/ENTREE sortie."
#define MSG_DEMO_VP_POS         "Vue x:%u y:%u   "

// ── Section K: menu demo ──────────────────────────────────────────────────────
#define MSG_DEMO_MENU_INTRO     "Naviguer dans le menu. ECHAP = sortir."
#define MSG_DEMO_MENU_CHOICE    "Choix menu: %u   "
#define MSG_DEMO_MENU_POPUPS    "Demo des fenetres surgissantes..."
#define MSG_DEMO_MENU_POPUP_MSG "Ceci est une fenetre surgissante."
#define MSG_DEMO_MENU_POPUP_ASK "Supprimer le fichier test?"
#define MSG_DEMO_MENU_POPUP_RES "Reponse: %u (1=Oui 2=Non)"
#define MSG_DEMO_MENU_SEL_RES   "Selection: %u (0=annuler)"
#define MSG_DEMO_MENU_ERR_SIM   "Erreur simulee #42:"

// ── Section L: LOCI demo ──────────────────────────────────────────────────────
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
