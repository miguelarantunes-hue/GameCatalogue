# Game Catalogue

A lightweight Windows desktop app to track your game collection — built in C with SDL2.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Features

- Track games with statuses: **Wishlist, Playing, Finished, Favourites**, and more
- **List and grid views** for your collection
- **Search and sorting** support
- Multiple **themes**
- Data saved automatically to AppData

## Download

👉 [**Download the latest installer**](https://miguelarantunes-hue.github.io/GameCatalogue)

---

## ⚠️ Windows SmartScreen Warning

When you download or run the app, Windows may show a warning like:

> *"Windows protected your PC"* or *"This app is unrecognized"*

**This is expected and safe to bypass.** Here's why it happens and what to do:

### Why does this happen?
Game Catalogue is an **open-source, independent app** and is not signed with a paid code-signing certificate. Windows SmartScreen flags any executable that hasn't built up enough download history or lacks a certificate — this is common for small open-source projects and does **not** mean the app is harmful.

The full source code is publicly available in this repository for anyone to audit.

### How to bypass the warning

**On the Edge download prompt:**
1. Click the **"..." (three dots)** menu next to the download
2. Select **Keep**
3. Click **Show more** → **Keep anyway**

**On the SmartScreen popup when running the installer:**
1. Click **"More info"**
2. Click **"Run anyway"**

### Still unsure?
You can build the app yourself from source — see the [Build](#build) section below. Everything you need is in this repository.

---

## Build

**Requirements:**
- [MinGW-w64](https://www.mingw-w64.org/) (gcc + windres)
- SDL2 and SDL2_ttf development libraries

**Steps:**
```bat
:: Make sure the build/ folder exists
mkdir build

:: Compile
build.bat
```

This produces `Game Catalogue.exe` in the project root.

**To build the installer:**
```bat
installer\package.bat
```

Requires [NSIS](https://nsis.sourceforge.io/) to be installed.

---

## Folder Structure

```
├── src/              # C source code (main.c, games.c, games.h, themes.h, themes.c)
├── res/              # App resources (app.rc, icon.ico)
├── docs/             # Website and installer (index.html, GameCatalogue-Setup-Latest.exe)
├── installer/        # Packaging scripts (installer.nsi, package.bat)
├── build/            # Compiled objects (gitignored)
├── build.bat         # Build script
├── SDL2.dll          # SDL2 runtime
├── SDL2_ttf.dll      # SDL2_ttf runtime
└── DejaVuSans.ttf    # Font
```

---

## Data Storage

Game data is saved automatically to:
```
%APPDATA%\GameCatalogue\catalogue_save.bin
```

---

## Website

Hosted on GitHub Pages: [miguelarantunes-hue.github.io/GameCatalogue](https://miguelarantunes-hue.github.io/GameCatalogue)
