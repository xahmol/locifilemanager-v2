# locifilemanager-v2

Gestionnaire de fichiers pour le périphérique de stockage de masse LOCI sur Oric Atmos — reconstruction v2 avec Oscar64.

## Sommaire

1. [Introduction](#introduction)
2. [Configuration requise](#configuration-requise)
3. [Compilation du code source](#compilation-du-code-source)
4. [Installation](#installation)
5. [Utilisation](#utilisation)
6. [Historique des versions et téléchargement](#historique-des-versions-et-téléchargement)
7. [Licence](#licence)

---

## Introduction

locifilemanager est un gestionnaire de fichiers plein écran pour le périphérique
de stockage de masse [LOCI](https://github.com/sodiumlb/loci-rom) sur le
micro-ordinateur Oric Atmos. Il permet de parcourir, copier, déplacer, supprimer
et renommer des fichiers sur le stockage connecté au LOCI ; monter et démonter
des images de disque, de bande et de ROM ; et démarrer depuis un média monté.

Il s'agit de la version 2 — une réécriture complète utilisant le compilateur C
[Oscar64](https://github.com/drmortalwombat/oscar64) ciblant le 6502A sans appels
ROM. La version 1 CC65 est disponible sur
[locifilemanager](https://github.com/xahmol/locifilemanager).

Disponible en anglais et en français.

**État actuel :** L'infrastructure des bibliothèques (bibliothèque de fenêtres
charwin, scanner de clavier, API LOCI complète) est finalisée et testée. La
phase 4 — reconstruction de l'application — est en cours.

---

## Configuration requise

### Matériel

- Oric Atmos
- Périphérique de stockage de masse [LOCI](https://github.com/sodiumlb/loci-rom)
- Stockage USB connecté au LOCI

### Pour les tests sur émulateur

- Émulateur [Oricutron](http://www.defence-force.org/index.php?page=articles&art=oricutron)

> **Remarque :** Les fonctionnalités matérielles LOCI (accès aux fichiers,
> liste des répertoires, montage/démontage) nécessitent un véritable périphérique
> LOCI. Oricutron n'émule pas le LOCI.

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

Pour copier les images bande directement sur une clé USB montée :

1. Copiez `.env.example` vers `.env` (`.env` est ignoré par git) :
   ```sh
   cp .env.example .env
   ```
2. Éditez `.env` et définissez `USBPATH` en indiquant le répertoire de votre clé
   USB monté où les fichiers `.tap` doivent être copiés :
   ```
   USBPATH = /media/votrelogin/CLUSB/oric
   ```
3. Montez la clé USB, puis exécutez :
   ```sh
   make usb
   ```
   Cette commande compile les quatre images bande et les copie vers `USBPATH`.

---

## Installation

1. Copiez `locifm.tap` (anglais) ou `locifm_fr.tap` (français) sur votre
   stockage USB LOCI.
2. Sur l'Oric, chargez depuis l'invite LOCI ou via BASIC :
   ```
   CLOAD""
   ```

---

## Utilisation

*(Reconstruction de l'application en cours — cette section sera complétée en phase 4.)*

L'application fournit :

- Navigateur de répertoires à deux volets
- Opérations sur les fichiers : copier, déplacer, supprimer, renommer
- Navigateur d'images bande
- Gestion des lecteurs : montage/démontage d'images disque, bande et ROM
- Démarrage depuis les médias montés (priorité disque > bande > ROM)
- Support du joystick IJK (interface Raxiss IJK)
- Interface en anglais et en français

---

## Historique des versions et téléchargement

| Version | Date | Notes |
|---|---|---|
| 2.0.0 | 2026 | Reconstruction Oscar64 — en développement |
| 1.x | 2025 | [Version CC65](https://github.com/xahmol/locifilemanager) |

---

## Licence

Copyright (C) 2025-2026 Xander Mol.

Ce programme est un logiciel libre : vous pouvez le redistribuer et/ou le
modifier selon les termes de la Licence Publique Générale GNU publiée par la
Free Software Foundation, version 3.

Consultez le fichier [LICENSE](LICENSE) pour le texte complet de la licence.
