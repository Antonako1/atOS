/*+++
    SOURCE/STD/ARG.h - VA_LIST support

    Part of atOS

    Licensed under the MIT License. See LICENSE file in the project root for full license information.
---*/
#ifndef STDARG_H
#define STDARG_H

// 'va_list' is a pointer type used to access variable arguments
typedef unsigned char *va_list;

// Align 't' to the next multiple of 'sizeof(int)'
#define _VA_ALIGN(t)    (((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

// Initialize 'ap' to point to the first variable argument
#define va_start(ap, last_arg)  (ap = (va_list)&(last_arg) + _VA_ALIGN(last_arg))

// Get the next argument value of type 't'
#define va_arg(ap, t)  (*(t *)((ap += _VA_ALIGN(t)) - _VA_ALIGN(t)))

// End traversal of arguments (nothing to do here)
#define va_end(ap)     (ap = (va_list)0)

#endif // STDARG_H
