# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```sh
sh autogen.sh
./configure
make
```

To rebuild after changing `configure.ac`:
```sh
autoconf
./configure
make
```

Run a single object recompile (no autoconf needed):
```sh
make -f system.mk foo.o
```

Install after build:
```sh
make install
```

Clean:
```sh
make clean       # removes .o and binary
make reallyclean # also removes config.h, system.mk, configure outputs
```

### SDL2 build (macOS / any SDL2 platform)

Install dependencies via Homebrew:

```sh
brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
```

Then build with the SDL2 backend:

```sh
sh autogen.sh
./configure --with-backend=sdl2
make
```

### X11 build (Linux)

```sh
sudo apt install libx11-dev libxt-dev libxxf86vm-dev \
    libimlib2-dev \
    libsdl-mixer1.2-dev libsdl1.2-dev
sh autogen.sh
./configure --with-backend=x11
make
```

### macOS app bundle

Build a self-contained `Netrek.app` that can be launched by double-clicking:

```sh
brew install dylibbundler   # needed to embed SDL2 dylibs
make bundle                  # builds Netrek.app in project root
open Netrek.app
```

Without `dylibbundler` the bundle still runs on the build machine (since SDL2 is available via Homebrew) but won't run on machines without SDL2 installed.

The bundle layout:
- `Contents/MacOS/Netrek` — wrapper shell script (sets CWD → Resources before launching the binary)
- `Contents/MacOS/netrek-client-cow` — actual game binary
- `Contents/Frameworks/` — embedded SDL2/SDL2_image/SDL2_ttf/SDL2_mixer dylibs (added by dylibbundler)
- `Contents/Resources/pixmaps/` — ship/planet images
- `Contents/Resources/sdl2/fonts/` — bundled monospace font
- `macos/Info.plist` — bundle metadata template

### Tests

Unit tests use the Unity framework with SDL2's headless dummy driver. No display required.

```sh
cd tests
make check
```

To run a single test binary:

```sh
cd tests
make test_drawing
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ../test_drawing   # from project root (font path)
```

All 6 test suites must pass: `test_event_pipe`, `test_event_translation`, `test_drawing`, `test_window_mgmt`, `test_sprite`, `test_text`.

The `spike/` directory contains standalone proof-of-concept programs (SDL sound, SDL mixer) that are built manually, not by the main Makefile.

## Architecture

### The Wlib abstraction layer

Every platform port implements the same ~50 windowing functions declared in `Wlib.h`. All game logic calls only `W_*` functions — it never touches X11, Win32, or any platform API directly. `Wlib.h` defines:

- Window lifecycle: `W_Initialize`, `W_MakeWindow`, `W_MakeTextWindow`, `W_MakeScrollingWindow`, `W_MakeMenu`, `W_DestroyWindow`, `W_MapWindow`, `W_UnmapWindow`, `W_Deinitialize`
- Drawing: `W_WriteText`, `W_WriteBitmap`, `W_OverlayBitmap`, `W_MakeLine`, `W_MakePhaserLine`, `W_MakeTractLine`, `W_WriteCircle`, `W_WriteTriangle`, `W_FillArea`, `W_ClearArea`, `W_ClearWindow`
- Events: `W_NextEvent`, `W_EventsPending`, `W_EventsQueued`, `W_SpNextEvent` — events are typed as `W_EV_EXPOSE`, `W_EV_KEY`, `W_EV_BUTTON`, `W_EV_CLOSED`
- Fonts: `W_BigFont`, `W_RegularFont`, `W_HighlightFont`, `W_UnderlineFont` (global `W_Font` handles)
- Colors: `W_White`, `W_Black`, `W_Red`, `W_Green`, `W_Yellow`, `W_Cyan`, `W_Grey` (global `W_Color` handles)
- Icons/sprites: `W_StoreBitmap`
- Misc: `W_Beep`, `W_Flush`, `W_Socket`, `W_Mono`, `W_FullScreenToggle`

`W_Window`, `W_Icon`, `W_Font`, and `W_Color` are all `typedef char *` or `typedef int` — opaque handles cast from internal structs.

### Platform backends

| File | Platform | Notes |
|---|---|---|
| `x11/x11window.c` + `x11/x11sprite.c` + `x11/audio.c` | X11/Linux | |
| `win32/mswindow.c` + `win32/winsprite.c` + `win32/winsndlib.c` | Windows | |
| `sdl2/sdl2window.c` + `sdl2/sdl2sprite.c` | SDL2 (macOS/Linux/Windows) | SDL2_ttf for fonts, SDL2_image for sprites |

The build system selects backends via `system.mk.in`. Each backend is gated by an `@NO<BACKEND>@` substitution that resolves to `#` (commenting out the object list) when that backend is inactive. `configure --with-backend=sdl2|x11|auto` controls which is selected.

### Game loop

`main()` → `cowmain()` (in `cowmain.c`) → `newwin()` (calls `W_Initialize`) → `input()`.

`input()` in `input.c` is the main event loop: it calls `W_NextEvent` / `W_SpNextEvent` in a `select()`-based loop that multiplexes `W_Socket()` with the game server socket. The SDL2 backend implements `W_Socket()` using a self-pipe: `W_Initialize()` creates a `pipe()`, `W_Socket()` returns the read end, and `W_EventsQueuedCk()` writes a byte to the write end when SDL events are pending — keeping `input.c`'s select loop unchanged.

### Sound

`sound.c` uses SDL_mixer (`#ifdef HAVE_SDL`). This is already cross-platform. Audio initialization happens in `cowmain.c` via `Init_Sound()`. The `audio.c` / `audio.h` files implement a legacy fork-based background sound system (Linux `/dev/dsp`) and are compiled only on X11 builds.

### Image loading

`x11window.c` uses Imlib2 for PNG pixmap loading (`W_GetPixmaps`, `W_ReadImage`, `W_DrawImage`). An SDL2 port should replace these calls with SDL2_image (`IMG_Load`).

### Build system flow

`configure.ac` → autoconf → `configure` script → writes `config.h` + `system.mk` (from `system.mk.in`). The top-level `Makefile` is a thin shim that calls `make -f system.mk`. All real build rules are in `system.mk`.

### Key data structures

- `struct.h` — game entities: `struct player`, `struct planet`, `struct torp`, `struct ship`
- `data.h` / `data.c` — global game state (declared extern in `data.h`, defined in `data.c`)
- `packets.h` — wire protocol packet structs (shared with server)
- `defs.h` — game constants (galaxy size, speeds, distances, window sizes)

### Window naming

The main windows are global `W_Window` variables in `data.c`:
- `w` — tactical (local) window
- `mapw` — galactic (strategic) window
- `baseWin` — root/parent window
- `tstatw`, `statwin`, `messagew`, `phaserWindow`, etc.

### SDL2 backend internals

`sdl2window.c` implements all ~50 `Wlib.h` functions. Key design points:

- **One SDL_Window, texture-per-W_Window**: each `W_Window` is an `SDL_TEXTUREACCESS_TARGET` texture; `W_Flush()` composites all mapped windows onto the master renderer.
- **Font rendering**: SDL_ttf with a glyph atlas (pre-rendered printable ASCII per color/font). Font loaded from `fonts/DejaVuSansMono.ttf` with fallbacks to system fonts.
- **W_Color / W_Font handles**: `W_White=0`, `W_Black=1`, etc.; `W_BigFont`, `W_RegularFont`, etc. are `(W_Font)&_fi0`, cast from static ints.
- **sdl2/sdl2window.h**: defines `struct sdl2_window`, `struct sdl2_icon`, `struct sdl2_font`, glyph/scroll/menu types, and `SDL2_NCOLORS`/`SDL2_NFONTS` constants.
- **sdl2/sdl2sprite.c**: loads PNG sprite sheets via SDL2_image; `nviews = height/width` frames per sheet; `struct sdl2_sprite` stores the target `struct sdl2_window*` set at load time by `W_StoreBitmap`.
- **Sound**: `sound.c` maps `HAVE_SDL2 → HAVE_SDL` internally to reuse all existing `#if defined(HAVE_SDL)` guards, then diverges only at `Init_Sound()` for SDL2_mixer's 44100Hz stereo init.
- **Unit test hook**: `#ifdef UNIT_TEST int sdl2_test_translate(SDL_Event *, W_Event *)` bypasses the SDL queue for mouse button tests (SDL dummy driver filters these on macOS).
