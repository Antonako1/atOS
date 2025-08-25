/*+++
    SOURCE/STD/STDINT.h - Minimum definitions for atOS-RT

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef ATOSMINDEF_H
#define ATOSMINDEF_H

// Keywords
#define STATIC       static
#define EXTERN       extern
#define AUTO         auto
#define REGISTER     register
#define TYPEDEF      typedef
#define STRUCT       struct
#define UNION        union
#define ENUM         enum
#define INLINE       inline
#define ASM          __asm__
#define RETURN       return

#define CONST        const
#define VOLATILE    volatile
#define INLINE       inline
#define RESTRICT    restrict
#define NULL         ((void*)0)
#define NULLPTR      ((void*)0)
#define VOID         void
#define VOIDPTR     void*
// Integer definitions
typedef VOID         U0;
typedef signed char   I8;
typedef unsigned char U8;
typedef signed short  I16;
typedef unsigned short U16;
typedef signed int    I32;
typedef unsigned int  U32;
typedef I8            S8;
typedef I16           S16;
typedef I32           S32;

// Floating-point definitions
typedef float         F32;

// Boolean definitions
#define BOOL         U32
#define BOOLEAN      BOOL
#define TRUE         1
#define FALSE        0

#define CHAR         U8
#define UCHAR        unsigned char
#define SHORT        I16
#define USHORT       U16
#define INT          I32
#define UINT         U32
typedef U32          UINTPTR;

typedef I8           BYTE;
typedef U8           UBYTE;
typedef I16          WORD;
typedef U16          UWORD;
typedef I32          DWORD;
typedef U32          UDWORD;

typedef U8*          PU8;
typedef U16*         PU16;
typedef U32*         PU32;


// Min - Max value definitions
#define MIN(a, b)    ((a) < (b) ? (a) : (b))
#define MAX(a, b)    ((a) > (b) ? (a) : (b))
#define I8_MIN    (-128)
#define I8_MAX    (127)
#define U8_MIN    (0)
#define U8_MAX    (255)
#define I16_MIN   (-32768)
#define I16_MAX   (32767)
#define U16_MIN   (0)
#define U16_MAX   (65535)
#define I32_MIN   (-2147483648)
#define I32_MAX   (2147483647)
#define U32_MIN   (0)
#define U32_MAX   (4294967295U)
#define F32_MIN   (-3.402823466e+38F)
#define F32_MAX   (3.402823466e+38F)


#endif // ATOSMINDEF_H