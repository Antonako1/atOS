Screen driver utilises VESA/VBE screen buffer, allowing 1024x768 resolution with 32 colours


 - VESA.h
    - Header file for VESA definitions. If you are not solely working on VBE, do not include this. Include VBE.h instead
 - VBE.h
    - Master header file for the VBE video driver. Contains low level functions to modify screen buffer
 - VBE.h
    - Header file for VBE. Contains only the bare bones functions needed to draw on screen.
 - VBE.c
    - Source code for VBE functions
