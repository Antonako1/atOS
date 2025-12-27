#include <LIBRARIES/ARGHAND/ARGHAND.h>
#include <STD/STRING.h>
#include <STD/MEM.h>

VOID ARGHAND_INIT(PARGHANDLER ah, U32 argc, PPU8 argv, PPU8 *argAliases[], U32 aliasCounts[], U32 argCount) {
    ah->count = argCount;
    ah->args = (ARG *)MAlloc(sizeof(ARG) * argCount);

    for(U32 i = 0; i < argCount; ++i) {
        ah->args[i].names = argAliases[i];
        ah->args[i].nameCount = aliasCounts[i];
        ah->args[i].value = NULL;
        ah->args[i].present = FALSE;

        for(U32 j = 0; j < argc; ++j) {
            for(U32 k = 0; k < ah->args[i].nameCount; ++k) {
                if(STRCMP(argv[j], ah->args[i].names[k]) == 0) {
                    ah->args[i].present = TRUE;
                    if(j + 1 < argc && argv[j + 1][0] != '-') {
                        ah->args[i].value = argv[j + 1];
                    }
                    break;
                }
            }
            if(ah->args[i].present) break;
        }
    }
}

BOOLEAN ARGHAND_IS_PRESENT(PARGHANDLER ah, PU8 name) {
    for(U32 i = 0; i < ah->count; ++i) {
        for(U32 k = 0; k < ah->args[i].nameCount; ++k) {
            if(STRCMP(ah->args[i].names[k], name) == 0)
                return ah->args[i].present;
        }
    }
    return FALSE;
}

PU8 ARGHAND_GET_VALUE(PARGHANDLER ah, PU8 name) {
    for(U32 i = 0; i < ah->count; ++i) {
        for(U32 k = 0; k < ah->args[i].nameCount; ++k) {
            if(STRCMP(ah->args[i].names[k], name) == 0)
                return ah->args[i].value;
        }
    }
    return NULL;
}

VOID ARGHAND_FREE(PARGHANDLER ah) {
    if(ah->args) {
        MFree(ah->args);
        ah->args = NULL;
    }
    ah->count = 0;
}

