#ifndef ATGL_DEFS_H
#define ATGL_DEFS_H

#include <STD/TYPEDEF.h>

typedef I32 WINHNDL;

/// @brief Local dimensions
typedef struct _ATGL_RECT {
    I32 width;
    I32 height;
    I32 x;
    I32 y;
} ATGL_RECT, *PATGL_RECT;

typedef struct _ATGL_2D_VECTOR {
    U32 x;
    U32 y;
} ATGL_2D_VECTOR, *PATGL_2D_VECTOR;



#endif // ATGL_DEFS_H