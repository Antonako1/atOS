# Debugging atOS

Debugging support is very minimal, due to the OS running in a very constrained environment (raw binary, no standard libraries, etc). However, there are some basic debugging facilities available.

## Kernel Debug Output (KDEBUG)

The kernel now emits lightweight debug output to the Bochs/QEMU debug I/O port `0xE9`. It also mirrors to the first serial port (COM1) inside the guest, but the default run targets do not capture serial output to a host file.

- QEMU flags (already used by `make run`):
- `-debugcon file:OUTPUT/DEBUG/debug.log`
- `-global isa-debugcon.iobase=0xe9`

Files are written under `OUTPUT/DEBUG/` so you can tail them while the VM runs:

```bash
tail -f OUTPUT/DEBUG/debug.log
```

Kernel usage:
- `KDEBUG_INIT()` — initializes COM1 (115200 8N1) and is called early in `rtos_kernel()`
- `KDEBUG_PUTS(const U8 *s)` — prints a string (adds CR before LF to help some terminals)
- `KDEBUG_PUTC(U8 c)` — prints a single character
- `KDEBUG_HEX32(U32 v)` — prints a 32-bit value as hex prefixed with `0x`

This is intended for early, low-overhead logging that does not depend on the graphics stack.

### Shell Bootstrap Markers

The kernel logs around user shell launch to help isolate boot hangs:

- `[rtos] Enter LOAD_AND_RUN_KERNEL_SHELL` — entering loader
- `[rtos] Screen flushed` — VBE framebuffer cleared/flushed
- `[rtos] Loading shell file record...` — ISO9660 lookup started
- `[rtos] Shell file loaded, size=0xXXXXXXXX` — file found and size determined
- `[rtos] Starting RUN_BINARY(atOShell) ...` — process creation starting
- `[proc] RUN_BINARY enter name="atOShell" size=0x...` — RUN_BINARY called with sizes
- `[proc] setup_user_process...` and `[proc] setup_user_process OK` — user address space and binary mapping
- `[proc] added to scheduler pid=0xXXXXXXXX` — shell enqueued in scheduler
- `[rtos] RUN_BINARY returned OK` and `[rtos] LOAD_AND_RUN_KERNEL_SHELL done` — control returned to kernel loop

If the log stops before RUN_BINARY, investigate ISO9660 read or memory allocation. If it stops inside RUN_BINARY, instrument process setup (paging, mapping, or initial trap frame) further.

## QEMU Debugging

When running atOS in Qemu, you can use the built-in compact-monitor for debugging. To enable it, press View -> compactmonitor0 or Ctrl + Alt + 2. This will open a console window inside Qemu.

To return to the main display, press Ctrl + Alt + 1.

### Useful Commands

- `info registers` - Displays the current state of the CPU registers.
- `info mem` - Displays memory mappings.
- `xp <format> <address>` - Examines memory at the specified address.
    - `<format>` can be `x` (hex), `d` (decimal), `u` (unsigned decimal), `o` (octal), `t` (binary), or `f` (float).
    - Example: `xp /16bx 0x00100000` will display 16 bytes in hexadecimal starting from address `0x00100000`.
- `stop` - Pauses the execution of the program.
- `c` - Resumes execution of the program.


### Debugging Kernel Panics

When the kernel encounters a critical error, it will display a panic message on the screen. The message shows file and line number, with an error code if applicable and error text.

### Debugging overall Kernel

This is can be really tricky, but easier than debugging user programs. Below are some tips and tricks I use to debug the kernel:

#### Locating the issue

Locate the last successful operation: Identify the last operation that completed successfully before the issue occurred. This can help narrow down the potential causes of the problem. If you have problems locating it, see `In-Depth of locating the issue` below.

#### In-Depth of locating the issue

Check register states: Use the Qemu monitor to inspect the CPU registers. 

ESP should be a value between `0x00F44000`-`0x00F34000` (the stack area). If anything else is shown, the stack is corrupted, and you should look for stack overflows or invalid memory accesses via brute force.

EIP should be a value between `0x00100000`-`0x00550000` (the kernel image area). If anything else is shown, the code execution has jumped to an invalid location, and you should look for buffer overflows or invalid function pointers via brute force.

If the values are within the expected ranges, you can use the `xp` command to inspect memory around the ESP and EIP values. Look for any anomalies or unexpected values that could indicate memory corruption.

To help in trying to locate the issue, try these following ways:

##### Way 1: Dump memory around EIP
1. Type `xp /256xb <EIP>` to dump 1024 bytes of memory starting from the EIP address. This can help you see the instructions being executed and identify any anomalies.

2. Copy the output to a text file and use a disassembler (like [Online Disassembler](https://defuse.ca/online-x86-assembler.htm#disassembly2)) to analyze the instructions. Look for any suspicious or unexpected instructions that could indicate a problem or similarities between the kernel code to locate it.

You will get an output like this:

```
0000000000106354: 0x90 0xc9 0xc3 0x55 0x89 0xe5 0x83 0xec
000000000010635c: 0x08 0x6a 0x1f 0x68 0xff 0xff 0x00 0x00
...
```

Remove the addresses and space, then replace `0x` with `\x` to get a format like this:
```
\x90 \xc9 \xc3 \x55 \x89 \xe5 \x83 \xec
\x08 \x6a \x1f \x68 \xff \xff \x00 \x00
...
```

Your code is now in a literal byte format that can be used in the [disassembler](https://defuse.ca/online-x86-assembler.htm#disassembly2).

##### Way 2: Calculate offset from kernel base

1. Calculate the offset of the EIP from the kernel base address (0x00100000). For example, if EIP is `0x00106354`, the offset is `0x00006354`.

2. Open the kernel binary and locate the offset using a hex editor or binary viewer. Copy the surrounding bytes (e.g., 256 bytes before and after the offset) as literal bytes, then use a disassembler to analyze the instructions.

##### Way 3: Use NOP instructions

1. If you have a rough idea of where the issue might be located, but aren't sure, you can try adding NOP (No Operation) instructions to the suspected area of code. This can help you determine if the issue is related to a specific section of code. This is a brute-force method and can be time-consuming, but it can be effective in some cases. It can be found in the `SOURCE/STD/ASM.h` file, as NOP.

#### Additional Debugging Techniques

Now that you have hopefully located the issue, you can use the following techniques to further investigate and resolve it:

1. Add debug output: Locate the exact line with halting the system and seek what operation is being performed. Add debug output (e.g., using `VBE_DRAW_LINE` (Remember to update VRAM)) before and after the operation to trace its execution. This can help identify where the issue occurs. Another great function to use is `panic_debug`, which prints a message and a number to the screen.

2. Use panic_if: The `panic_if` function can be used to check for specific conditions and trigger a panic if the condition is met. This can help catch errors early and provide useful information about the state of the system when the panic occurs. This is located in `32RTOSKRNL/RTOSKRNL_INTERNAL.h.`
