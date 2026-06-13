// strings_fr.h - Main application French user-visible strings
// All MSG_* macros for the main locifilemanager application (French).
// No accented characters - Oric ROM charset has no e-acute, e-grave, etc.
// Do not use MSG_DEMO_* macros from here; use strings_demo_fr.h instead.

#ifndef STRINGS_FR_H
#define STRINGS_FR_H

// ── LOCI library error / status messages used in get_locicfg() ───────────────
#define MSG_LOCI_NOT_FOUND  "Aucun LOCI detecte ou micrologiciel trop ancien."
#define MSG_PRESS_KEY_EXIT  "Appuyer sur une touche pour quitter."

// ── Menu system strings ───────────────────────────────────────────────────────

// Application header / window title
#define MSG_MENU_HEADER         "LOCI Gestionnaire"

// Menu bar titles
#define MSG_MENU_BAR_APP        "App"
#define MSG_MENU_BAR_FILE       "Fich"
#define MSG_MENU_BAR_DIR        "Rep"
#define MSG_MENU_BAR_MOUNTS     "Monts"
#define MSG_MENU_BAR_INFO       "Info"
#define MSG_MENU_BAR_TOOLS      "Outils"

// Pulldown 0 — App (items 0-3 overwritten at runtime via snprintf + _FMT macros)
#define MSG_MENU_APP0           "Confirm.: 1x"
#define MSG_MENU_APP1           "Retour:  Select"
#define MSG_MENU_APP2           "[F] Type: Aucun"
#define MSG_MENU_APP3           "[O] Tri:  Non"
#define MSG_MENU_APP4           "[ESC] Quitter"

// Pulldown 1 — File
#define MSG_MENU_FILE0          "[S]el. basculer"
#define MSG_MENU_FILE1          "Sel. [A] Tout"
#define MSG_MENU_FILE2          "Sel. Aucu[N]"
#define MSG_MENU_FILE3          "[I]nverser sel."
#define MSG_MENU_FILE4          "[DEL]ete"
#define MSG_MENU_FILE5          "[R]enommer"
#define MSG_MENU_FILE6          "[C]opier"
#define MSG_MENU_FILE7          "[V] Deplacer"
#define MSG_MENU_FILE8          "[W] Lire bande"

// Pulldown 2 — Dir
#define MSG_MENU_DIR0           "[\\] Racine"
#define MSG_MENU_DIR1           "[<] Retour"
#define MSG_MENU_DIR2           "Page suivante[D]"
#define MSG_MENU_DIR3           "[P]age prec."
#define MSG_MENU_DIR4           "[T] Premier"
#define MSG_MENU_DIR5           "[B] Dernier"
#define MSG_MENU_DIR6           "Nouv. r[E]p."

// Pulldown 3 — Mounts (item 5 overwritten at runtime)
#define MSG_MENU_MNT0           "[/] Autre pan."
#define MSG_MENU_MNT1           "[.] Suiv. unite"
#define MSG_MENU_MNT2           "[,] Prec. unite"
#define MSG_MENU_MNT3           "[M]onter"
#define MSG_MENU_MNT4           "[U] Demonter"
#define MSG_MENU_MNT5           "[G] Cible: A"
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

// Pulldown 10 — Tools
#define MSG_MENU_TOOLS0         "[K] Proprietes"
#define MSG_MENU_TOOLS1         "[L] Texte: Non"
#define MSG_MENU_TOOLS2         "[J] Voir texte"
#define MSG_MENU_TOOLS3         "[Y] Favoris"

// Format macros for dynamic App pulldown entries (snprintf label + value → 16 chars max)
#define MSG_MENU_APP_CONFIRM_FMT    "Confirm.: %s"
#define MSG_MENU_APP_RETURN_FMT     "Retour: %s"
#define MSG_MENU_APP_FILTER_FMT     "[F] Type: %s"
#define MSG_MENU_APP_SORT_FMT       "[O] Tri: %s"
#define MSG_MENU_MNT_TARGET_FMT     "[G] Cible: %s"
#define MSG_MENU_TOOLS_TEXT_FMT     "[L] Texte: %s"

// Value strings for dynamic entries (padded so total fits in 16 chars)
#define MSG_MENU_VAL_ONCE       "1x"
#define MSG_MENU_VAL_ALL        "Tous"
#define MSG_MENU_VAL_SELECT     "Select"
#define MSG_MENU_VAL_ENTER      "Entrer"
#define MSG_MENU_VAL_LAUNCH     "Lancr."
#define MSG_MENU_VAL_OFF        "Non"
#define MSG_MENU_VAL_ON         "Oui"
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
#define MSG_MENU_CONFIRM_NAME   "Nom:"

// ── Dir module strings ────────────────────────────────────────────────────────
#define MSG_DIR_TAPE_PREFIX     "Ruban: "
#define MSG_DIR_EMPTY           "Repertoire vide."
#define MSG_DIR_CREATE_TITLE    "Creer repertoire."
#define MSG_DIR_ENTER_NAME      "Nom du repertoire:"
#define MSG_DIR_DELETE_Q        "Effacer rep.?"
#define MSG_DIR_DELETE_RECURSE_Q "Rep. non vide. Tout effacer?"
#define MSG_DIR_DELETING        "Suppression:"
#define MSG_DIR_RECURSE_TRUNCATED "Arbre trop profond, suppr. incompl."
#define MSG_DIR_NAMEFILTER_TITLE  "Filtre par nom."
#define MSG_DIR_NAMEFILTER_CURRENT_FMT "Actuel: %s"
#define MSG_DIR_NAMEFILTER_PROMPT "Motif (* ?), vide=non:"

// Popup proprietes (Outils -> Proprietes, touche K)
#define MSG_PROP_TITLE          "Proprietes"
#define MSG_PROP_NAME_FMT       "Nom: %s"
#define MSG_PROP_TYPE_FMT       "Type: %s"
#define MSG_PROP_PATH_FMT       "Chemin: %s"
#define MSG_PROP_ATTR_FMT       "Attr: %s"
#define MSG_PROP_SIZE_FMT       "Taille: %s %s"
#define MSG_PROP_BYTES          "octets"
#define MSG_PROP_BYTES_APPROX   "octets+"
#define MSG_PROP_CALCULATING    "Calcul en cours..."
#define MSG_PROP_CANCELLED      "Annule."
#define MSG_PROP_TYPE_DIR       "Repertoire"
#define MSG_PROP_TYPE_DSK       ".DSK - Image disque"
#define MSG_PROP_TYPE_TAP       ".TAP - Image bande"
#define MSG_PROP_TYPE_ROM       ".ROM - Image ROM"
#define MSG_PROP_TYPE_LCE       ".LCE - Executable LOCI"
#define MSG_PROP_TYPE_CFG       ".CFG - Fichier config"
#define MSG_PROP_TYPE_SYS       ".SYS - Fichier systeme"

// Popup favoris (Outils -> Favoris, touche Y)
#define MSG_FAV_TITLE           "Favoris"
#define MSG_FAV_SLOT_FMT        "%u: %s"
#define MSG_FAV_EMPTY           "(vide)"
#define MSG_FAV_HELP            "1-8:Aller A:Ajout D:Suppr ESC:Fermer"
#define MSG_FAV_ADD_PROMPT      "Ajouter a emplacement (1-8):"
#define MSG_FAV_DELETE_PROMPT   "Supprimer emplacement (1-8):"

// ── Drive module strings ──────────────────────────────────────────────────────
#define MSG_DRIVE_SELECT_TARGET   "Choisir lecteur cible."
#define MSG_DRIVE_SELECT_UNMOUNT  "Choisir quoi demonter/ejecter."
#define MSG_DRIVE_MOUNT_FAIL      "Montage echoue."
#define MSG_DRIVE_MOUNT_DRIVE_FMT "%s sur %c."
#define MSG_DRIVE_MOUNT_TAPE_FMT  "%s pour bande."
#define MSG_DRIVE_MOUNT_ROM_FMT   "%s pour ROM."
#define MSG_DRIVE_TAPE_SEEK       "Deplace vers fich. sur bande."
#define MSG_DRIVE_PRESENT_MOUNTS  "Montages actuels:"
#define MSG_DRIVE_LBL_DRIVE       "Lect. %c: %s"
#define MSG_DRIVE_LBL_TAPE        "Bande  : %s"
#define MSG_DRIVE_LBL_ROM         "ROM    : %s"

// ── File module strings ───────────────────────────────────────────────────────
#define MSG_FILE_TARGET_SAME    "Rep. cible = rep. actif."
#define MSG_FILE_COPYMOVE_FMT   "%s %u fich. vers:"
#define MSG_FILE_VAL_COPY       "Copier"
#define MSG_FILE_VAL_MOVE       "Deplacer"
#define MSG_FILE_PROCESSING     "Traitement fichier:"
#define MSG_FILE_ESC_CANCEL     "ESC pour annuler."
#define MSG_FILE_PATH_TOO_LONG  "Chemin trop long."
#define MSG_FILE_COPY_CANCELLED "Copie annulee."
#define MSG_FILE_NOTHING_COPY   "Rien a copier."
#define MSG_FILE_OVERWRITE_Q    "Fichier existe. Ecraser?"
#define MSG_FILE_DELETE_FMT     "Effacer %u fichiers:"
#define MSG_FILE_DELETE_NONE    "Aucun fichier a effacer."
#define MSG_FILE_DELETE_Q       "Effacer fichier?"
#define MSG_FILE_RENAME_TITLE   "Renommer fichier."
#define MSG_FILE_RENAME_PROMPT  "Nouveau nom:"
#define MSG_FILE_RECURSE_TRUNCATED "Arbre trop profond, incomplet."
#define MSG_FILE_MKDIR_FAILED   "Creation repertoire echouee."

// ── Main module strings ───────────────────────────────────────────────────────
#define MSG_MAIN_EXIT_Q         "Quitter l'application."
#define MSG_MAIN_BOOT_FAILED    "Demarrage echoue."
#define MSG_MAIN_FW_FMT         "%s FW %s"
#define MSG_MAIN_ENTER_PROMPT   "Action pour Entree"
#define MSG_MAIN_FILTER_PROMPT  "Appliquer filtre"
#define MSG_MAIN_PRESS_CONTINUE "Appuyer touche pour continuer"

// Lecteur de fichiers texte
#define MSG_VIEWER_PRESS_KEY_FMT "ESPACE: suiv.  X: %s  ESC: sortir"
#define MSG_VIEWER_EOF_FMT      "X: voir %s  autre: sortir"
#define MSG_VIEWER_MODE_HEX     "hex"
#define MSG_VIEWER_MODE_TEXT    "texte"

// Version/credits popup
#define MSG_VERSION_TITLE       "Informations de version et credits"
#define MSG_VERSION_AUTHOR      "Ecrit 2025-2026 par Xander Mol"
#define MSG_VERSION_FMT         "Version: v%u.%u.%u - %s"
#define MSG_VERSION_SOURCE      "Source, doc. et credits sur :"
#define MSG_VERSION_URL         "github.com/xahmol/locifilemanager"
#define MSG_VERSION_COPYRIGHT   "(C) 2025-2026, IDreamtIn8bits.com"
#define MSG_VERSION_QR_TITLE    "Scannez pour le code source :"

// Help screen
#define MSG_HELP_TITLE1         "Aide sur les raccourcis clavier"
#define MSG_HELP_TITLE2         "Menu principal et listes"
#define MSG_HELP_TABLE \
    {"Curs Up/Do", "Deplacer haut/bas"}, \
    {"Curs Left",  "Aller au rep. parent"}, \
    {"Curs Right", "Aller au menu"}, \
    {"RETURN",     "Selec./monter/lancer"}, \
    {"ESC",        "Quitter l'application"}, \
    {". / ,",      "Lect. suiv./prec. panneau"}, \
    {"/",          "Changer de panneau"}, \
    {"\\",         "Aller a la racine"}, \
    {"D/P/T/B",    "Page bas/haut, debut/fin"}, \
    {"S/A/N/I",    "Sel. bascul/tout/aucun/inv"}, \
    {"O",          "Basculer tri"}, \
    {"F",          "Selectionner le filtre"}, \
    {"C/V",        "Copier/Deplacer fich."}, \
    {"DEL",        "Effacer fich./rep."}, \
    {"G",          "Choisir lecteur cible"}, \
    {"R",          "Renommer fich./rep."}, \
    {"M / U",      "Monter/demonter"}, \
    {"W",          "Lire la bande"}, \
    {"E",          "Creer nouv. repertoire"}, \
    {"H",          "Afficher cette aide"}, \
    {"K",          "Proprietes"}, \
    {"L",          "Filtrer fich. par nom"}, \
    {"J",          "Voir fich. texte"}, \
    {"Y",          "Favoris"}, \
    {"Cur/Esc/Rt", "Naviguer/annuler/confirmer"}

#endif
