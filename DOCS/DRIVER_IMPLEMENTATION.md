# Driver Development Guide

This document explains how to **create drivers** for the operating system based on the example implementations:
- **ATA PIO** (Programmed I/O) driver  
- **ATA PIIX3** (Bus Master DMA) driver  
- **CMOS (RTC)** driver  
- **RTL8139** (Network) driver  
- **AC'97** (Audio) driver  

Each driver follows a consistent **driver architecture** within the kernel and uses ONLY kernel usable functions:
- `PCI` for device discovery and configuration
- `PIC` and `ISR` for interrupt handling
- `KHEAP` for memory allocation
- `ASM` for hardware I/O and other cpu tasks
- `MEM` and `STRING` for utility functions

---

## 🧩 1. Driver Structure

Each driver consists of:
- A **header file** in `DRIVERS/<name>/<name>.h`  
  Defines macros, constants, data structures, and public APIs.
- A **source file** in `DRIVERS/<name>/<name>.c`  
  Implements initialization, data transfer, and IRQ handling.

Example:
```
DRIVERS/
 ├── ATA_PIO/
 │   ├── ATA_PIO.c
 │   └── ATA_PIO.h
 ├── ATA_PIIX3/
 │   ├── ATA_PIIX3.c
 │   └── ATA_PIIX3.h
 └── CMOS/
     ├── CMOS.c
     └── CMOS.h
```

---

## ⚙️ 2. Required Components

All hardware drivers typically depend on these kernel subsystems:

| Module | Purpose |
|--------|----------|
| `PCI` | Discover and configure devices on the PCI bus |
| `PIC` | Manage interrupt masking and end-of-interrupt signaling |
| `ISR` | Register interrupt handlers |
| `KHEAP` | Allocate memory for buffers and descriptor tables |
| `MEM` / `STRING` | Provide `MEMCPY`, `MEMZERO`, `MEMCMP` utilities |
| `ASM` | Provide `_inb`, `_outb`, `_inw`, `_outw`, and CPU flags control (`CLI`, `STI`, `HLT`) |

---

## 🧠 3. Common Driver Initialization Pattern

Each driver must provide an initialization function that:
1. **Locates hardware** (via PCI or I/O probing)
2. **Configures registers**
3. **Allocates memory** if needed
4. **Registers interrupts** if applicable
5. **Returns TRUE/FALSE** based on success

Example (from `INIT_RTC`):
```c
BOOLEAN INIT_RTC(VOID) {
    CLI;
    MEMZERO(&rtc, sizeof(RTC_REG));

    NMI_disable();

    U8 status_b = cmos_read_register(RTC_REG_B);
    status_b |= (RTC_REG_B_24H | RTC_REG_B_DM | RTC_REG_B_PIE);
    status_b &= ~(RTC_REG_B_SET | RTC_REG_B_UIE | RTC_REG_B_SQWE); // Ensure SET, UIE, SQWE are clear
    cmos_write_register(RTC_REG_B, status_b);
    rtc.reg_b = status_b;

    U8 status_a = cmos_read_register(RTC_REG_A);
    status_a = (status_a & (RTC_REG_A_UIP | 0xF0)) | 0x06; // Preserve UIP, other bits 4-7, set RS=0x06 (1024 Hz)
    cmos_write_register(RTC_REG_A, status_a);
    rtc.reg_a = status_a;

    cmos_read_register(RTC_REG_C);

    NMI_enable();
    
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + 8, RTC_HANDLER);
    PIC_Unmask(8);
    STI;
    return TRUE;
}
```

---

## ⚡ 4. Interrupt Handling

Drivers that depend on hardware interrupts must:
- Implement an IRQ handler of the form:
  ```c
  VOID <DEVICE>_HANDLER(U32 vector, U32 errcode);
  ```
- Acknowledge the interrupt in PIC:
  ```c
  pic_send_eoi(vector - PIC_REMAP_OFFSET);
  ```
- Clear or read device status registers to reset IRQ flags.

Example (from CMOS RTC handler):
```c
VOID RTC_HANDLER(U32 vector, U32 errcode) {
    cmos_get_date_time(&rtc.t);

    cmos_read_register(RTC_REG_C);
    
    pic_send_eoi(vector - PIC_REMAP_OFFSET);
}
```

---

## 💾 5. Memory and Buffer Management

Use kernel heap functions for allocating DMA buffers and descriptor tables:
```c
PRDT = (PRDT_ENTRY*)KMALLOC_ALIGN(sizeof(PRDT_ENTRY), 0x1000);
DMA_BUFFER = KMALLOC_ALIGN(BUS_MASTER_LIMIT_PER_ENTRY, 0x1000);
MEMZERO(PRDT, sizeof(PRDT_ENTRY));
MEMZERO(DMA_BUFFER, BUS_MASTER_LIMIT_PER_ENTRY);
```

Alignment is critical for DMA and hardware buffers — always use aligned allocations with constants defined in the driver header. These must be freed with `KFREE_ALIGN`

If alignment is not required, use KMALLOC and KFREE

---

## 🔍 6. PCI Device Discovery

For PCI-based drivers (e.g., ATA PIIX3, RTL8139):
1. Iterate over PCI devices:
   ```c
   for (U32 i = 0; i < PCI_GET_DEVICE_COUNT(); i++) {
       PCI_DEVICE_ENTRY* dev = PCI_GET_DEVICE(i);
       if (dev->header.class_code == PCI_CLASS_MASS_STORAGE &&
           dev->header.subclass  == PCI_SUBCLASS_IDE) {
           // Match device
       }
   }
   ```
2. Extract BAR (Base Address Register) to get I/O or MMIO base address.
3. Enable bus mastering with `PCI_ENABLE_BUS_MASTERING`.

---

## 🧮 7. I/O Operations

Use `_inb`, `_outb`, `_inw`, `_outw` for port I/O.
Example:
```c
_outb(ATA_PRIMARY_BASE + ATA_SECCOUNT, sectors);
_outb(ATA_PRIMARY_BASE + ATA_LBA_LO, (U8)(lba & 0xFF));
```

For devices using memory-mapped I/O (MMIO), replace with read/write to physical memory via mapped addresses.

---

## ⏱️ 8. Polling and Synchronization

For non-interrupt devices, use polling:
```c
while ((_inb(base + ATA_COMM_REG) & STAT_BSY) && timeout--) {}
```

When IRQs are not available use `yield()` or:
```c
while (!dma_done) cpu_relax()
```

---

## 🧩 9. Public API Design

Each driver exposes high-level APIs for kernel usage.

Example (ATA):
```c
BOOLEAN ATA_PIIX3_READ_SECTORS(U32 lba, U8 sectors, VOIDPTR out);
BOOLEAN ATA_PIIX3_WRITE_SECTORS(U32 lba, U8 sectors, VOIDPTR in);
U32 ATA_GET_IDENTIFIER(VOID);
```

Example (RTC):
```c
BOOLEAN INIT_RTC(VOID);
RTC_DATE_TIME GET_SYS_TIME();
```

All public functions are declared in the driver’s `.h` file.

---

## 🌐 10. Example: Creating a New Driver

Let’s outline a simple **PS/2 Keyboard** driver structure.

### Header (`DRIVERS/KEYBOARD/KEYBOARD.h`)
```c
#ifndef KEYBOARD_DRIVER_H
#define KEYBOARD_DRIVER_H

#include <STD/TYPEDEF.h>

#define KEYBOARD_IRQ 1
BOOLEAN INIT_KEYBOARD(VOID);
VOID KEYBOARD_HANDLER(U32 vector, U32 errcode);
U8 READ_KEYCODE(VOID);

#endif
```

### Source (`DRIVERS/KEYBOARD/KEYBOARD.c`)
```c
#include <DRIVERS/KEYBOARD/KEYBOARD.h>
#include <CPU/ISR/ISR.h>
#include <CPU/PIC/PIC.h>
#include <STD/ASM.h>

static volatile U8 last_key = 0;

VOID KEYBOARD_HANDLER(U32 vector, U32 errcode) {
    (void)errcode;
    last_key = _inb(0x60); // Read key from PS/2 data port
    pic_send_eoi(KEYBOARD_IRQ);
}

BOOLEAN INIT_KEYBOARD(VOID) {
    ISR_REGISTER_HANDLER(PIC_REMAP_OFFSET + KEYBOARD_IRQ, KEYBOARD_HANDLER);
    PIC_Unmask(KEYBOARD_IRQ);
    return TRUE;
}

U8 READ_KEYCODE(VOID) {
    return last_key;
}
```

This minimal example shows the typical steps:
- Register an interrupt handler.
- Unmask IRQ in PIC.
- Handle hardware events in ISR.

---

## ✅ 11. Driver Guidelines

- Always **verify hardware presence** (PCI or probe).
- **Avoid blocking indefinitely** — use timeouts.
- **Acknowledge all interrupts** properly.
- Use `KMALLOC_ALIGN` for hardware structures if required `KMALLOC` in other cases.
- Define **clear macros and constants** for readability.
- Keep **ISR minimal** — defer heavy work to a kernel thread if available.

---

## 🧮 12. Syscall Assignment and Usage

The syscall system provides a bridge between **user processes**, such as process management, memory allocation, interprocess communication (IPC), and drivers.

Each syscall is assigned a unique **system call ID** and mapped to its **handler function** inside the `syscall_table`. This mapping allows processes and subsystems to call kernel functions safely via interrupt `0x80`.

### 🧱 Syscall Table Definition

Each syscall is defined using the `SYSCALL_ENTRY` macro inside `SYSCALL_LIST.h`, for example:

```c
// Messaging
SYSCALL_ENTRY(SYSCALL_MESSAGE_AMOUNT, SYS_MESSAGE_AMOUNT)
SYSCALL_ENTRY(SYSCALL_GET_MESSAGE, SYS_GET_MESSAGE)
SYSCALL_ENTRY(SYSCALL_SEND_MESSAGE, SYS_SEND_MESSAGE)

// Memory
SYSCALL_ENTRY(SYSCALL_KMALLOC, SYS_KMALLOC)
SYSCALL_ENTRY(SYSCALL_KFREE, SYS_KFREE)
```

During kernel initialization, this header is included into the syscall table:

```c
#define SYSCALL_ENTRY(id, fn) [id] = fn,
static SYSCALL_HANDLER syscall_table[SYSCALL_MAX] = {
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
};
#undef SYSCALL_ENTRY
```

This automatically binds syscall numbers (e.g. `SYSCALL_KMALLOC`) to their implementation (e.g. `SYS_KMALLOC`).

---

### ⚙️ Syscall Dispatcher

All system calls are handled through a **single dispatcher** that validates the syscall number and executes the correct handler:

```c
U32 syscall_dispatcher(U32 num, U32 a1, U32 a2, U32 a3, U32 a4, U32 a5) {
    if (num >= SYSCALL_MAX) return (U32)-1;
    SYSCALL_HANDLER h = syscall_table[num];
    if (!h) return (U32)-1;
    return h(a1, a2, a3, a4, a5);
}
```

Each handler must match the standard prototype:

```c
//                           eax, ebx, ecx, edx, esi, edi
typedef U32 (*SYSCALL_HANDLER)(U32, U32, U32, U32, U32);
```

---

### 🧩 Adding a New Syscall

To introduce a new syscall:

1. **Declare** it in `SYSCALL_LIST.h`:

   ```c
   SYSCALL_ENTRY(SYSCALL_MY_NEW_FEATURE, SYS_MY_NEW_FEATURE)
   ```

2. **Implement** the corresponding function:

   ```c
   U32 SYS_MY_NEW_FEATURE(U32 a1, U32 a2, U32 a3, U32 a4, U32 a5) {
       return my_new_feature((U8*)a1, (U32)a2);
   }
   ```

3. **Expose** it to user-space using a macro or helper:

   ```c
   #define MY_NEW_FEATURE(ptr, len) SYSCALL2(SYSCALL_MY_NEW_FEATURE, ptr, len)
   ```

---

### 🧠 Syscall Assignment Guidelines

As of now, no clear guidelines are, but add them inside their own corresponding comment block

---

### 💡 Example: Process Communication Syscalls

Example syscall pair for sending/receiving messages:

```c
// Declaration in SYSCALL_LIST.h
SYSCALL_ENTRY(SYSCALL_SEND_MESSAGE, SYS_SEND_MESSAGE)
SYSCALL_ENTRY(SYSCALL_GET_MESSAGE, SYS_GET_MESSAGE)

// Implementation
U32 SYS_SEND_MESSAGE(U32 msg_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    if (!msg_ptr) return (U32)-1;
    PROC_MESSAGE *msg = (PROC_MESSAGE *)msg_ptr;
    send_msg(msg);
    return 0;
}

U32 SYS_GET_MESSAGE(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    TCB *t = get_current_tcb();
    if (!t || !t->msg_count) return 0;
    PROC_MESSAGE *msg = dequeue_message(t);
    return (U32)msg;
}
```

Process access:

```c
PROC_MESSAGE *msg = GET_MESSAGE();
if (msg) {
    // Process message
    FREE_MESSAGE(msg);
}
```

---

### 🧾 Summary

The syscall system:

* Provides **safe kernel entry points** for user processes.
* Uses **a uniform dispatch interface** for up to 255 syscalls.
* Allows **processes and drivers** to share kernel-level services.
* Ensures **consistent syscall ID organization** across all subsystems.

When adding new syscalls, always:

* Define them in `SYSCALL_LIST.h`
* Implement the `SYS_` prefixed function
* Expose it through user-space wrappers in the appropriate subsystem

---

## 🔊 AC'97 Audio Driver Notes

The AC'97 driver targets Intel ICH-compatible controllers exposed via PCI. It performs:

- PCI discovery for devices with class `Multimedia` and subclass `Audio`.
- Extraction of BAR0/BAR1 as I/O ports for NAM (mixer) and NABM (bus master).
- Codec reset and power-up via NAM registers.
- DMA setup for PCM out using a Buffer Descriptor List (BDL) programmed into NABM.
- IRQ installation for buffer completion notifications.

QEMU run requirements (already present in `make run`):

```bash
-device ac97,audiodev=snd0 -audiodev sdl,id=snd0
```

Key registers and concepts (subset):
- NAM reset: `NAM_RESET (0x00)`
- Sample rate: `NAM_FRONT_DAC_RATE (0x2C)`
- Volumes: `NAM_MASTER_VOLUME (0x02)`, `NAM_PCM_OUT_VOLUME (0x18)`
- BDL base: `NABM_PO_BDBAR (0x10)`
- Control: `NABM_PO_CR (RUN/IOCE/LVBIE)`
- Status: `NABM_PO_SR (BCIS/LVBCI)`

Driver API:
- `BOOLEAN AC97_INIT(void);` — discover device, reset codec, allocate DMA, enable IRQs.
- `BOOLEAN AC97_PLAY(const U16* pcm, U32 frames, U8 channels, U32 rate);` — play 16-bit stereo PCM.
- `BOOLEAN AC97_TONE(U32 freq, U32 duration_ms, U32 rate, U16 amp);` — generate a square tone.
- `VOID AC97_STOP(void);` — stop playback and clear status.

Syscalls exposed to userland:
- `SYSCALL_AC97_TONE` → `SYS_AC97_TONE(freq, ms, amp, rate)`
- `SYSCALL_AC97_STOP` → `SYS_AC97_STOP()`

IRQ handling:
The IRQ handler acknowledges PIC and clears NABM status bits. It also tracks buffer completions to stop playback when a finite buffer set has played out.

See: `SOURCE/KERNEL/32RTOSKRNL/DRIVERS/AC97/AC97.c` and `AC97.h`.

---

## 📚 13. Summary

| Task | Typical Function | Example Driver |
|------|------------------|----------------|
| Device Discovery | `PCI_GET_DEVICE` | ATA_PIIX3, RTL8139 |
| Initialization | `*_INIT()` | ATA_PIO_INIT, INIT_RTC |
| Data Transfer | `*_XFER()` | ATA_PIO_XFER, ATA_PIIX3_XFER |
| Interrupt Handling | `ISR_REGISTER_HANDLER()` | ATA_IRQ_HANDLER |
| Memory Management | `KMALLOC_ALIGN`, `MEMZERO` | ATA_PIIX3 DMA Buffers |
| Timekeeping | `GET_SYS_TIME()` | CMOS Driver |

---

### 🧭 Summary Rule of Thumb
> **Discover → Initialize → Register IRQ → Operate → Expose as syscall → Cleanup**

Once this flow is mastered, creating any new driver becomes a predictable and modular process within the OS’s driver framework.
