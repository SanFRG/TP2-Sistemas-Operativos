# Repository Guidelines

## Project Structure & Module Organization

This repository is an x64 OS project based on x64BareBones/Pure64. Main code is split by runtime layer:

- `Bootloader/`: Pure64 and BMFS bootloader sources.
- `Kernel/`: kernel C and NASM code. Submodules include `core/`, `drivers/`, `mm/`, `proc/`, `sync/`, `syscall/`, `lib/`, `asm/`, and public headers in `include/`.
- `Userland/UserModule/`: shell, commands, drivers, tests, userland assembly, and headers.
- `Toolchain/` and `Image/`: module packing and final bootable image generation.
- `Test/`: host-side CuTest unit tests for memory managers.
- `MemoryTest/`: userland/OS-level stress test sources from the course material.

Keep interfaces in the nearest `include/` directory and place implementation files in the matching module directory.

## Build, Test, and Development Commands

- `./compile.sh`: starts Docker container `TPE`, rebuilds the toolchain and OS, fixes image ownership, then runs QEMU.
- `make` or `make all`: builds bootloader, kernel, userland, and `Image/x64BareBonesImage.qcow2` with the default memory manager.
- `make buddy`: builds the image using `Kernel/mm/memoryManagerBuddy.c`.
- `make clean`: removes generated objects, binaries, and image artifacts.
- `./run.sh`: boots the generated image with `qemu-system-x86_64`.
- `cd Test && make run`: builds and runs host CuTest memory manager tests.
- `cd Test && make valgrind`: runs the same host tests under Valgrind.

After moving module files, update dependent Makefiles; for example, memory manager paths should stay aligned with `Kernel/mm/`.

## Coding Style & Naming Conventions

Use C99 and NASM conventions already enforced by the Makefiles. C code uses 4-space indentation, K&R-style braces, `snake_case` for functions and variables, and `_t` suffixes for typedefs such as `block_header_t`. Keep constants and command names lowercase unless hardware or ABI naming requires otherwise. Avoid libc assumptions: kernel and userland are built freestanding with `-nostdlib`, `-ffreestanding`, and no red zone.

## Testing Guidelines

Add host unit tests in `Test/MemoryManagerTest.c` and register them in `MemoryManagerTests[]`. Name tests descriptively with a `test...` prefix, for example `testAllocAfterFree`. Add OS-facing stress or integration tests under `Userland/UserModule/tests/` or adapt `MemoryTest/` sources only when their syscall wrappers match the current kernel ABI.

## Commit & Pull Request Guidelines

Recent history uses short imperative messages, often Conventional Commit style: `feat(process): ...`, `feat(tests): ...`, `refactor: ...`, and `docs: ...`. Prefer that format for new commits. Pull requests should describe the kernel/userland behavior changed, list the commands run, mention the selected memory manager, and include screenshots or terminal output when shell behavior or QEMU-visible output changes.
