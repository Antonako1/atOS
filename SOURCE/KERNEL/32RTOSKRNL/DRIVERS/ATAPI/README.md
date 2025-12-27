# ATAPI

ATAPI driver for RTOS kernel.

## Overview

This driver provides support for ATAPI devices, such as CD-ROM drives within the RTOS kernel. It enables communication between the operating system and ATAPI-compliant hardware, allowing for data transfer and device management.

## Features

- Support for a small range of ATAPI commands, including:
    - Read(12)
    - Inquiry, used to identify the device type
    
It can be extended to support additional ATAPI commands as needed.

## Helpful information

The whole ATA system can support 4 drives, 2 per channel (Primary and Secondary). Each channel can have a master and a slave drive.

