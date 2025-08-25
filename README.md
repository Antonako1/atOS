# atOS-RT - atOS Revised Technology

A custom 32-bit operating system written in C and assembly language, designed without security constraints to allow deep low-level exploration and experimentation.

---

## Table of Contents

* [Overview](#overview)
* [Features](#features)
* [Running atOS-RT](#running-atos-rt)
* [Development](#development)
  * [Dependencies](#dependencies)
  * [Building](#building)
  * [Debugging](#debugging)
* [License](#license)

---

## Overview

**atOS-RT** is a 32-bit operating system built in **C** and **assembly language** from the ground up. This open-source project is crafted for enthusiasts and developers who want to dive deep into low-level system programming without the typical security restrictions of modern OSes. With atOS-RT, you have complete freedom to explore and modify every aspect of the system.

---

## Features

* **32-bit Architecture**: Targeted specifically for 32-bit x86 processors.
* **Custom Programming Language**: Includes a unique integrated language for OS-level application development.
* **No Security Constraints**: Designed for ultimate flexibility, allowing unrestricted operations—ideal for experimental and educational purposes.
* **Open Source**: Released under the MIT License, encouraging developers to explore, contribute, and learn how operating systems function at a low level.

---

## Running atOS-RT

Run the provided ISO image in a virtual machine environment. QEMU is recommended for simplicity, though other VMs may work.

### System Requirements

Requirements for the Virtual Machine.

|     | Recommended | Minimum |
| --- | ----------- | ------- |
| RAM | 512 MiB     | 128 MiB |
| CPU | 1 Core      | 1 Core  |
| HDD | 256 MiB     | 128 MiB  |

### Installing QEMU

On Debian/Ubuntu-based systems:

```bash
sudo apt install qemu-system-x86
```

### Running atOS-RT


Run atOS-RT with qemu:

```bash
qemu-img create -f raw hdd.img 256M
qemu-system-i386 \
  -boot d \
  -vga std \
  -cdrom atOS-RT.iso \
  -drive file=hdd.img,format=raw,if=ide,index=0,media=disk \
  -m 512
```

---

## Development

Interested in contributing or exploring atOS-RT’s internals? Here's how to get started.

### Dependencies

Install the essential tools:

```bash
sudo apt install qemu-system-x86 nasm make gcc genisoimage
```

| Tool            | Purpose                        |
| --------------- | ------------------------------ |
| qemu-system-x86 | Running the ISO                |
| make            | Build automation               |
| nasm            | Assembler                      |
| gcc             | Toolchain for utility programs |
| genisoimage     | ISO file generation            |

### Building

The project uses `make` with convenient targets.

* **List all commands:**

  ```bash
  make help
  ```

* **Build and run the ISO:**

  ```bash
  make iso run
  ```

### Debugging

Not needed. Not supported. Print to screen.

---

### Documentation

Find documentation in the DOCS directory and within each source directory's README.

## License

This project is licensed under the **MIT License**. See the `LICENSE` file for full details.

---
