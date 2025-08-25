/*+++
Dumps VESA/VBE video modes and information.

Compile:
    gcc -o dump_vesa_modes TOOLS/DUMP_MODES/DUMP_MODES.c -O2 -Wall -Wextra

Usage:
    ./dump_vesa_modes <mode>

This program will print the available video modes and their details.
It assumes that the video modes are stored at a specific memory address.
Values for VESA video mode:
00h-FFh OEM video modes (see #00010 at AH=00h)
100h   640x400x256
101h   640x480x256
102h   800x600x16
103h   800x600x256
104h   1024x768x16
105h   1024x768x256
106h   1280x1024x16
107h   1280x1024x256
108h   80x60 text
109h   132x25 text
10Ah   132x43 text
10Bh   132x50 text
10Ch   132x60 text
---VBE v1.2+ ---
10Dh   320x200x32K
10Eh   320x200x64K
10Fh   320x200x16M
110h   640x480x32K
111h   640x480x64K
112h   640x480x16M
113h   800x600x32K
114h   800x600x64K
115h   800x600x16M
116h   1024x768x32K
117h   1024x768x64K
118h   1024x768x16M
119h   1280x1024x32K (1:5:5:5)
11Ah   1280x1024x64K (5:6:5)
11Bh   1280x1024x16M
---VBE 2.0+ ---
120h   1600x1200x256
121h   1600x1200x32K
122h   1600x1200x64K
81FFh   special full-memory access mode
---*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("Usage: %s <mode>\n", argv[0]);
        return 1;
    }

    printf("Arguments:\n");
    for (int i = 0; i < argc; i++) {
        printf("  argv[%d]: %s\n", i, argv[i]);
    }

    printf("Available video modes:\n");
    printf("100h - 640x400x256\n");
    printf("101h - 640x480x256\n");
    printf("102h - 800x600x16\n");
    printf("103h - 800x600x256\n");
    printf("104h - 1024x768x16\n");
    printf("105h - 1024x768x256\n");
    printf("106h - 1280x1024x16\n");
    printf("107h - 1280x1024x256\n");
    printf("108h - 80x60 text\n");
    printf("109h - 132x25 text\n");
    printf("10Ah - 132x43 text\n");
    printf("10Bh - 132x50 text\n");
    printf("10Ch - 132x60 text\n");
    printf("10Dh - 320x200x32K\n");
    printf("10Eh - 320x200x64K\n");
    printf("10Fh - 320x200x16M\n");
    printf("110h - 640x480x32K\n");
    printf("111h - 640x480x64K\n");
    printf("112h - 640x480x16M\n");
    printf("113h - 800x600x32K\n");
    printf("114h - 800x600x64K\n");
    printf("115h - 800x600x16M\n");
    printf("116h - 1024x768x32K\n");
    printf("117h - 1024x768x64K\n");
    printf("118h - 1024x768x16M\n");
    printf("119h - 1280x1024x32K (1:5:5:5)\n");
    printf("11Ah - 1280x1024x64K (5:6:5)\n");
    printf("11Bh - 1280x1024x16M\n");
    printf("120h - 1600x1200x256\n");
    printf("121h - 1600x1200x32K\n");
    printf("122h - 1600x1200x64K\n");
    printf("81FFh - special full-memory access mode\n");

    uint64_t mode = (uint64_t)strtoull(argv[1], NULL, 16);
    uint16_t target_mode = (uint16_t)mode;
    uint16_t *video_modes = (uint16_t*)0x010E010D; // Assuming this is the address where video modes are stored
    for (int i = 0; video_modes[i] != 0xFFFF; i++) {
        printf("Video Mode %d: 0x%04X\n", i, video_modes[i]);
    }
    return 0;
}