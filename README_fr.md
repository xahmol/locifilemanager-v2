# locifilemanager-v2

Gestionnaire de fichiers pour le périphérique de stockage de masse LOCI sur Oric Atmos — reconstruction v2 avec Oscar64.

![Interface principale de LOCI FM](screenshots/LociFM-main-interface.png)

## Sommaire

1. [Introduction](#introduction)
2. [Configuration requise](#configuration-requise)
3. [Compilation du code source](#compilation-du-code-source)
4. [Installation et démarrage du programme](#installation-et-démarrage-du-programme)
5. [Commandes clavier](#commandes-clavier)
6. [Interface principale de l'application](#interface-principale-de-lapplication)
7. [Explication des commandes](#explication-des-commandes)
8. [Historique des versions et téléchargement](#historique-des-versions-et-téléchargement)
9. [Crédits](#crédits)
10. [Licence](#licence)

---

## Introduction

locifilemanager est un gestionnaire de fichiers plein écran pour le
périphérique de stockage de masse [LOCI](https://github.com/sodiumlb/loci-rom)
sur le micro-ordinateur Oric Atmos. Il permet de parcourir, copier, déplacer,
supprimer et renommer des fichiers sur le stockage connecté au LOCI ; monter
et démonter des images de disque, de bande et de ROM ; et démarrer depuis un
média monté.

Il s'agit de la version 2 — une réécriture complète utilisant le compilateur C
[Oscar64](https://github.com/drmortalwombat/oscar64) ciblant le 6502A sans
appels ROM. La version 1 CC65 est disponible sur
[locifilemanager](https://github.com/xahmol/locifilemanager).

Disponible en anglais et en français.

**État actuel :** L'application v2 est fonctionnellement complète : elle
couvre l'ensemble des fonctionnalités de la v1, ainsi que les nouvelles
fonctionnalités v2 listées ci-dessus (copie/déplacement/suppression
récursifs, annulation de copie en cours, filtres par nom et par type,
visionneuse de texte, fenêtre de propriétés), et est
couverte par une suite de tests automatisés (`make test`). Les captures
d'écran plus loin dans ce manuel proviennent d'une version antérieure de la
v2 et datent d'avant l'ajout du menu « Tools » (Propriétés/Filtre de
texte/Voir le texte) ; elles seront mises à jour pour montrer la barre de menu
actuelle à 6 éléments.

Pour plus d'informations sur le périphérique LOCI lui-même, voir le
[manuel utilisateur du LOCI](https://github.com/sodiumlb/loci-hardware/wiki/LOCI-User-Manual)
(en anglais).

Fonctionnalités :
- Parcourir aussi bien le stockage interne du LOCI que tous les périphériques de stockage de masse USB connectés
- Deux volets de navigation avec deux répertoires chargés indépendamment
- Copier et déplacer des fichiers et des répertoires (avec tout leur contenu) entre les volets
- Supprimer et renommer des fichiers et des répertoires, y compris la suppression récursive de répertoires non vides
- Créer des répertoires
- Copier, déplacer et supprimer en fonction d'une sélection de plusieurs fichiers
- Annuler une copie de fichier en cours avec ESC ; tout fichier de destination partiel est supprimé automatiquement
- Filtrer la liste des répertoires par type de fichier, ou par un motif de nom de fichier avec caractères génériques
- Afficher le contenu des fichiers texte dans une visionneuse plein écran, paginée et avec retour à la ligne automatique
- Afficher les propriétés (type, attributs, taille) d'un fichier ou répertoire, y compris une taille totale calculée récursivement pour les répertoires
- Monter des images de disque, de bande et de ROM
- À la sortie, démarrer en fonction des images montées (disque > bande > ROM)
- Parcourir l'intérieur d'une image de bande pour sélectionner un fichier à monter/démarrer
- Interface joystick IJK prise en charge : navigation et toutes les opérations de menu possibles au joystick
- Pilotable au menu comme au clavier
- Code source entièrement public, inclut une bibliothèque API LOCI pour vos propres projets

---

## Configuration requise

### Matériel

- Oric Atmos
- Périphérique de stockage de masse [LOCI](https://github.com/sodiumlb/loci-rom)
- Stockage USB connecté au LOCI

### Firmware LOCI

- Version minimale du firmware LOCI : 0.2.5
- Le firmware 0.3.0 ou supérieur est nécessaire pour pouvoir créer des répertoires

### Pour les tests sur émulateur

- Émulateur [Oricutron](http://www.defence-force.org/index.php?page=articles&art=oricutron)

> **Remarque :** Les fonctionnalités matérielles LOCI (accès aux fichiers,
> liste des répertoires, montage/démontage) nécessitent un véritable
> périphérique LOCI. Oricutron n'émule pas le LOCI.

### Pour la compilation du code source

| Outil | Rôle | Installation |
|---|---|---|
| [Oscar64](https://github.com/drmortalwombat/oscar64) | Compilateur C pour 6502 | Compiler depuis les sources |
| Python 3 | Création des images bande (`mktap.py`) | `sudo apt install python3` |
| pandoc | Génération de la documentation PDF | `sudo apt install pandoc` |
| zip | Archive de distribution | `sudo apt install zip` |

---

## Compilation du code source

```sh
git clone https://github.com/xahmol/locifilemanager-v2
cd locifilemanager-v2
```

| Cible | Action |
|---|---|
| `make` | Compiler la version anglaise (`build/locifm.tap`) |
| `make LANG=FR` | Compiler la version française (`build/locifm_fr.tap`) |
| `make all-langs` | Compiler les quatre images bande (EN+FR pour l'app et la démo) |
| `make run` | Compiler + lancer l'app EN dans Oricutron |
| `make libdemo` | Compiler la démo de bibliothèque (`build/libdemo.tap`) |
| `make libdemo-run` | Compiler + lancer la démo EN dans Oricutron |
| `make docs` | Régénérer la documentation PDF depuis Markdown |
| `make zip` | Compiler + docs + créer le ZIP de distribution |
| `make usb` | Copier toutes les images bande sur clé USB (voir ci-dessous) |
| `make clean` | Supprimer les fichiers de compilation |

### Transfert sur clé USB pour le matériel réel

Pour copier les images bande directement sur une clé USB :

1. Copiez `.env.example` vers `.env` (`.env` est ignoré par git) :
   ```sh
   cp .env.example .env
   ```
2. Éditez `.env` et définissez `USBPATH` avec le chemin du répertoire cible
   sur la clé USB :

   **Linux natif** — chemin sous `/media/` :
   ```
   USBPATH = /media/votrelogin/CLUSB/oric
   ```

   **WSL2 (Windows Subsystem for Linux 2)** — aucun outil supplémentaire
   requis. Windows attribue une lettre de lecteur à la clé USB (ex. `E:`) ;
   WSL2 monte automatiquement tous les lecteurs Windows sous `/mnt/<lettre>` :
   ```
   USBPATH = /mnt/e/oric
   ```

3. Branchez la clé USB puis exécutez :
   ```sh
   make usb
   ```
   Cette commande compile les quatre images bande et les copie vers `USBPATH`.

---

## Installation et démarrage du programme

1. Copiez `locifm.tap` (anglais) ou `locifm_fr.tap` (français) sur un
   emplacement de la clé USB connectée à votre périphérique LOCI.
2. Allez dans l'interface utilisateur du LOCI en appuyant sur le bouton rouge
   du LOCI. Vous devriez obtenir une interface LOCI similaire à :

   ![Interface LOCI - démarrage du programme](screenshots/LociFM-start-program.png)
3. Allez dans le champ de sélection des images de bande en appuyant sur **T**
   (ou naviguez-y avec les touches curseur).
4. Appuyez sur **ESPACE** pour aller au navigateur de fichiers.
5. Naviguez jusqu'à l'emplacement de la clé USB où vous avez stocké
   `locifm.tap` (ou `locifm_fr.tap`).
6. Sélectionnez-le avec **ESPACE**.
7. Démarrez l'application en appuyant sur **ESC**.
8. Si l'option de chargement automatique du LOCI est activée, le chargement
   depuis la bande démarre automatiquement. Sinon, tapez `CLOAD""` et appuyez
   sur **RETURN**.
9. Le programme devrait maintenant se charger et démarrer.

Si aucun LOCI n'est détecté, ou si le LOCI connecté a une version de firmware
inférieure à celle requise (voir [Configuration requise](#configuration-requise)),
le programme affiche un message d'erreur et se termine.

Pour savoir comment utiliser votre périphérique LOCI, voir le
[manuel utilisateur du LOCI](https://github.com/sodiumlb/loci-hardware/wiki/LOCI-User-Manual)
(en anglais).

---

## Commandes clavier

### Dans le volet du navigateur de fichiers :

|Touche|Description
|---|---|
|**Curseur Haut / Bas**|Se déplacer vers le bas ou le haut dans le volet du navigateur de fichiers
|**Curseur Gauche**|Aller au répertoire parent
|**Curseur Droite**|Aller au menu principal
|**RETURN**|Exécuter l'action choisie pour la touche RETURN : sélectionner, monter ou lancer un fichier.
|**ESC**|Quitter l'application et démarrer sur la (les) image(s) montée(s)
|**.**|Aller au prochain lecteur LOCI disponible pour le volet actif
|**,**|Aller au lecteur LOCI précédent disponible pour le volet actif
|**/**|Basculer le volet actif vers l'autre volet
|**\\**|Aller au répertoire racine du lecteur du volet actif
|**D**|Page suivante (**d**own) dans le volet du navigateur de fichiers
|**P**|Page précédente (u**p**) dans le volet du navigateur de fichiers
|**T**|Aller en haut (**t**op) : première entrée du volet du navigateur de fichiers
|**B**|Aller en bas (**b**ottom) : dernière entrée du volet du navigateur de fichiers
|**S**|Basculer la **s**élection du fichier (sélectionné ou non)
|**A**|Sélectionner **t**ous les fichiers du volet actif
|**N**|Désélectionner tous les fichiers (aucun = **n**one) du volet actif
|**I**|**I**nverser la sélection dans le volet actif
|**O**|Activer/désactiver le tri alphabétique des répertoires (s**o**rt)
|**F**|Choisir le **f**iltre à appliquer pour l'affichage des entrées du répertoire, ou désactiver le filtrage.
|**C**|**C**opier le fichier présent ou tous les fichiers sélectionnés du répertoire du volet actif vers le répertoire du volet non actif
|**V**|Déplacer (mo**v**e) le fichier présent ou tous les fichiers sélectionnés du répertoire du volet actif vers le répertoire du volet non actif
|**DEL**|Supprimer (**del**ete) le fichier ou répertoire présent (récursivement pour les répertoires non vides, après confirmation)
|**G**|Choisir le lecteur cible (**g**) pour le montage des images disque
|**R**|**R**enommer le fichier ou répertoire présent
|**M**|**M**onter le fichier présent comme disque, bande ou ROM
|**U**|Choisir le lecteur, la bande ou la ROM à démonter (**u**nmount)
|**W**|Démarrer la navigation (bro**w**se) à l'intérieur d'une image de bande à partir du fichier .TAP présent
|**E**|Créer un nouveau (**e**) répertoire
|**H**|Afficher l'écran d'aide (**h**elp) des commandes clavier
|**K**|Afficher les propriétés (type, attributs, taille) du fichier ou répertoire présent
|**L**|Définir ou effacer un fi**l**tre de nom de fichier (caractères génériques) pour les deux volets
|**J**|Afficher le fichier texte présent dans une visionneuse plein écran avec retour à la ligne automatique

### Dans le menu principal et les menus déroulants

|Touche|Description
|---|---|
|**Curseur Haut / Bas**|Se déplacer vers le bas ou le haut dans les options du menu déroulant
|**Curseur Gauche / Droite**|Aller à gauche ou à droite dans le menu principal
|**RETURN**|Sélectionner l'option
|**ESC**|Annuler (si autorisé)

### Correspondance des directions du joystick avec les commandes clavier

Les directions gauche, droite, bas et haut du joystick sont traduites vers les
touches curseur correspondantes et déclenchent les mêmes commandes.
Le bouton de tir déclenche la même action que la touche RETURN.

---

## Interface principale de l'application

Après le démarrage de l'application, l'interface principale ressemble à :

![Interface principale de LOCI FM](screenshots/LociFM-main-interface.png)

La première ligne de l'écran est un en-tête affichant le nom de l'application,
suivi de la chaîne d'identification et de version de firmware du périphérique
LOCI connecté.

La deuxième ligne est la barre de menu principale. On y accède en appuyant sur
la touche **Curseur Droite** ou en poussant le joystick vers la droite.

En dessous se trouvent deux volets, représentant chacun un répertoire sur un
lecteur de stockage fourni par le LOCI. Le volet affiché en jaune est le volet
actif, en blanc le volet non actif.
La première ligne de chaque volet est le nom d'identification du lecteur dont
un répertoire est affiché, la deuxième ligne montre le chemin actif du
répertoire affiché, dont le premier nombre est l'identifiant du lecteur. 0 est
le stockage interne, 1 à 9 sont les périphériques de stockage de masse USB
connectés au LOCI.

En dessous se trouvent les entrées du répertoire, affichées par pages de 10.
D'abord le nom, puis à droite le type :
- DSK : image disque
- TAP : image bande
- ROM : image ROM
- LCE : exécutable LOCI (pas encore implémenté)
- DIR : répertoire
- espaces : type inconnu

Dans le volet actif, l'entrée présente est mise en évidence en cyan. Elle peut
être déplacée vers le haut ou le bas avec les touches curseur ou le joystick.
La page suivante/précédente est également prise en charge, ainsi que l'aller
au début ou à la fin.
Toutes les commandes s'exécutent sur la base des fichiers présents/sélectionnés.
Voir la section [Commandes clavier](#commandes-clavier) ci-dessus pour les
commandes clavier disponibles, et [Explication des commandes](#explication-des-commandes)
pour une explication de ce que font toutes les commandes et comment les
utiliser, classées par option du menu principal.

---

## Explication des commandes

### App : Options de l'application

![Menu Application](screenshots/LociFM-menu-app.png)

*Confirm.*

Confirm. est un commutateur qui détermine si une fenêtre « Êtes-vous sûr ? »
est affichée pour confirmer une suppression ou une copie avec écrasement, soit
une seule fois, soit pour chaque fichier individuellement. Par défaut : une
seule fois (1x).

Cette fonction n'est accessible que via le menu.

*Retour*

![App : Retour](screenshots/LociFM-menu-app-return.png)

Sélectionne l'action à effectuer en appuyant sur RETURN :
- Select : Bascule la sélection du fichier en appuyant sur **RETURN**, qui se
  comporte alors comme la touche **S**. Option par défaut.
- Entrer (Mount) : Le fichier est monté en appuyant sur **RETURN**, qui se
  comporte alors comme la touche **M**.
- Lancer (Launch) : Le fichier est lancé en appuyant sur **RETURN**. Cela
  signifie que tous les autres montages sont démontés, le fichier présent est
  monté, et l'application se termine pour démarrer sur ce montage.
  **RETURN** se comporte dans ce cas comme un appui sur la touche **U** pour
  chaque montage actif, suivi de la touche **ESC**.

Cette fonction n'est accessible que via le menu.

*Filtre de type*

![App : Filtre](screenshots/LociFM-menu-app-filter.png)

Sélectionne le filtre à appliquer pour l'affichage des répertoires. Le menu
affiche directement la valeur active (`[F] Type: Aucun`/`.DSK`/`.TAP`/`.ROM`/`.LCE`) :
- Aucun : aucun filtre appliqué
- .DSK : seules les images disque .DSK sont affichées
- .TAP : seules les images bande .TAP sont affichées
- .ROM : seules les images ROM .ROM sont affichées
- .LCE : seuls les exécutables LOCI sont affichés (pas encore implémenté, le
  format .LCE reste à définir)

Cette fonction est aussi accessible avec la touche **F**.

*Tri*

Active ou désactive le tri des entrées du répertoire.

**NB** : le tri peut être très lent sur de grands répertoires, attention donc
en l'activant sur ces répertoires volumineux. Pour cette raison, il est
désactivé par défaut.

Cette fonction est aussi accessible avec la touche **O**.

*Sortie*

![App : Sortie](screenshots/LociFM-menu-app-exit.png)

L'application se termine et démarre en fonction des montages actifs (lecteur
A > .TAP > ROM).

Le démarrage automatique depuis les images .TAP est activé.

Cette fonction est aussi accessible avec la touche **ESC**.

### Fichier : Opérations sur les fichiers

![Menu Fichier](screenshots/LociFM-menu-file.png)

*Sélection*

Bascule la sélection du fichier actuellement mis en évidence. Les fichiers
sélectionnés sont affichés avec un - devant leur nom, comme dans la capture
d'écran ci-dessous.

Cette fonction est aussi accessible avec la touche **S**.

![Fichier : Sélection](screenshots/LociFM-menu-file-select.png)

*Tout sélectionner*

Sélectionne tous les fichiers du volet présent. Cette fonction est aussi
accessible avec la touche **A**.

*Tout désélectionner*

Désélectionne tous les fichiers du volet présent. Cette fonction est aussi
accessible avec la touche **N**.

*Inverser la sélection*

Inverse la sélection des fichiers du volet présent. Cette fonction est aussi
accessible avec la touche **I**.

*Effacer*

Supprime le fichier ou répertoire présent. Ou, si une sélection de fichiers a
été faite, tous les fichiers sélectionnés.

Une fenêtre demande confirmation (selon le réglage Confirm. du menu App, une
seule fois ou pour chaque fichier), après quoi la suppression a lieu. Appuyer
sur une touche pour revenir.

Cette fonction permet aussi de supprimer un répertoire :
- Si le répertoire est vide, il est supprimé directement après la confirmation
  habituelle.
- Si le répertoire n'est pas vide, une confirmation supplémentaire « Rep. non
  vide. Tout effacer? » s'affiche. Confirmer supprime récursivement tout le
  contenu du répertoire (fichiers et sous-répertoires) puis le répertoire
  lui-même ; refuser laisse le répertoire intact.
- Les répertoires ne peuvent pas être sélectionnés, leur suppression doit donc
  se faire un par un.
- Si l'arborescence dépasse 8 niveaux de profondeur, le(s) niveau(x) le(s) plus
  profond(s) peuvent ne pas être supprimés ; un message l'indique.

Cette fonction est aussi accessible avec la touche **DEL**.

![Fichier : Effacer](screenshots/LociFM-menu-file-delete.png)

*Renommer*

Renomme le fichier ou répertoire présent.

Une fenêtre apparaît pour modifier le nom. Les touches curseur et DEL sont
prises en charge pour l'édition. Appuyez sur RETURN pour valider, ESC pour
annuler.

Cette fonction est aussi accessible avec la touche **R**.

![Fichier : Renommer](screenshots/LociFM-menu-file-rename.png)

*Copier / Déplacer*

Copie ou déplace le fichier ou répertoire présent. Ou, si une sélection de
fichiers et/ou répertoires a été faite, toutes les entrées sélectionnées. Les
répertoires sont copiés ou déplacés récursivement, avec tous les fichiers et
sous-répertoires qu'ils contiennent ; si le répertoire cible existe déjà, son
contenu est fusionné.

La copie ou le déplacement s'effectue depuis le répertoire du volet actif vers
le répertoire du volet non actif. Vous ne pouvez pas copier ou déplacer vers
le même répertoire ; dans ce cas un message d'erreur s'affiche :

![Fichier : Copier - erreur même répertoire](screenshots/LociFM-menu-file-copy-same-dir.png)

Si un ou plusieurs fichiers du même nom existent déjà, une fenêtre demande
confirmation pour écraser les fichiers du répertoire cible (selon le réglage
Confirm. du menu App, une seule fois ou pour chaque fichier).

![Fichier : Copier - confirmation d'écrasement](screenshots/LociFM-menu-file-copy-confirm.png)

Sinon, une fenêtre affiche la progression de la copie ou du déplacement.
Appuyer sur **ESC** annule immédiatement, même en cours de copie d'un fichier ;
tout fichier de destination partiellement écrit est supprimé automatiquement.

![Fichier : Copier](screenshots/LociFM-menu-file-copy.png)

Cette fonction est aussi accessible avec la touche **C** pour copier ou **V**
pour déplacer.

Lors de la copie ou du déplacement d'une arborescence de plus de 8 niveaux de
profondeur, le(s) niveau(x) le(s) plus profond(s) peuvent rester incomplets ;
un message l'indique.

*Parcourir une bande*

Permet de parcourir l'intérieur d'une image de bande .TAP. Ne fonctionne
évidemment que si l'entrée de répertoire actuellement mise en évidence est de
type TAP.

La sélection commence par monter l'image de bande. Cette fenêtre s'affiche,
appuyez sur une touche pour continuer :

![Fichier : Parcourir bande - message de montage](screenshots/LociFM-menu-file-browse-tape-mount.png)

Le volet actif du navigateur ressemble alors à :

![Fichier : Parcourir bande](screenshots/LociFM-menu-file-browse-tape-browse.png)

La première ligne affiche désormais « Ruban : » pour indiquer que vous êtes à
l'intérieur d'une image de bande, suivi du nom de l'image. La deuxième ligne
reste le chemin actif vers l'image de bande.

Les entrées de répertoire affichent maintenant les fichiers contenus dans
l'image de bande, avec d'abord le nom (le cas échéant), suivi de l'adresse de
départ et de la taille, et enfin le type de bande détecté : BAS pour un
fichier BASIC, BIN pour tous les autres.

À l'intérieur d'une bande, les fonctions copier, supprimer et renommer ne
fonctionnent pas (impossible, ou pas encore implémenté). La sélection de
fichiers ne fonctionne donc pas non plus.

La seule chose qui fonctionne en parcourant une bande est de se positionner
sur le fichier souhaité à l'intérieur de la bande en appuyant sur **RETURN**.
Cela affiche cette fenêtre :

![Fichier : Parcourir bande - se positionner sur un fichier](screenshots/LociFM-menu-file-browse-tape-mount-file-on-tape.png)

Revenez au répertoire normal en appuyant sur **Curseur Gauche** (ou joystick
gauche).

Cette fonction est aussi accessible avec la touche **W**.

### Répertoire : Navigation et opérations sur les répertoires

![Menu Répertoire](screenshots/LociFM-menu-dir.png)

*Aller à la racine*

Va au répertoire racine du lecteur du volet actif. Cette fonction est aussi
accessible avec la touche **\\**.

*Retour*

Va au répertoire parent du lecteur du volet actif. Cette fonction est aussi
accessible avec la touche **Curseur Gauche**.

*Page suivante*

Va à la page suivante d'entrées du volet actif. Cette fonction est aussi
accessible avec la touche **D**.

*Page précédente*

Va à la page précédente d'entrées du volet actif. Cette fonction est aussi
accessible avec la touche **P**.

*Début*

Va à la première entrée du volet actif. Cette fonction est aussi accessible
avec la touche **T**.

*Fin*

Va à la dernière entrée du volet actif. Cette fonction est aussi accessible
avec la touche **B**.

*Nouveau répertoire*

Crée un nouveau répertoire dans le volet actif, à partir du chemin actif.

Une fenêtre s'affiche pour saisir le nom du répertoire. Saisissez le nom (les
touches curseur et DEL peuvent être utilisées pour l'édition), appuyez sur
**RETURN** pour valider et **ESC** pour annuler.

Cette fonction est aussi accessible avec la touche **E**.

![Répertoire : Nouveau répertoire](screenshots/LociFM-menu-dir-create-dir.png)

### Outils : propriétés, filtre par nom et visionneuse de texte

*Propriétés*

Affiche une fenêtre avec les détails du fichier ou répertoire présent :
- Nom, type et chemin actif. Le type est affiché sous forme de description :
  "Repertoire" pour les répertoires, ".DSK - Image disque", ".TAP - Image
  bande", ".ROM - Image ROM" ou ".LCE - Executable LOCI" pour ces types de
  fichiers, et l'extension propre du fichier (par ex. ".TXT") pour tout autre
  fichier
- Attributs : R (lecture seule) et S (système), affichés par un tiret (-) si absents
- Taille en octets. Pour un répertoire, la taille est calculée récursivement
  sur tous les fichiers de son arborescence ; pendant le calcul, "Calcul en
  cours..." s'affiche, et appuyer sur **ESC** annule le calcul, après quoi
  "Annulé." s'affiche à la place d'une taille. Si l'arborescence dépasse 8
  niveaux de profondeur, le total est affiché avec un "+" final pour indiquer
  qu'il peut être incomplet.

Appuyer sur une touche pour fermer la fenêtre.

Cette fonction est aussi accessible avec la touche **K**.

*Filtre de texte*

Ouvre une fenêtre pour saisir un motif avec caractères génériques (`*` et `?`,
insensible à la casse) qui filtre la liste des répertoires des deux volets par
nom de fichier. Les répertoires sont toujours affichés quel que soit le motif,
afin que la navigation ne soit jamais bloquée.

Saisir un motif vide pour effacer le filtre. Appuyer sur **RETURN** pour
appliquer, **ESC** pour annuler sans modification. Ce filtre n'est pas
mémorisé entre les redémarrages.

Le menu Outils affiche `[L] Texte: Oui` ou `[L] Texte: Non` selon qu'un motif
est actif ou non ; le motif lui-même est affiché sur la ligne « Actuel: » de
cette fenêtre.

Cette fonction est aussi accessible avec la touche **L**.

*Voir le texte*

Ouvre le fichier présent dans une visionneuse de texte plein écran avec retour
à la ligne automatique. Appuyer sur **ESPACE** (ou toute autre touche) pour
passer à la page suivante, ou **ESC** pour revenir au navigateur de fichiers.
La pagination se fait uniquement vers l'avant.

Tout octet qui n'est pas un caractère ASCII imprimable (0x20-0x7E) -- par
exemple lors de l'affichage d'un fichier binaire/non-texte -- est affiché
comme un caractère de remplacement `.`, afin que le contenu binaire
s'affiche sans corrompre l'écran ni tronquer la ligne.

Cette fonction est aussi accessible avec la touche **J**.

### Info : Informations sur la version et aide

![Menu Info](screenshots/LociFM-menu-info.png)

*Version/crédits*

Affiche deux écrans pleins successifs. Le premier montre le logo "I Dream in
8 Bits" avec le nom de l'application, la version (horodatage de compilation
`vMAJOR.MINOR.PATCH - AAAAMMJJ-HHMMSS`) et les crédits ; le second montre un
code QR menant vers la page GitHub du projet. Appuyer sur une touche pour
passer à l'écran suivant, puis à nouveau pour revenir à l'application.

![Info : Version](screenshots/LociFM-menu-info-version.png)

*Aide*

Affiche un écran d'aide pour les commandes clavier.

![Info : Aide](screenshots/LociFM-menu-info-help.png)

---

## Historique des versions et téléchargement

| Version | Date | Notes |
|---|---|---|
| 2.0.0 | 2026 | Reconstruction Oscar64 — en développement |

### Nouveautés de la v1 à la v2

La v2 est une reconstruction complète (Oscar64, bare-metal) de la version 1
(CC65). Pour les utilisateurs venant de la v1, les principaux changements
sont :

- Les répertoires (et plus seulement les fichiers) peuvent désormais être
  copiés et déplacés, avec tout leur contenu
- La suppression d'un répertoire non vide propose désormais de supprimer
  récursivement tout son contenu, au lieu d'être refusée
- Une copie ou un déplacement peut être annulé en cours de transfert avec
  **ESC** ; tout fichier de destination partiel est automatiquement supprimé
- Un nouveau filtre par motif de nom de fichier (**L**) complète le filtre
  par type existant
- Une nouvelle visionneuse de fichiers texte plein écran et avec retour à la
  ligne automatique (**J**)
- Une nouvelle fenêtre de propriétés (**K**) affichant le type, les attributs
  et la taille, avec une taille totale calculée récursivement pour les
  répertoires
- Disponible en anglais et en français
- Nouveaux écrans de démarrage affichant les informations de version et un
  QR code GitHub

### Versions précédentes (v1, CC65)

La version 1 (CC65) reste disponible sur
[locifilemanager](https://github.com/xahmol/locifilemanager) :

- https://github.com/xahmol/locifilemanager/raw/refs/heads/main/BUILD/locifm.tap

| Version | Notes |
|---|---|
| 0.1.2 | Ajout de la fonction déplacer |
| 0.1.1 | Corrections de bugs sur la navigation dans les bandes et la préférence de démarrage |
| 0.1.0 | Première version bêta complète |

---

## Crédits

LOCI File Manager v2
Gestionnaire de fichiers pour le périphérique de stockage de masse LOCI pour Oric Atmos
Écrit en 2025-2026 par Xander Mol

https://github.com/xahmol/locifilemanager-v2

https://www.idreamtin8bits.com/

Pour des informations et de la documentation sur le périphérique de stockage
de masse LOCI :
-   Manuel utilisateur du LOCI

    https://github.com/sodiumlb/loci-hardware/wiki/LOCI-User-Manual
-   Vendeur de mon périphérique LOCI assemblé :
    - Raxiss : https://www.raxiss.com/article/id/38-LOCI

Code et ressources d'autres auteurs utilisés :
-   LOCI ROM par Sodiumlightbaby, 2024

    https://github.com/sodiumlb/loci-rom

-   Compilateur croisé Oscar64 par drmortalwombat :

    https://github.com/drmortalwombat/oscar64

-   LOCI File Manager v1 (CC65), référence fonctionnelle pour la v2 :

    https://github.com/xahmol/locifilemanager

-   Compilateur croisé CC65, utilisé par la v1 :

    https://cc65.github.io/

-   Code source DraBrowse pour son inspiration et sa routine de saisie de texte
    DraBrowse (db*) est un simple navigateur de fichiers.
    Créé à l'origine en 2009 par Sascha Bader.
    Version utilisée adaptée par Dirk Jagdmann (doj)

    https://github.com/doj/dracopy

-   lib-ijk-egoist d'oricOpenLibrary (pour le support joystick via l'interface
    Raxiss IJK)
    Par Raxiss, (c) 2021

    https://github.com/iss000/oricOpenLibrary/tree/main/lib-ijk-egoist

-   forum.defence-force.org : pour son inspiration et ses conseils pendant le
    développement.

-   Code source du système de fenêtrage original sur Commodore 128, auteur
    inconnu.

-   Phosphoric, émulateur ORIC-1/Atmos cycle-exact par Xander Mol, utilisé
    pour la suite de tests automatisés (`make test`)

    https://github.com/xahmol/Phosphoric

-   Testé sur matériel réel Oric Atmos avec LOCI

Le code peut être utilisé librement à condition de conserver une mention
décrivant la source et l'auteur d'origine.

CES PROGRAMMES SONT DISTRIBUÉS DANS L'ESPOIR QU'ILS SERONT UTILES, MAIS SANS
AUCUNE GARANTIE. UTILISEZ-LES À VOS PROPRES RISQUES !

---

## Licence

Copyright (C) 2025-2026 Xander Mol.

Ce programme est un logiciel libre : vous pouvez le redistribuer et/ou le
modifier selon les termes de la Licence Publique Générale GNU publiée par la
Free Software Foundation, version 3.

Consultez le fichier [LICENSE](LICENSE) pour le texte complet de la licence.
