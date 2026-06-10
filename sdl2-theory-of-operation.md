# SDL2 Backend — Theory of Operation

## Purpose

`sdl2window.c` + `sdl2sprite.c` implement every function declared in `Wlib.h` using SDL2
as the platform layer.  This replaces the X11 backend (`x11window.c`) on macOS and
optionally on Linux/Windows.  The design was cross-validated against the WIN32 backend
(`win32/mswindow.c`) because WIN32 is the only other fully complete Wlib port.

---

## Architecture Overview

### One SDL_Window / Many Textures

Every call to `W_MakeWindow`, `W_MakeTextWindow`, `W_MakeScrollingWindow`, or `W_MakeMenu`
allocates a `struct sdl2_window` and an off-screen `SDL_Texture` render target:

```
SDL_Window  (single OS window, full-screen or sized to WINWIDTH×WINHEIGHT)
  └─ SDL_Renderer  (accelerated, TARGETTEXTURE flag)
       ├─ texture[baseWin]    640×480
       ├─ texture[w]          tactical window   500×500
       ├─ texture[mapw]       galactic map      500×500
       ├─ texture[messagew]   scrolling text    600×100
       ├─ texture[statwin]    status bar        ...
       └─ ... (up to 256 windows)
```

All draw calls (`W_WriteBitmap`, `W_MakeLine`, etc.) target the texture for their window
via `SDL_SetRenderTarget`.  Nothing reaches the screen until `W_Flush()`.

### W_Flush — the compositor

```c
void W_Flush(void) {
    SDL_SetRenderTarget(sdl2_renderer, NULL);   // back to screen
    SDL_RenderClear(sdl2_renderer);             // black background
    for each win in winorder[] where win->mapped:
        SDL_RenderCopy(sdl2_renderer, win->texture, NULL, &{win->x, win->y, ...});
    SDL_RenderPresent(sdl2_renderer);           // flip
}
```

This is called once per server update from the game loop.  WIN32's `W_Flush` is a no-op
because WIN32 draws directly to HWND DCs on every call — no compositing step needed.
X11's `W_Flush` is `XFlush(W_Display)` — just drains the protocol buffer.

The SDL2 compositor model means **every draw call is deferred until `W_Flush`** — this
is invisible to the game but critical to understand when debugging partial-frame issues.

### Retained vs Immediate Rendering

| Backend | Rendering model | Cleared per-frame? |
|---|---|---|
| X11 | Immediate to X drawable, partial `clearzone` tracking | No (only dirty rects) |
| WIN32 | Immediate to HWND DC, no explicit clear | No (Windows repaints on WM_PAINT) |
| SDL2 | Retained textures, compositor | **Yes — W_FastClear=1** |

Because SDL2 textures are persistent, old frames accumulate unless explicitly cleared.
`W_FastClear = 1` causes `clearLocal()` in `local.c` to call `W_ClearWindow(w)` every
frame instead of using the partial `clearzone` mechanism.  The clearzone approach was
unreliable in SDL2 because the det circle has a pre-existing coordinate bug
(`W_WriteCircle(w, dcy, dcy, ...)` — uses `dcy` for both x and y), so the clearzone
rectangle does not match where the circle was drawn.

---

## Global State

```c
static SDL_Window   *sdl2_window_handle;   // the one OS window
SDL_Renderer        *sdl2_renderer;        // the one renderer (shared with sdl2sprite.c)
static int           pipe_read_fd;         // W_Socket() return value
static int           pipe_write_fd;        // written by W_EventsQueuedCk()
static struct sdl2_window *winorder[];     // insertion order for compositor
struct sdl2_font     sdl2_fonts[4];        // per-font glyph atlases
SDL_Color            sdl2_colortable[12];  // W_Color → RGBA
```

---

## Window Types

| Constant | INT | Used for | Coordinate convention |
|---|---|---|---|
| `SDL2WIN_GRAPH` | 1 | tactical, galactic, phaser windows | pixel |
| `SDL2WIN_TEXT`  | 2 | stat windows, player list | char grid → pixel |
| `SDL2WIN_MENU`  | 3 | context menus | char grid → pixel |
| `SDL2WIN_SCROLL`| 4 | message windows, server list | pixel (internal), scroll buffer |

WIN32 uses identical `WIN_GRAPH/TEXT/MENU/SCROLL = 1/2/3/4` with the same semantics.
X11 stores window type as `W_Window.type` with integer constants.

### Pixel dimensions from char counts

```c
// TEXT and MENU: same formula across all three backends
px_w = cols * W_Textwidth  + WIN_EDGE * 2;   // WIN_EDGE=1
px_h = rows * W_Textheight + MENU_PAD * 2;   // MENU_PAD=4
```

WIN32 SCROLL windows add a vertical scrollbar width (`SM_CXVSCROLL`) to `px_w`.  SDL2
does not implement a scrollbar widget; the scroll buffer is stored as a linked list and
the last N visible rows are re-rendered on each `W_FlushScrollingWindow` call.

---

## Font System

### SDL2: glyph atlas

At `W_Initialize` time, all four TTF faces are loaded and every printable ASCII character
(32–126) is pre-rendered for every `W_Color` into per-font, per-color `SDL_Texture` atlases:

```
sdl2_fonts[fidx].atlas[color]  →  SDL_Texture
sdl2_fonts[fidx].glyphs[i].src →  SDL_Rect   (source rect in atlas for char i)
sdl2_fonts[fidx].advance       →  int        (monospace advance = W_Textwidth)
sdl2_fonts[fidx].height        →  int        (cell height = W_Textheight)
```

`W_WriteText` does:
1. `SDL_SetRenderTarget` → window texture
2. Fill background rect (XDrawImageString semantics — background always painted)
3. For each character: `SDL_RenderCopy` from atlas rect to destination

### WIN32 fonts

WIN32 loads TrueType fonts from its resource `.res` file via `AddFontResource` +
`CreateFontIndirect`, four logical fonts: Regular (FW_REGULAR), Bold (FW_BOLD),
Underline, and Arial 52pt Big.  Text is drawn with `ExtTextOut` which handles both
foreground color and background fill in a single call — equivalent to XDrawImageString.

---

## Text: W_WriteText vs W_MaskText

| Function | X11 | WIN32 | SDL2 |
|---|---|---|---|
| `W_WriteText` | `XDrawImageString` — fills GC background under text | `ExtTextOut` with `SetBkColor` — fills BkColor under text | `draw_string(fill_bg=1)` — fills black rect before glyphs |
| `W_MaskText` | `XDrawString` — glyphs only, no background fill | `ExtTextOut` with `ETO_OPAQUE` cleared — text only | `draw_string(fill_bg=0)` — glyphs only, no fill |

`W_MaskText` is called by the beeplite subsystem and by TTS text overlay.  If it fills
the background, text drawn over other content (ship bitmaps, planet icons) creates a
visible black rectangle.  The `fill_bg` parameter flag was added to `draw_string` to
implement the distinction without duplicating the glyph-rendering loop.

---

## Bitmap Rendering — W_StoreBitmap / W_WriteBitmap / W_OverlayBitmap

### XBM format

Bitmaps are stored in XBM 1-bit format: each byte contains 8 pixels, LSB first.  Bit 1 =
foreground (ship/planet shape); bit 0 = background (transparent).

### SDL2 conversion

`W_StoreBitmap` converts the 1-bit data to a 32-bit RGBA `SDL_Surface`: 1-bits become
white (255,255,255,255), 0-bits become transparent (alpha=0).  `SDL_CreateTextureFromSurface`
uploads it as `SDL_BLENDMODE_BLEND`.

At draw time, `W_WriteBitmap` calls `SDL_SetTextureColorMod` to tint white → desired
W_Color, then `SDL_RenderCopy` in BLEND mode.  0-bit (alpha=0) pixels are skipped by
the blender — leaving the destination unchanged.  This exactly reproduces X11's
`BITGC = GXor` behavior:

```
X11 GXor:  1-bit → FG XOR dst (on black dst: FG XOR 0 = FG)
           0-bit → BG XOR dst = 0 XOR dst = dst (unchanged)

SDL2 BLEND:  alpha=255 → composite FG over dst → FG (opaque)
             alpha=0   → composite nothing → dst (unchanged)
```

### WIN32 comparison

WIN32's `W_WriteBitmap` uses `BitBlt` with `SRCPAINT` (bitwise OR):

```c
SetBkColor(hdc, colortable[color].rgb);   // 1-bits → FG
SetTextColor(hdc, colortable[BLACK].rgb); // 0-bits → black
BitBlt(hdc, x, y, w, h, GlobalMemDC, sx, sy, SRCPAINT);  // OR
```

`SRCPAINT` ORs source into destination.  The color mapping makes 1-bits = FG color and
0-bits = 0x000000 (black).  On a black destination: FG OR 0 = FG; 0 OR dst = dst.
This is identical to SDL2's BLEND mode in practice, and to X11's GXor.

### W_WriteBitmap vs W_OverlayBitmap

In X11:
- `W_WriteBitmap` → `contexts[BITGC]` with `GXor` (transparent 0-bits)
- `W_OverlayBitmap` → `contexts[0]` with `GXcopy` (0-bits painted as BG color = black)

In WIN32 and SDL2 both functions produce identical output because the background is
always black (WIN32: black HWND background; SDL2: `W_FastClear=1` clears to black each
frame).  With a black destination, GXor and GXcopy produce the same result.  SDL2
therefore uses BLEND (= GXor semantics) for both.

---

## Drawing Primitives

### Circles — W_WriteCircle

All three backends follow the X11 `XDrawArc` coordinate convention:
`(bx, by)` = top-left of bounding box, `diam` = diameter (not radius).

```c
// SDL2: convert to center + radius for midpoint circle algorithm
int rad = diam / 2;
int cx  = bx + rad, cy = by + rad;
```

The SDL2 midpoint algorithm produces an 8-fold symmetric ring of `SDL_RenderDrawPoint`
calls — closest approximation to X11's arc rendering without requiring a filled GDI arc.

X11: `XDrawArc(display, d, gc, x, y, r, r, 0, 23040)` — full circle (23040 = 360×64).
WIN32: `Ellipse(hdc, x, y, x+w, y+h)` with NULL_BRUSH to draw outline only.

### Triangles — W_WriteTriangle

All three backends agree on orientation:

```
t=0: ▽ downward  — tip at (x,y), base vertices at (x±s, y-s)  [base above tip]
t=1: △ upward    — tip at (x,y), base vertices at (x±s, y+s)  [base below tip]
```

This was initially wrong in SDL2 (t=0/t=1 inverted).  The WIN32 implementation
(`mswindow.c:1988`) was the reference used to verify the correct orientation, since it
matches X11.

X11 uses `XDrawLines` with 4 points (closed polygon).  WIN32 uses GDI `Polygon` with
3 points (auto-closed).  SDL2 uses `SDL_RenderDrawLines` with 4 points (pts[3]=pts[0]).

### Lines — W_MakeLine / W_MakeTractLine / W_MakePhaserLine

`W_MakeLine` and `W_MakePhaserLine` are solid lines: `SDL_RenderDrawLine`, GDI
`LineTo`, or `XDrawLine`.  All three are equivalent.

`W_MakeTractLine` (tractor/pressor beam indicator) is a sparse dotted line.

| Backend | Dash pattern | Period | Notes |
|---|---|---|---|
| X11 | GC dash `{1, 8}` LineOnOffDash | 9 px | 1 on, 8 off |
| WIN32 | `dashdesc[] = {10, 1}` (configurable via `tpdotdist`) | 11 px | 10 off, 1 on |
| SDL2 | one point per 9 px | 9 px | matches X11 |

```c
// SDL2 implementation
for (float t = 0; t < len; t += 9.0f)
    SDL_RenderDrawPoint(sdl2_renderer, (int)(x0+ux*t), (int)(y0+uy*t));
```

### Fill and Clear — W_FillArea / W_ClearArea

`W_FillArea` fills with a given color.  `W_ClearArea` fills with the game's
`backColor` global (declared `extern W_Color backColor` in `data.h`).

Coordinate conversion for character-grid window types:

| Backend | W_FillArea conversion | W_ClearArea conversion |
|---|---|---|
| X11 | None — always pixels | None — always pixels |
| WIN32 | None — always pixels | WIN_TEXT only |
| SDL2 | SDL2WIN_TEXT + SDL2WIN_MENU | None (always pixels) |

Note: WIN32's `W_ClearArea` converts char-grid for `WIN_TEXT` but not `WIN_MENU`.
SDL2's `W_FillArea` converts for TEXT and MENU to match how callers use them.
`W_ClearArea` in both WIN32 and SDL2 always receives pixel coordinates from the game.

The `backColor` variable connects to the game's `.xtrekrc` / `defaults.c` pipeline.
Earlier SDL2 code had `static int sdl2_backColor = 1` (hardcoded BLACK), ignoring the
game's configured value.  This was corrected to use `(int)backColor` directly.

---

## Event System

### SDL2: self-pipe

`input.c` uses a POSIX `select()` loop multiplexing the server socket with the window
system fd returned by `W_Socket()`.  SDL2 has no fd-based event source.

**Solution**: at `W_Initialize` time, create a `pipe(pipe_fds)`.  `W_Socket()` returns
`pipe_fds[0]` (read end).  `W_EventsQueuedCk()` calls `SDL_PumpEvents()` and if events
are pending, writes one byte to `pipe_fds[1]`.  This makes the pipe readable in
`select()`, triggering `W_SpNextEvent()`, which drains the pipe byte and calls
`SDL_PollEvent`.

```
SDL2 event loop:
  server select()  ←→  pipe_fds[0]
                         ↑
                   W_EventsQueuedCk()
                         ↑
                   SDL_PumpEvents() + SDL_PeepEvents()
```

The 10 ms `select()` timeout in `input.c` ensures `SDL_PumpEvents` is called at least
100 times/second even during idle periods.

### WIN32: magic number

WIN32's `W_Socket()` returns `0x0CABBABE` — a sentinel that the custom `select()`
implementation in `winmain.c` recognizes as "the Windows message queue fd" and polls
via `PeekMessage`.  No pipe or OS fd is involved.  This is WIN32-specific and does not
port to other platforms.

### X11: display connection fd

X11 returns the file descriptor of the X11 server connection (`ConnectionNumber(display)`).
Standard `select()` on this fd detects incoming X events directly.

---

## W_Socket Summary

| Backend | W_Socket return value | How select() works |
|---|---|---|
| X11 | X11 display connection fd | Real fd; kernel signals readability on X events |
| WIN32 | `0x0CABBABE` (magic) | Custom select() in winmain.c polls Windows message queue |
| SDL2 | `pipe_fds[0]` (read end) | Real fd; signaled by W_EventsQueuedCk via pipe write |

---

## Sprites and Image Loading

`sdl2sprite.c` replaces `x11sprite.c`.  Both load PNG sprite sheets from `pixmapDir`
(compiled in via `DATADIR`) and extract `nviews = sheet_height / frame_height` animation
frames.

| X11 | SDL2 |
|---|---|
| Imlib2 `imlib_load_image` | SDL2_image `IMG_Load` |
| X11 Pixmap + shape mask (XBM) for transparency | SDL2 RGBA texture (alpha channel = shape mask) |
| `XCopyArea` / `XCopyPlane` for drawing | `SDL_RenderCopy` |

The `struct sdl2_sprite` stores the target `struct sdl2_window *` (set from `W_StoreBitmap`'s
window argument) so `W_WriteBitmap` knows which texture to target.

---

## Full-Screen Toggle

`W_FullScreenToggle()`:

```c
// SDL2
SDL_SetWindowFullscreen(sdl2_window_handle, SDL_WINDOW_FULLSCREEN_DESKTOP);

// WIN32: not implemented (no fullscreen code in mswindow.c)
// X11: not implemented in standard COW (uses XF86VidMode on some builds)
```

---

## Cursors

WIN32 loads custom pixmap cursors from a `.res` resource file.  X11 creates cursors from
XBM bitmaps via `XCreatePixmapCursor`.  SDL2 cursor functions are currently stubs.  The
fix is `SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_*)` for stock shapes and
`SDL_CreateCursor(data, mask, w, h, hot_x, hot_y)` for custom XBM-based cursors (same
bit format as X11).

---

## W_Beep — Discussion

### What calls W_Beep

`W_Beep()` is called in two contexts:

1. **Sound-disabled fallback** (`beeplite.c:196`):
   ```c
   if (sound_toggle)
       Play_Sound(MESSAGE_SOUND);  // plays nt_message.wav
   else
       W_Beep();                   // only called when SOUND is off
   ```

2. **Direct UI error feedback** (13 call sites in `input.c`, `findslot.c`, `getname.c`,
   `option.c`, `newwin.c`, `smessage.c`) — called unconditionally, unguarded by
   `sound_toggle`.  These are "invalid command / bad input" signals.

### How each backend implements it

| Backend | Implementation | Notes |
|---|---|---|
| X11 (`x11window.c`) | `XBell(W_Display, 0)` | X11 server bell, heard through hardware audio or PC speaker |
| WIN32 (`mswindow.c`, `gnu_win32.c`) | `MessageBeep(MB_ICONEXCLAMATION)` | Plays the Windows "Exclamation" system sound event |
| SDL2 current | `fputs("\a", stderr)` | Terminal bell — has no effect when running as a GUI app |

### Recommended SDL2 approach: synthesized bell via SDL2_mixer

When SDL2_mixer is initialized (which it is if `SOUND` was compiled in and `Init_Sound()`
succeeded), a short synthesized sine-wave beep can be generated in memory and played
through the mixer without requiring a `.wav` file on disk.  The tone can be pre-built
at `W_Initialize` time:

```c
// Generate 200ms of 880 Hz sine wave at 44100 Hz stereo 16-bit signed
// (called once at init, stored as Mix_Chunk for reuse)
static Mix_Chunk *beep_chunk;

static void build_beep_chunk(void) {
    const int rate = 44100, ms = 200, freq = 880;
    int nsamples = rate * ms / 1000 * 2;  /* stereo */
    Sint16 *buf = SDL_malloc(nsamples * 2);
    for (int i = 0; i < nsamples; i += 2) {
        double t = (double)(i/2) / rate;
        Sint16 v = (Sint16)(12000 * sin(2.0 * M_PI * freq * t));
        /* fade out last 20ms to avoid click */
        if (i/2 > nsamples/2 - rate*20/1000)
            v = (Sint16)(v * (double)(nsamples/2 - i/2) / (rate*20/1000));
        buf[i] = buf[i+1] = v;
    }
    SDL_RWops *rw = SDL_RWFromMem(buf, nsamples * 2);
    beep_chunk = Mix_QuickLoad_RAW((Uint8*)buf, nsamples * 2);
    // Note: Mix_QuickLoad_RAW doesn't copy, so buf must stay allocated
}

void W_Beep(void) {
    if (beep_chunk)
        Mix_PlayChannel(-1, beep_chunk, 0);
    // else: silent (no SOUND compiled, or audio init failed)
}
```

This approach:
- Works whether or not the `.wav` sound directory is found
- Is completely silent when audio is not initialized (no-sound builds, CI headless)
- Does not depend on the `\a` terminal escape having any effect
- Matches the spirit of `XBell` (OS-mediated alert tone) and `MessageBeep` (system sound)
- Plays through the same SDL2_mixer pipeline as all other game sounds

If the simpler approach is preferred: leave W_Beep as a no-op.  All 13 direct call
sites are responding to user input errors that are already visually indicated by the
game — the bell is purely optional feedback.  The critical beeplite path already uses
`Play_Sound(MESSAGE_SOUND)` when sound is enabled.

---

## Gap Analysis: SDL2 vs X11 / WIN32

| # | Function | X11 behavior | WIN32 behavior | SDL2 status | Notes |
|---|---|---|---|---|---|
| ✅ | `W_WriteCircle` | `XDrawArc` bounding-box convention (x,y=topleft, diam) | `Ellipse` with NULL_BRUSH | Fixed: center=(bx+r/2, by+r/2), radius=diam/2 | X11 bounding-box convention is unintuitive |
| ✅ | `W_WriteBitmap` / `W_OverlayBitmap` | GXor (transparent) / GXcopy (opaque) | `SRCPAINT` OR for both | Both use BLEND; functionally identical with W_FastClear=1 | GXor ≡ BLEND on black bg |
| ✅ | `W_WriteTriangle` | t=0 = ▽ tip at (x,y), base at y-s | Same as X11 | Fixed: was t=0/t=1 inverted | WIN32 is the reference |
| ✅ | `W_MakeTractLine` | LineOnOffDash `{1,8}` — very sparse dots | `{10,1}` (10 off, 1 on) | Fixed: one point per 9px (period=9, matches X11) | WIN32 default period=11 |
| ✅ | `W_MaskText` | `XDrawString` — no background fill | `ExtTextOut` without opaque flag | Fixed: `draw_string(fill_bg=0)` | Needed for beeplite overlay |
| ✅ | `backColor` | `colortable[backColor]` extern | Game extern | Fixed: uses `extern W_Color backColor` from data.h | Was hardcoded BLACK=1 |
| ✅ | `W_FastClear` | Clearzone partial erase | WIN32 paints on WM_PAINT | Set to 1 (full-window clear per frame) | Clearzone unreliable in SDL2 |
| ⚠ | `W_Beep` | `XBell(display, 0)` | `MessageBeep(MB_ICONEXCLAMATION)` | `fputs("\a", stderr)` — no effect in GUI | See synthesized-bell approach above |
| ⚠ | Cursors | Custom XBM pixmap cursors | Resource file cursors | All stubs | Use `SDL_CreateCursor` / `SDL_CreateSystemCursor` |
| ✅ | `W_FillArea` coord conv | Always pixels | Always pixels | TEXT+MENU windows converted from char-grid | WIN32 does no conversion |
| ✅ | `W_ClearArea` | Always pixels | WIN_TEXT only | Always pixels | Consistent with WIN32 for GRAPH windows |
| ✅ | `W_Socket` | X11 display fd | Magic number, custom select | Real pipe fd, standard select | Clean POSIX design |
| ✅ | `W_Flush` | `XFlush` | No-op | Full compositor + `SDL_RenderPresent` | Necessary for texture model |
| ⬜ | `W_MakeLines` | Exists | Exists (batch GDI) | Not in Wlib.h / SDL2 build | Only called via `#ifdef SHORT_PACKETS` |
| ⬜ | `W_ClearAreas` | Exists | Exists (batch clear) | Not exposed | Batch variant; single-clear fallback exists |
| ✅ | Sprite loading | Imlib2 PNG → X Pixmap | GDI bitmap | SDL2_image PNG → SDL_Texture | SDL_BLENDMODE_BLEND replaces X shape mask |
| ✅ | Sound | `XBell` / SDL_mixer | Direct audio | SDL2_mixer via `sound.c` | SDL2_SOUND flag remaps HAVE_SDL guards |
| ✅ | Full-screen | XF86VidMode | Not implemented | `SDL_SetWindowFullscreen` | |

---

## Coordinate Systems

All draw calls in `W_Window` coordinates.  The SDL2 backend inherits the X11 convention:
origin at top-left, y increases downward.  This is also WIN32's GDI convention.

Border pixels (`win->border`) are added by WIN32 before every draw call.  SDL2 and X11
do not add border offsets at the draw level — the border is part of the window's pixel
dimensions (the 1-pixel edge is within the texture).

---

## Build Configuration

```
configure --with-backend=sdl2
```

Sets `NOSDL2=""` and `NOX11="#"` in `system.mk.in`.  `HAVE_SDL2=1` is defined in
`config.h`.  `sdl2window.c` and `sdl2sprite.c` are compiled; `x11window.c` and
`x11sprite.c` are excluded.  SDL2 / SDL2_ttf / SDL2_image / SDL2_mixer are linked via
`pkg-config`.

Dependencies: `sdl2 >= 2.0`, `SDL2_ttf >= 2.0`, `SDL2_image >= 2.0`,
`SDL2_mixer >= 2.0`.  All available via Homebrew on macOS.
