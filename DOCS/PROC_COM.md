# Process Handling and Process Communication in atOS-RT

This document describes how **process management**, **inter-process communication (IPC)**, and **process–kernel–shell interaction** work in **atOS-RT**.

The system is split into two major layers:

* **User-side process communication (`STD/PROC_COM`)**
* **Kernel-side process management (`RTOSKRNL/PROC`)**

---

## 1. Overview

atOS-RT uses a **kernel-managed multitasking system** with:

* Task Control Blocks (TCBs)
* Message queues per process
* Cooperative + interrupt-driven scheduling
* Explicit message passing between:

  * Processes
  * Kernel
  * Shell

User processes are **flat binaries**, linked at a fixed virtual address and executed under kernel supervision.

---

## 2. STD/PROC_COM — User-side Process Communication

`STD/PROC_COM` is the **user-facing API** for:

* Process initialization and termination
* Sending and receiving messages
* Communicating with:

  * Other processes
  * The kernel
  * The shell

> ⚠️ **This API must NOT be used in kernel code**

### Responsibilities

* Initialize console or graphical process state
* Query process metadata (PID, name, parent)
* Send and receive messages
* Request kernel services (sleep, restart, focus, framebuffer, etc.)
* Interact with the shell (stdout, spawning processes)

---

## 3. Process Initialization & Lifetime

### Initialization

```c
void PROC_INIT_CONSOLE();
void PRIC_INIT_GRAPHICAL();
```

* Initializes the process environment
* Sets up console or graphical mode
* Automatically called by runtime in most cases

### State Queries

```c
BOOLEAN IS_PROC_INITIALIZED();
BOOLEAN IS_PROC_GUI_INITIALIZED();
```

### Termination

```c
VOID KILL_SELF();
VOID EXIT(U32 code);
VOID START_HALT();
```

* `KILL_SELF()` sends a termination request to the kernel
* `EXIT()` terminates the process with an exit code
* `START_HALT()` halts execution immediately.

---

## 4. Process Identity & Metadata

```c
U32 PROC_GETPID();
U8* PROC_GETNAME();
U32 PROC_GETPPID();
U8* PROC_GETPARENTNAME();
```

* Retrieve runtime information about the current process.

### Lookup by Name (path)

```c
U32 PROC_GETPID_BY_NAME(U8 *name);
```

Returns `-1` if not found.

---

## 5. Task Control Blocks (TCB)

A **TCB (Task Control Block)** represents a process internally.

### Accessing TCBs

```c
TCB *GET_CURRENT_TCB();
TCB *GET_MASTER_TCB();
TCB *GET_TCB_BY_PID(U32 pid);
TCB *GET_PARENT_TCB();
void FREE_TCB(TCB *tcb);
```

> ⚠️ TCBs obtained via `GET_TCB_BY_PID` must be freed using `FREE_TCB`.

---

## 6. Inter-Process Messaging (IPC)

atOS-RT uses **explicit message passing**, not shared queues or signals.

Each process has:

* A fixed-size message queue
* Messages delivered by the kernel

---

### Message Structure

```c
typedef struct proc_message {
    U32 sender_pid;
    U32 receiver_pid;
    PROC_MESSAGE_TYPE type;

    BOOLEAN data_provided;
    VOIDPTR data;
    U32 data_size;

    U32 signal;
    U32 timestamp;
    BOOLEAN read;

    VOIDPTR raw_data;
    U32 raw_data_size;
} PROC_MESSAGE;
```

---

### Creating Messages

Use helper macros:

```c
CREATE_PROC_MSG(receiver, type, data, size, signal)
CREATE_PROC_MSG_RAW(receiver, type, raw, size, signal)
```

* `data` is deep-copied by the kernel
* `raw_data` is **not copied** and must remain valid

---

### Sending Messages

```c
VOID SEND_MESSAGE(PROC_MESSAGE *msg);
```

Send to:

* Another process (`receiver_pid`)
* Kernel (`receiver_pid == 0`)

---

### Receiving Messages

```c
U32 MESSAGE_AMOUNT();
PROC_MESSAGE *GET_MESSAGE();
VOID FREE_MESSAGE(PROC_MESSAGE *msg);
```

* Messages are removed from the queue when fetched
* Receiver must free messages using `FREE_MESSAGE`

---

## 7. Message Types

### Kernel / Process Messages

Defined in `PROC_MESSAGE_TYPE`:

* `PROC_MSG_TERMINATE_SELF`
* `PROC_MSG_CREATE_PROCESS`
* `PROC_MSG_KILL_PROCESS`
* `PROC_MSG_SET_FOCUS`
* `PROC_MSG_SLEEP`
* `PROC_GET_FRAMEBUFFER`
* `PROC_FRAMEBUFFER_GRANTED`
* …

User-defined messages start at:

```c
0x100000
```

---

## 8. Shell Communication (SHELLCOM)

Processes interact with the shell using predefined commands.

### Shell Commands

```c
typedef enum {
    SHELL_CMD_CREATE_STDOUT = 0x100000,
    SHELL_CMD_SHELL_FOCUS,
    SHELL_CMD_INFO_ARRAYS,
    SHELL_CMD_EXECUTE_BATSH,
    ...
} COMMND_TYPE;
```

### Shell API

Please note that if process is created with shell, parent pid will always be your shell.
It is recommended that if you create a process inside your own program, the parent would be the shell

```c
BOOLEAN START_PROCESS(
    U8 *proc_name,
    VOIDPTR file,
    U32 bin_size,
    U32 initial_state,
    U32 parent_pid,
    PPU8 argv,
    U32 argc
);

VOID KILL_PROCESS(U32 pid);
STDOUT *GET_PROC_STDOUT();
SHELL_INSTANCE *GET_SHELL_HANDLE();
```

---

## 9. Kernel-side Process Management (RTOS_PROC)

`RTOS_PROC` is the **kernel implementation** of:

* Multitasking
* Scheduling
* Context switching
* Memory management per process
* Message routing

> ⚠️ **Kernel-only — not accessible to user programs**

---

### Task States

```c
TCB_STATE_ACTIVE
TCB_STATE_WAITING
TCB_STATE_SLEEPING
TCB_STATE_ZOMBIE
TCB_STATE_IMMORTAL
```

---

### Context Switching

* Triggered by PIT interrupts
* Uses `TrapFrame` structure
* Kernel saves and restores CPU + segment registers

---

## 10. Memory Model & Paging (Summary)

* Kernel is identity-mapped
* User processes are linked at:

  ```
  0x10000000
  ```
* User libraries live at:

  ```
  0x20000000
  ```
* Each process has:

  * Its own page directory
  * Isolated heap and stack
  * Shared kernel mappings

This allows:

* Process isolation
* Shared libraries
* Shared kernel memory

---

## 11. Design Philosophy

* Explicit message passing (no implicit signals)
* Fixed memory layout for simplicity
* Kernel remains authoritative
* User processes are sandboxed but cooperative
* Debuggable and inspectable by design

---

## 12. Summary

| Layer          | Purpose                     |
| -------------- | --------------------------- |
| `STD/PROC_COM` | User process communication  |
| `SHELLCOM`     | Process ↔ Shell interaction |
| `RTOS_PROC`    | Kernel multitasking & IPC   |
| Paging         | Memory isolation            |

This system provides a **clean, explicit, low-level RTOS-style process model** suitable for experimentation, OS research, and educational kernels.

---

