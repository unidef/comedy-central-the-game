# CC Click Game

A chaotic Windows/OpenGL arcade sandbox where you deform a spinning Comedy Central logo, battle Cartman for logo consumption, fire shader lasers, and tune runtime behavior from an in-game options panel.

This project is a native C++ Win32 desktop app using:

- Win32 API window/message loop
- OpenGL (fixed-function + GLSL shader programs loaded via `wglGetProcAddress`)
- GDI+ for PNG loading
- WinMM for system sounds
- Common Controls (`comctl32`) for the options window controls

---

## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Repository Layout](#repository-layout)
- [Building in Visual Studio](#building-in-visual-studio)
- [Building with MinGW-w64](#building-with-mingw-w64)
- [Running the Game](#running-the-game)
- [Assets](#assets)
- [Controls](#controls)
- [Gameplay Notes](#gameplay-notes)
- [Options Window](#options-window)
- [Troubleshooting](#troubleshooting)
- [Performance Notes](#performance-notes)
- [License / Credits](#license--credits)

---

## Features

- Animated gradient and procedural backgrounds
- Main deformable logo with shader effects
- Spawned logo orbs with interactions, implosion/explosion fragments, and node-link rendering
- Cartman AI entity that competes by eating logos
- Laser beam (spacebar) with stereo-style shader coloring
- Runtime options window:
  - Resolution selection
  - Sound on/off
  - VSync toggle
  - Triple buffering toggle (driver dependent)
  - Sliders for spawn/gravity/laser/jiggle behavior
- Fullscreen toggle
- Window title FPS updates

---

## Requirements

### Runtime (Windows)

- Windows 10/11
- GPU/driver with OpenGL support
- Visual C++ runtime (if using non-static MSVC runtime)

### Build Tools

Choose one:

1. **Visual Studio** (recommended)
   - Visual Studio 2022/2026 with **Desktop development with C++**
2. **MinGW-w64**
   - `g++` with Windows headers/libs

---

## Repository Layout

Typical key files:

- `main.cpp` — Win32 app, rendering, input, gameplay loop, options window
- `LogoNodeTree.h` / `LogoNodeTree.cpp` — node tree logic for spawned-logo neighbor queries
- `CC_Click_Game.vcxproj` — Visual Studio project
- `CC_Click_Game.vcxproj.filters` — VS filter organization

Optional runtime assets (same folder as executable or project root):

- `cc.png`
- `cartman.png`
- `drawn.png`

If missing, fallback textures are generated automatically.

---

## Building in Visual Studio

1. Open `CC_Click_Game.sln` (or `.slnx` if your VS uses it).
2. Select configuration:
   - `Debug` / `Release`
   - Platform: `x64` recommended
3. Build:
   - **Build > Build Solution**
4. Run:
   - **Debug > Start Without Debugging** (`Ctrl+F5`)

### Notes

- The project links against: `opengl32`, `gdiplus`, `winmm`, `comctl32`.
- Additional Windows libraries are typically resolved automatically by MSVC toolchain/project settings.

---

## Building with MinGW-w64

> This is a Win32 desktop OpenGL build (not cross-platform). Build on Windows with MinGW-w64.

### 1) Ensure MinGW-w64 is installed

You need:

- `g++`
- `mingw32-make` (optional)
- Windows SDK-compatible headers/libs (usually included with MinGW-w64)

### 2) Open terminal at repo root

```powershell
cd C:\Users\jmaha\source\repos\CC_Click_Game
```

### 3) Compile

Use this baseline command:

```powershell
g++ -std=c++20 -O2 -municode -mwindows main.cpp LogoNodeTree.cpp -o CC_Click_Game.exe -lopengl32 -lgdiplus -lwinmm -lcomctl32 -lole32 -luuid -lgdi32 -luser32 -lkernel32
```

### 4) Run

```powershell
.\CC_Click_Game.exe
```

### MinGW Notes

- If your MinGW toolchain is older and has `<filesystem>` linker issues, add:
  - `-lstdc++fs`
- If `gdiplus` is unresolved, ensure your MinGW distribution ships import libs for `gdiplus.dll`.
- If `-municode` causes entry-point mismatch, ensure you are using modern MinGW-w64 and keep `main()` as in source (the current source works with standard C++ `main`).

---

## Running the Game

- On launch, the console window is hidden and the game window appears.
- If startup fails, a message box appears: **"Failed to start the OpenGL game."**

---

## Assets

The app searches for assets in multiple locations (project dir, exe dir, parent paths).

Expected files:

- `cc.png` — main logo texture
- `cartman.png` — Cartman texture
- `drawn.png` — victory screen texture

If files are not found:

- fallback textures are generated
- overlay text warns about missing `cc.png`

Recommended: place PNGs next to the executable (`x64\Debug` or `x64\Release`) or in project root.

---

## Controls

### Mouse

- **Left click logo**: deform and perturb scene
- **Left click spawned logo**: consume it (adds to "You ate")
- **Double-click Cartman**: increase Cartman size
- **Drag Cartman**: reposition him
- **Exit button**: close game
- **Options button**: open options window

### Keyboard

- `A` / `D`: spin control
- `W` / `S`: flying influence
- Arrow keys: scene/laser chaos controls
- Number keys `0-9` (and numpad): deformation presets
- `Space`: hold to fire laser
- `F11`: fullscreen toggle
- `Esc`: quit
- `Alt+Enter`: fullscreen toggle

---

## Gameplay Notes

- You compete against Cartman to consume logos.
- Win target is randomized each run (currently 20–100).
- Game can trigger explosion/game-over states via chaos conditions.
- The overlay displays FPS, clicks, flight meter, and score progress.

---

## Options Window

Opened by clicking the glowing green **options** button under Exit.

### Available settings

- **Resolution** + **Apply Resolution**
- **Sound On/Off**
- **VSync On/Off**
- **Triple Buffering On/Off** (driver dependent behavior)
- Sliders:
  - Logo spawn rate
  - Logo gravity
  - Laser speed
  - Scene jiggle

### Presentation settings behavior

- VSync uses `wglSwapIntervalEXT` when available.
- Triple buffering support depends on GPU driver and WGL implementation.
- If unsupported, toggles may have limited/no effect.

---

## Troubleshooting

### 1) "Failed to start the OpenGL game"

Possible causes:

- OpenGL context creation failed
- incompatible/remote desktop graphics path
- bad/old GPU driver

Fixes:

- update GPU driver
- run locally (not via limited remote session)
- try another machine/GPU

### 2) Black or empty window

- verify OpenGL is available (`opengl32.dll` path normal)
- disable overlays/injectors (RTSS/old capture hooks)
- reset options to default by restarting

### 3) PNG assets not loading

- verify filenames are exact: `cc.png`, `cartman.png`, `drawn.png`
- ensure files are valid PNGs
- place files beside the executable

### 4) No sound

- check options window: `Sound On`
- app uses Windows system aliases (`SystemAsterisk`, etc.), so system sound scheme must be enabled
- verify Windows volume mixer and output device

### 5) VSync toggle does nothing

- driver may not expose `wglSwapIntervalEXT`
- driver control panel may override app setting
- test with fullscreen/windowed variations

### 6) Triple buffering toggle does nothing

- triple buffering behavior is driver dependent in this path
- some drivers ignore app-level requests in windowed mode
- use GPU control panel to enforce if needed

### 7) Resolution apply appears incorrect

- try applying again after switching from fullscreen to windowed
- ensure selected resolution exists in current display mode list
- if DPI scaling causes perceived mismatch, set app DPI behavior in compatibility settings

### 8) MinGW link errors

Common unresolved externals:

- `__imp_GdiplusStartup` -> ensure `-lgdiplus`
- OpenGL symbols -> ensure `-lopengl32`
- sound symbols -> ensure `-lwinmm`
- common controls -> ensure `-lcomctl32`
- COM/OLE symbols -> add `-lole32 -luuid`

### 9) `<filesystem>` link failure (older MinGW)

Add:

```powershell
-lstdc++fs
```

---

## Performance Notes

- VSync can cap FPS to monitor refresh.
- Triple buffering may reduce stutter on some systems, no effect on others.
- Release builds are strongly recommended for smooth gameplay.

---

## License / Credits

- Project repository: `https://github.com/unidef/comedy-central-the-game`
- Uses system APIs/libraries from Microsoft Windows and OpenGL.

If you add third-party assets (`cc.png`, etc.), ensure you have rights to use/distribute them.
