# CPU

## Overview

This directory contains CPU-specific implementations and configurations for the RTOS kernel. It includes assembly routines, context switching mechanisms, and interrupt handling tailored to the target architecture.

Do NOT compile or link any files against NON-KERNEL code. Doing so will result in a broken kernel.  
Files in this directory are compiled using KERNEL defines and flags, trying to compile them against user-space code will result in errors.

## Directory Structure

- `FPU/`: Contains code for initializing and managing the FPU (Floating Point Unit).
- `GDT/`: Contains code for setting up the Global Descriptor Table (GDT) for memory segmentation.
- `IDT/`: Contains code for setting up the Interrupt Descriptor Table (IDT)
- `INTERRUPT/`: Contains code for handling CPU interrupts and exceptions.
- `IRQ/`: Contains code for handling hardware interrupts (IRQs).
- `ISR/`: Contains code for handling Interrupt Service Routines (ISRs) and dispatching them to appropriate handlers.
- `PIC/`: Contains code for programming the Programmable Interrupt Controller (PIC).
- `PIT/`: Contains code for programming the PIT.
- `STACK/`: Contains code for setting up stack. Not in use...
- `SYSCALL/`: Contains code for handling system calls from user-space applications.
- `TSS/`: Contains code for setting up the Task State Segment (TSS) for task management. Not implemented yet.
- `YIELD/`: Contains code for yielding CPU control between tasks.
