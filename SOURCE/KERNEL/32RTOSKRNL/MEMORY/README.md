| Start (inclusive)     | End (exclusive)         | Label     | Size  | Notes |
| --------------------- | --------------| ----------- | --------------| ----|
| `0x00000000`          | `0x00001000`            | Low Memory Reserved (IVT+BDA)  | 4 KiB                         | Real-mode tables          |
| `0x00001000`          | `0x00002000`            | Bootloader (stage 1)           | 4 KiB                         |                           |
| `0x00002000`          | `0x00004000`            | Stage 2 loader buffers         | 12 KiB                        |                           |
| `0x00006000`          | `0x00007000`            | E820 map copy                  | 4 KiB                         |                           |
| `0x00007000`          | `0x0000C000`            | Kernel entry stub              | 17 KiB                        |                           |
| `0x0009FC00`          | `0x00100000`            | BIOS reserved                  | ~0.4 MiB                      |                           |
| `0x00100000`          | `0x00550000`            | **Kernel image**               | ~4.3 MiB                      | Text/data sections        |
| `0x00550000`          | `0x08550000`            | **Kernel heap / kmalloc pool** | **128 MiB**                   | Dynamic allocations, page allocator, slabs, IPC buffers |
| `0x08550000`          | `0x08560000`            | Guard page below kernel stack  | 4 KiB                         |                           |
| `0x08560000`          | `0x08570000`            | Kernel stack                   | 64 KiB                        |                           |
| `0x08570000`          | `0x08580000`            | Guard page above kernel stack  | 4 KiB                         |                           |
| `0x08580000`          | `0x0B000000`            | Framebuffer (LFB)              | ~40 MiB max                   | If you map 1920×1080×4 B  |
| `0x0B000000`          | `0x10000000`            | MMIO / Device-reserved         | ~80 MiB                       | PCI/graphics/apic areas   |
| `0x10000000`          | `0x20000000`            | **User space start**           | ~256 MiB                      | For processes             |
| `0x20000000`          | `0x30000000`            | **Library space start**        | ~256 MiB                      | For libraries             |
| `0x30000000` → beyond | Free / future expansion | (512 MiB +)                    | Only present if > 512 MiB RAM |                           |
