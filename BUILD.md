# Cross-Platform Build Guide (macOS + Windows)

## Important for SharePoint Collaborators

`CMakeCache.txt` stores absolute paths, so a build directory generated on one machine cannot be reused on another. To prevent path collisions between macOS and Windows collaborators:

- Keep source files in SharePoint.
- Keep build directories **outside** SharePoint.
- Use the shared CMake presets in this repo.

## One-Time Cleanup (if you already hit the cache mismatch)

From the project root, remove the shared stale build folder:

```bash
rm -rf cmake-build-debug
```

On Windows PowerShell:

```powershell
Remove-Item -Recurse -Force .\\cmake-build-debug
```

## Build with CMake Presets

### macOS

Install dependencies:

```bash
brew install cmake ninja ncurses
```

Configure + build:

```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
```

Run:

```bash
~/.ultima2/build/macos-debug/ultima_os
```

### Windows (MSYS2 MinGW64 recommended)

Install dependencies from `MSYS2 MinGW x64` shell:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-pdcurses
```

Configure + build:

```bash
cmake --preset windows-debug
cmake --build --preset windows-debug
```

Run:

```powershell
$env:USERPROFILE\\.ultima2\\build\\windows-debug\\ultima_os.exe
```

If CMake cannot find PDCurses, add:

```bash
-DPDCURSES_ROOT=C:/msys64/mingw64
```

## CLion Setup (Recommended)

1. `Settings > Build, Execution, Deployment > CMake`.
2. Enable `Load CMake presets`.
3. Select `macos-debug` on Mac, `windows-debug` on Windows.
4. Do not use `cmake-build-debug` inside the SharePoint project tree.

## Notes on ncurses / pthread portability

- macOS/Linux: CMake uses `find_package(Curses)` and links native pthreads via `Threads::Threads`.
- Windows: CMake links PDCurses. Threading links through `Threads::Threads`; if native `pthread.h` is unavailable, the code automatically uses the local mutex compatibility shim in `platform_threads.h`.
