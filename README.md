# 🌱 Farming Game

A 2D farming simulation game built from scratch in C using SDL2. Plant seeds, water crops, harvest produce, and discover hybrid plant varieties through a dynamic breeding system.

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-brightgreen)
![License](https://img.shields.io/badge/license-MIT-yellow)

---

## Gameplay

- **Plant** seeds on empty soil tiles
- **Water** your crops using watering mode
- **Advance the day** to grow your plants
- **Harvest** fully grown crops to collect seeds and produce
- **Sell** your harvest in the shop for money
- **Buy** new plant varieties — Blueberry, Pumpkin, Corn
- **Breed** adjacent crops to discover unique hybrid plants with generated names, colors, and stats

### Plant Varieties

| Plant | Growth Speed | Sell Price |
|-------|-------------|------------|
| Wheat | Fast | $20 |
| Tomato | Medium | $30 |
| Carrot | Fast | $25 |
| Blueberry | Slow | $40 |
| Pumpkin | Medium | $35 |
| Corn | Fast | $22 |

Hybrids are generated dynamically — their names, colors, and stats are blended from their parents. No two playthroughs are the same.

---

## Controls

| Input | Action |
|-------|--------|
| Left click (normal mode) | Plant seed / Harvest grown crop |
| Left click (watering mode) | Water tile |
| Right click | Advance day |
| Scroll wheel | Cycle through seed types |
| S key | Save game |
| Save button | Save game |
| Next Day button | Advance day |
| Water button | Toggle watering mode |
| Shop button | Open shop |

---

## Download & Play

1. Go to the [Releases](../../releases) page
2. Download `FarmingGame-v1.0.zip`
3. Extract the zip anywhere
4. Double-click `farming_game.exe`

No installation required. Windows only.

---

## Building from Source

### Requirements
- [MSYS2](https://www.msys2.org) with UCRT64 environment
- GCC
- SDL2
- SDL2_ttf

### Install dependencies
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-SDL2
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
```

### Compile
```bash
gcc src/main.c src/tile.c src/farm.c -o farming_game.exe -I/c/msys64/ucrt64/include/SDL2 -L/c/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -mwindows
```

---

## Built With

- **C** — core language
- **SDL2** — window, rendering, input
- **SDL2_ttf** — font rendering

---

## About

This game was built as a hands-on learning project for C programming — covering pointers, structs, manual memory management, multi-file architecture, and SDL2 game development from scratch.
