#ifndef ARGHAND_H
#define ARGHAND_H
#include <STD/TYPEDEF.h>

typedef struct {
    PU8 *names;      // Array of aliases
    U32  nameCount;  // Number of aliases
    PU8  value;      // Option value (NULL for flag)
    BOOLEAN present; // TRUE if any alias is present
} ARG, *PARG;

typedef struct {
    ARG *args;
    U32 count;
} ARGHANDLER, *PARGHANDLER;

VOID ARGHAND_INIT(PARGHANDLER ah, U32 argc, PPU8 argv, PPU8 *argAliases[], U32 aliasCounts[], U32 argCount);
BOOLEAN ARGHAND_IS_PRESENT(PARGHANDLER ah, PU8 name);
PU8 ARGHAND_GET_VALUE(PARGHANDLER ah, PU8 name);
VOID ARGHAND_FREE(PARGHANDLER ah);

#define ARGHAND_ARG(...) \
    { __VA_ARGS__ }

/* Macro to count aliases in ARGHAND_ARG */
#define ARGHAND_ALIAS_COUNT(arg) \
    (sizeof(arg)/sizeof(PU8))

/* Macro to combine multiple arguments into the allAliases array */
#define ARGHAND_ALL_ALIASES(...) \
    { __VA_ARGS__ }

/* Macro to automatically generate aliasCounts array from multiple arguments */
#define ARGHAND_ALL_ALIAS_COUNTS(...) \
    { __VA_ARGS__##_COUNT }

/* Example usage:

PU8 help[] = ARGHAND_ARG("-h", "--help");
PU8 name[] = ARGHAND_ARG("-n", "--name");

PPU8 allAliases[] = ARGHAND_ALL_ALIASES(help, name);
U32 aliasCounts[] = { ARGHAND_ALIAS_COUNT(help), ARGHAND_ALIAS_COUNT(name) };

ARGHANDLER ah;
ARGHAND_INIT(&ah, argc, argv, allAliases, aliasCounts, 2);
ARGHAND_FREE(&ah);

*/
#endif // ARGHAND_H
