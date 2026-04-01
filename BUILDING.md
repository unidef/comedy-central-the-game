# Building CC Click Game

This file is a build-only quick reference.

## Requirements

- Windows 10/11
- C++ compiler toolchain
- OpenGL-capable GPU/driver

Choose one:
- Visual Studio 2026/2022 with **Desktop development with C++**
- MinGW-w64 (`g++`)

---

## Visual Studio (Recommended)

1. Open `CC_Click_Game.sln` (or `.slnx`).
2. Select configuration:
   - `Debug` or `Release`
   - Platform: `x64` (recommended)
3. Build:
   - **Build > Build Solution**
4. Run:
   - **Debug > Start Without Debugging** (`Ctrl+F5`)

---

## MinGW-w64

From project root:

```powershell
cd C:\Users\jmaha\source\repos\CC_Click_Game
g++ -std=c++20 -O2 -municode -mwindows main.cpp LogoNodeTree.cpp -o CC_Click_Game.exe -lopengl32 -lgdiplus -lwinmm -lcomctl32 -lole32 -luuid -lgdi32 -luser32 -lkernel32
```

Run:

```powershell
.\CC_Click_Game.exe
```

### Optional (older MinGW)

If you hit `<filesystem>` link issues:

```powershell
g++ -std=c++20 -O2 -municode -mwindows main.cpp LogoNodeTree.cpp -o CC_Click_Game.exe -lopengl32 -lgdiplus -lwinmm -lcomctl32 -lole32 -luuid -lgdi32 -luser32 -lkernel32 -lstdc++fs
```

---

## Linked Libraries

The app requires:

- `opengl32`
- `gdiplus`
- `winmm`
- `comctl32`

And commonly:

- `ole32`
- `uuid`
- `gdi32`
- `user32`
- `kernel32`

---

## Output Assets

Optional PNG files (if present) should be placed beside the executable or project root:

- `cc.png`
- `cartman.png`
- `drawn.png`

If missing, fallback textures are used.