32-bit kernel folder.

Contains files used by the 32-bit kernel and its entry file

- CPU\
  - CPU initialization code used by the kernel
- Drivers\
  - Drivers used by the kernel
- FS\
  - Filesystem used by the kernel
- MEMORY\
  - Memory definitions and functions
- SYSCALL\
  - Syscalls

Additional subsystems:
- `DEBUG/` — Minimal kernel debug I/O helpers (I/O port 0xE9; also mirrors to COM1 when present). See `DOCS/DEBUGGING.md` for QEMU flags and log details.
- `DRIVERS/AC97/` — AC'97 audio driver initialization and simple playback helpers.
