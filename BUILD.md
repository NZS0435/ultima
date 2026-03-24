<img src="/Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg" alt="Team Thunder" />

Phase Label: Phase 1 - Scheduler and Semaphore

# Cross-Platform Build Guide (macOS + Windows + Cygwin)

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

### Windows (MSYS2 MinGW64)

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

### Windows (Cygwin)

Install packages in Cygwin Setup (`setup-x86_64.exe`):

- `gcc-g++`
- `cmake`
- `ninja`
- `gdb`
- `libncurses-devel`

Configure + build from a Cygwin shell:

```bash
cmake --preset cygwin-debug
cmake --build --preset cygwin-debug
```

Run:

```bash
/cygdrive/c/Users/<YourUser>/.ultima2/build/cygwin-debug/ultima_os.exe
```

## CLion Setup (Recommended)

1. `Settings > Build, Execution, Deployment > CMake`.
2. Enable `Load CMake presets`.
3. Select:
   - `macos-debug` on Mac
   - `windows-debug` for MSYS2/MinGW or MSVC workflows
   - `cygwin-debug` for Cygwin workflows
4. Do not use `cmake-build-debug` inside the SharePoint project tree.

### CLion Cygwin Toolchain/Profile Setup

1. Open `Settings > Build, Execution, Deployment > Toolchains`.
2. Add a new toolchain and set environment to `Cygwin`.
3. Set paths (adjust if your Cygwin root differs):
   - `C Compiler`: `C:\\cygwin64\\bin\\gcc.exe`
   - `C++ Compiler`: `C:\\cygwin64\\bin\\g++.exe`
   - `Debugger`: `C:\\cygwin64\\bin\\gdb.exe`
   - `CMake`: `C:\\cygwin64\\bin\\cmake.exe`
   - `Build Tool`: `C:\\cygwin64\\bin\\ninja.exe`
4. Open `Settings > Build, Execution, Deployment > CMake`.
5. Enable `Load CMake presets`.
6. Select preset `cygwin-debug`.
7. Ensure the selected toolchain is the Cygwin toolchain created above.

## Notes on ncurses / pthread portability

- macOS/Linux: CMake uses `find_package(Curses)` and links native pthreads via `Threads::Threads`.
- Cygwin: CMake uses `find_package(Curses)` with Cygwin ncurses and links pthreads via `Threads::Threads` (no manual path wiring).
- Non-Cygwin Windows (MSYS2/MinGW/MSVC): CMake links PDCurses. Threading links through `Threads::Threads`; if native `pthread.h` is unavailable, the code automatically uses the local mutex compatibility shim in `platform_threads.h`.
