<img src="/Users/stewartpawley/Library/CloudStorage/OneDrive-SharedLibraries-IndianaUniversity/O365-IU-CSCI-CSCI-C435 - General/Ultima 2.0/Team Thunder.jpeg" alt="Team Thunder" />

Phase Label: Phase 1 - Scheduler, Semaphore, and Memory Manager

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
~/.ultima2/run/macos-debug/ultima_os
```

Default behavior:

- `ultima_os` now runs a single presentation cycle, keeps the final UI state on screen, and waits for a key press before closing the ncurses windows and printing the transcript.
- The live proof-board now includes the scheduler, semaphore, and a deterministic first-fit memory manager that tracks one task buffer per demo task.
- It also writes `phase1output.txt` in the current working directory.
- Use `--no-post-ui-transcript` only if you explicitly want the UI to exit without printing the transcript afterward.
- Use `--continuous` only if you explicitly want the old repeating demo loop.

If you want the Phase 1 ncurses windows from Finder or an unsupported macOS debug console, you can launch the runtime-directory launcher copied beside the binary:

```bash
~/.ultima2/run/macos-debug/run_ultima_ui.command
```

The repository copy also works:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_ui.command
```

If you want the same shell command your Cygwin collaborators can use, launch:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_ui.sh
```

If you want the exact plain-text transcript output for screenshots or write-up material, launch:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_transcript.command
```

Portable shell equivalent:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_transcript.sh
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

Default behavior:

- `ultima_os.exe` now runs a single presentation cycle, keeps the final UI state on screen, and waits for a key press before closing the ncurses windows and printing the transcript.
- The live proof-board now includes the scheduler, semaphore, and a deterministic first-fit memory manager that tracks one task buffer per demo task.
- It also writes `phase1output.txt` in the current working directory.
- Use `--no-post-ui-transcript` only if you explicitly want the UI to exit without printing the transcript afterward.
- Use `--continuous` only if you explicitly want the old repeating demo loop.

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

Why this preset uses a `/cygdrive/...` path:

- The Cygwin preset intentionally resolves into `/cygdrive/c/Users/<user>/.ultima2/build/cygwin-debug` so CLion's Cygwin toolchain and direct Cygwin login shells agree on the same cache directory.
- Using Cygwin `HOME` here can resolve to `/home/<user>/.ultima2/build/cygwin-debug`, which causes the exact `is not a directory` error when the actual cache lives under `C:\Users\<user>\.ultima2\build\cygwin-debug`.
- Using a raw Windows path here can be misread as a relative path by a real Cygwin shell, so the preset keeps the directory in native Cygwin form.

Run:

```bash
/cygdrive/c/Users/<YourUser>/.ultima2/build/cygwin-debug/ultima_os.exe
```

Default behavior:

- `ultima_os.exe` now runs a single presentation cycle, keeps the final UI state on screen, and waits for a key press before closing the ncurses windows and printing the transcript.
- The live proof-board now includes the scheduler, semaphore, and a deterministic first-fit memory manager that tracks one task buffer per demo task.
- It also writes `phase1output.txt` in the current working directory.
- Use `--no-post-ui-transcript` only if you explicitly want the UI to exit without printing the transcript afterward.
- Use `--continuous` only if you explicitly want the old repeating demo loop.

Or, from the shared source tree, use the same launcher command your macOS teammates can use:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_ui.sh
```

Transcript-only:

```bash
./ULTIMA/Phase\ 1\ \(Scheduler\)/run_ultima_transcript.sh
```

## CLion Setup (Recommended)

1. `Settings > Build, Execution, Deployment > CMake`.
2. Enable `Load CMake presets`.
3. Select:
   - `macos-debug` on Mac
   - `windows-debug` for MSYS2/MinGW or MSVC workflows
   - `cygwin-debug` for Cygwin workflows
4. Do not use `cmake-build-debug` inside the SharePoint project tree.

CLion note:

- The plain Run output console is not a full ncurses terminal and can print raw escape codes instead of drawing the windows.
- The LLDB Debug console shown during `Debug` runs is also not a real ncurses terminal; it will show the escape stream instead of the Phase 1 subwindows.
- Use `Emulate terminal in output console`, run in an external terminal, or use `ULTIMA/Phase 1 (Scheduler)/run_ultima_ui.command` on macOS.
- For a team-wide shell entrypoint that works in macOS Terminal and Cygwin shells, use `ULTIMA/Phase 1 (Scheduler)/run_ultima_ui.sh`.

Preferred presentation layout at `140 x 62`:

- `Header`: `5 x 138` at `(y=0, x=1)`
- `Scheduler`: `7 x 138` at `(5, 1)`
- `Semaphore`: `7 x 138` at `(12, 1)`
- `State + Race + Memory`: `7 x 138` at `(19, 1)`
- `Task_A`: `7 x 138` at `(26, 1)`
- `Task_B`: `7 x 138` at `(33, 1)`
- `Task_C`: `7 x 138` at `(40, 1)`
- `Output Log + Dumps`: `9 x 138` at `(47, 1)`
- `Controls`: `6 x 138` at `(56, 1)`

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

## Phase 1 Module Summary

- `Sched.h` / `Sched.cpp`: round-robin scheduler, task-state transitions, and process-table dumps.
- `Sema.h` / `Sema.cpp`: FIFO binary semaphore for `Printer_Output`.
- `Mem_mgr.h` / `Mem_mgr.cpp`: deterministic first-fit memory manager used by the demo to allocate, track, and free one buffer per task.
- `Ultima.cpp`: ties the scheduler, semaphore, and memory manager together in the live proof-board and transcript flow.

## Phase 3 Module Summary

- `ULTIMA/Phase 3 (Memory Management)/mmu.h` / `mmu.cpp`: 1024-byte first-fit MMU with fixed 64-byte blocks, linked-list segment tracking, base/limit-style protection, coalescing, and char/hex dumps.
- `ULTIMA/Phase 3 (Memory Management)/Phase3_main.cpp`: ncurses demonstration showing scheduler, semaphores, IPC, MMU ledger, core dump, cumulative testing checkpoints, and event log in one stepped UI.
- `ULTIMA/Phase 3 (Memory Management)/phase3_smoke_test.cpp`: headless cumulative validation covering IPC, successful allocation, blocked allocation, shark-pond wake-up, coalescing, and protection faults.
