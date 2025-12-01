/*+++
    SOURCE/STD/STDINT.h - Minimum definitions for atOS

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef TYPEDEF_H
#define TYPEDEF_H

#define ATTRIB_DATA __attribute__((section(".data")))
#define ATTRIB_RODATA __attribute__((section(".rodata")))
#define ATTRIB_CODE __attribute__((section(".text")))
#define ATTRIB_PACKED __attribute__((packed))
#define ATTRIB_ALIGNED(x) __attribute__((aligned(x)))
#define ATTRIB_NOINLINE __attribute__((noinline))
#define ATTRIB_UNUSED __attribute__((unused))
#define ATTRIB_USED __attribute__((used))
#define ATTRIB_WEAK __attribute__((weak))
#define ATTRIB_DEPRECATED __attribute__((deprecated))
#define ATTRIB_COLD __attribute__((cold))
#define ATTRIB_HOT __attribute__((hot))
#define ATTRIB_NAKED __attribute__((naked))
#define ATTRIB_NORETURN __attribute__((noreturn))

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
#define NULLT     '\0'
#define VOID         void
#define VOIDPTR     void*
#define SET_NULL(x) (x = NULLPTR);
// Integer definitions
typedef VOID            U0;
typedef VOID            I0;
typedef VOID            S0;
typedef signed char     I8;
typedef unsigned char   U8;
typedef signed short    I16;
typedef unsigned short  U16;
typedef signed int      I32;
typedef unsigned int    U32;
typedef I8              S8;
typedef I16             S16;
typedef I32             S32;

// Floating-point definitions
// Warning: Floats are not supported as of now
typedef float         F32;

// 64-bit integer definition... not nice...
typedef struct {
    U32 Low;
    U32 High;
} 
__attribute__((packed))
U64;

// Boolean definitions
typedef U32          BOOL;
typedef BOOL         BOOLEAN;
#define TRUE         1
#define FALSE        0

#define CHAR         U8
#define UCHAR        unsigned char
#define CHARPTR      CHAR*
#define STRING       CHAR*
#define SHORT        I16
#define USHORT       U16
#define INT          I32
#define UINT         U32

typedef U8           BYTE;
typedef U16          WORD;
typedef U32          DWORD;

typedef U8*          PU8;
typedef U16*         PU16;
typedef U32*         PU32;

typedef PU8*         PPU8;
typedef PU16*        PPU16;
typedef PU32*        PPU32;

typedef U32          SIZE_T;
typedef U32          size_t;
typedef U32          PTR;
typedef U32          ADDR;

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

#endif // TYPEDEF_H