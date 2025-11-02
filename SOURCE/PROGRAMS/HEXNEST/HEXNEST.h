#ifndef HEXNEST_H
#define HEXNEST_H

#include <STD/TYPEDEF.h>

typedef struct _HEXNEST_ARGS {
    PU8 file;
    BOOL display_editor; // -e
} HEXNEST_ARGS, *PHEXN;

U32 EDITOR(PU8 file);
U32 HEXDUMP(PU8 file);

#endif // HEXNEST_H