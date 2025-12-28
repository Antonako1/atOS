#ifndef ATGL_WINDOW_H
#define ATGL_WINDOW_H

#include <STD/TYPEDEF.h>
#include <LIBRARIES/ATGL/ATGL_INPUT.h>
#include <LIBRARIES/ATGL/ATGL_GRAPHICS.h>

typedef struct _ATGL_WINDOW {
    ATGL_INPUT_DEVICE input_device;
    
} ATGL_WINDOW, *PATGL_WINDOW;

#endif // ATGL_WINDOW_H