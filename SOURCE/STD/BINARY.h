/*+++
    SOURCE/STD/BINARY.h - Some ease-of-life binary macros

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef BINARY_H
#define BINARY_H

#define FLAG_SET(x, flag) x |= (flag)
#define FLAG_UNSET(x, flag) x &= ~(flag)
#define IS_FLAG_SET(x, flag) ((x & (flag)) != 0)
#define IS_FLAG_UNSET(x, flag) ((x & (flag)) == 0)

#define SHR(x, n) (x >> n)
#define SHL(x, n) (x << n)

#define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))

#endif // BINARY_H