# Pageframe Management in 32RTOS Kernel

This document describes the physical page frame management system used in the 32RTOS kernel. The PAGEFRAME module is responsible for tracking and managing the allocation and deallocation of physical memory pages, which are the fundamental units of memory in a paged memory system.

Since this is a crucial part of the memory management system, I have written more detailed documentation in the [PAGING](../PAGING/README.md) module, which explains how virtual memory and paging structures interact with physical page frames.