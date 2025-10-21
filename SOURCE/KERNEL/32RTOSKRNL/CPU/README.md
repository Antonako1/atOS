# CPU

## Overview

This directory contains CPU-specific implementations and configurations for the RTOS kernel. It includes assembly routines, context switching mechanisms, and interrupt handling tailored to the target architecture.

Do NOT compile or link any files against NON-KERNEL code. Doing so will result in a broken kernel.  
Files in this directory are compiled using KERNEL defines and flags, trying to compile them against user-space code will result in errors.

## Directory Structure

- `GDT/`: Contains code for setting up the Global Descriptor Table (GDT) for memory segmentation.
- `IDT/`: Contains code for setting up the Interrupt Descriptor Table (IDT)
- `TSS/`: Contains code for setting up the Task State Segment (TSS) for task management. Not implemented yet.
- `SYSCALL/`: Contains code for handling system calls from user-space applications.
- `ISR/`: Contains code for handling Interrupt Service Routines (ISRs) and dispatching them to appropriate handlers.
- `IRQ/`: Contains code for handling hardware interrupts (IRQs).
- `PIC/`: Contains code for programming the Programmable Interrupt Controller (PIC).
- `STACK/`: Contains code for setting up stack. Not in use...

## Notes on recent changes

- ISR dispatch now explicitly routes `int 0x80` to the syscall dispatcher when `__RTOS__` is defined. The syscall return value is written back to `EAX`.
- Default handlers are pre-registered for all IDT entries; IRQ vectors (0x20–0x2F) are wired to `irq_common_handler`, while exception vectors route to `isr_common_handler` unless specialized handlers are installed.
- New exception helpers added:
  - Divide error, NMI, invalid opcode, double fault, and FPU exceptions call into `panic_reg`/`panic` with clear messages.
  - Page fault handler reads `CR2`, decodes error bits, and overlays fault info via VBE drawing before halting.
- A dedicated `isr_syscall` assembly stub is provided for system calls and is installed at vector `0x80`.
