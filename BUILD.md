# Cross-Platform Build Guide (macOS + Windows)

This project uses CMake and supports `ncurses`/`pthreads` behavior on:

- macOS via `ncurses` + native pthreads
- Windows via `PDCurses` + either native `pthread.h` (MinGW/winpthreads) or the built-in mutex compatibility shim

## 1) macOS (Apple Silicon or Intel)

Install dependencies:

```bash
brew install cmake ncurses
```

Configure and build:

```bash
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$(brew --prefix ncurses)"
cmake --build build/macos
```

Run:

```bash
./build/macos/ultima_os
```

## 2) Windows with MSYS2 (recommended)

Install MSYS2 packages in `MSYS2 MinGW x64` shell:

```bash
pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-pdcurses
```

Configure and build:

```bash
cmake -S . -B build/windows-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DPDCURSES_ROOT=C:/msys64/mingw64
cmake --build build/windows-mingw
```

Run:

```bash
build\\windows-mingw\\ultima_os.exe
```

## 3) Windows with Visual Studio + prebuilt PDCurses

If you use MSVC, install/build PDCurses and pass its root folder:

```powershell
cmake -S . -B build/windows-msvc -G "Visual Studio 17 2022" -A x64 -DPDCURSES_ROOT=C:/path/to/pdcurses
cmake --build build/windows-msvc --config Debug
```

## Notes

- On non-Windows platforms CMake resolves curses through `find_package(Curses)`.
- On Windows, CMake searches for `pdcurses`/`pdcursesw` and can be pointed with `-DPDCURSES_ROOT=...`.
- Threading is linked via `Threads::Threads`. If `pthread.h` is unavailable on Windows, the code automatically uses a local compatibility shim.
