#include "./STRING.h"
#include "./MEM.h"
#include <STD/ARG.h>
U32 STRLEN(CONST U8* str) {
    U32 res = 0;
    while (*str++) res++;
    return res;
}
U32 STRNLEN(CONST U8* str, U32 maxlen) {
    U32 res = 0;
    while (res < maxlen && *str++) res++;
    return res;
}
U0 *STRCPY(U8* dest, CONST U8* src) {
    U0 *start = dest;
    while ((*dest++ = *src++));
    return start;
}
U0 *STRNCPY(U8* dest, CONST U8* src, U32 maxlen) {
    U0 *start = dest;
    while (maxlen && (*dest++ = *src++)) maxlen--;
    if (maxlen) *dest = 0;
    return start;
}
U0 *STRCAT(U8* dest, CONST U8* src) {
    U0 *start = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return start;
}
U0 *STRNCAT(U8* dest, CONST U8* src, U32 maxlen) {
    U0 *start = dest;
    while (*dest) dest++;
    while (maxlen && (*dest++ = *src++)) maxlen--;
    if (maxlen) *dest = 0;
    return start;
}
U8* STRNCONCAT(U8 *dest, U32 dest_pos, U8 *src, U32 max_len) {
    if (!dest || !src || dest_pos >= max_len) return dest;

    U32 i = dest_pos;
    U32 j = 0;

    while (i < max_len && src[j] != '\0') {
        dest[i++] = src[j++];
    }

    dest[i] = '\0';

    while(i < max_len) {
        dest[i++] = '\0';
    }

    return dest;
}
U32 FIRST_INDEX_OF(CONST U8* str, U8 c) {
    U32 index = 0;
    while (*str) {
        if (*str == c) return index;
        str++;
        index++;
    }
    return (U32)-1; // not found
}
INT STRCMP(const U8* str1, const U8* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (INT)(*str1) - (INT)(*str2);
}

INT STRNCMP(const U8* str1, const U8* str2, U32 n) {
    while (n && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }

    if (n == 0) return 0;
    return (INT)(*str1) - (INT)(*str2);
}

U0 *STRCHR(CONST U8* str, U8 c) {
    while (*str && (*str != c)) str++;
    return (*str == c) ? (U0 *)str : NULL;
}
U0 *STRRCHR(CONST U8* str, U8 c) {
    U0 *last = NULL;
    while (*str) {
        if (*str == c) last = (U0 *)str;
        str++;
    }
    return last;
}

INT STRICMP(U8 *a, U8 *b) {
    if (!a || !b) return -1;

    while (*a && *b) {
        CHAR ca = *a;
        CHAR cb = *b;

        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;

        if (ca != cb)
            return (INT)(ca - cb);

        a++;
        b++;
    }

    return (INT)(*a - *b);
}

INT STRNICMP(U8 *a, U8 *b, U32 n) {
    if (!a || !b) return -1;
    if (n == 0) return 0;

    while (n-- && *a && *b) {
        CHAR ca = *a;
        CHAR cb = *b;

        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;

        if (ca != cb)
            return (INT)(ca - cb);

        a++;
        b++;

        // if we consumed n characters, stop (avoid reading past)
        if (n == 0)
            break;
    }

    return (INT)(*a - *b);
}


U32 ATOI(CONST U8* str) {
    U32 res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}
U32 ATOI_HEX(CONST U8* str) {
    U32 res = 0;
    while ((*str >= '0' && *str <= '9') || (*str >= 'A' && *str <= 'F') || (*str >= 'a' && *str <= 'f')) {
        res *= 16;
        if (*str >= '0' && *str <= '9') {
            res += (*str - '0');
        } else if (*str >= 'A' && *str <= 'F') {
            res += (*str - 'A' + 10);
        } else if (*str >= 'a' && *str <= 'f') {
            res += (*str - 'a' + 10);
        }
        str++;
    }
    return res;
}
U32 ATOI_BIN(CONST U8* str) {
    U32 res = 0;
    while ((*str == '0' || *str == '1')) {
        res = (res << 1) | (*str - '0');
        str++;
    }
    return res;
}
U0 *ITOA(S32 value, I8* buffer, U32 base) {
    if(value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }
    switch (base) {
        case 2:
            // Convert integer to binary string
            {
                U32 i = 0;
                for (i = 0; i < 32; i++) {
                    buffer[31 - i] = (value & (1 << i)) ? '1' : '0';
                }
                buffer[32] = '\0';
            }
            break;
        case 10:
            // Convert integer to decimal string
            {
                U32 i = 0;
                U32 isNegative = 0;
                if (value < 0) {
                    isNegative = 1;
                    value = -value;
                }
                do {
                    buffer[i++] = (value % 10) + '0';
                    value /= 10;
                } while (value);
                if (isNegative) buffer[i++] = '-';
                buffer[i] = '\0';
                // Reverse the string
                for (U32 j = 0; j < i / 2; j++) {
                    U8 temp = buffer[j];
                    buffer[j] = buffer[i - j - 1];
                    buffer[i - j - 1] = temp;
                }
            }
            break;
        case 16:
            // Convert integer to hexadecimal string
            {
                U32 i = 0;
                if (value == 0) {
                    buffer[i++] = '0';
                } else {
                    while (value) {
                        U32 digit = value % 16;
                        buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
                        value /= 16;
                    }
                }

                // Ensure 2 digits for a byte
                if (i == 1) {
                    buffer[i++] = '0';
                }

                buffer[i] = '\0';

                // Reverse the string
                for (U32 j = 0; j < i / 2; j++) {
                    U8 temp = buffer[j];
                    buffer[j] = buffer[i - j - 1];
                    buffer[i - j - 1] = temp;
                }
            }

            break;
        default:
            // Unsupported base
            return NULL;
    }
    return buffer;
}

U0 *ITOA_U(U32 value, U8* buffer, U32 base) {
    if(value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return buffer;
    }
    switch (base) {
        case 2:
            // Convert integer to binary string
            {
                U32 i = 0;
                for (i = 0; i < 32; i++) {
                    buffer[31 - i] = (value & (1 << i)) ? '1' : '0';
                }
                buffer[32] = '\0';
            }
            break;
        case 10:
            // Convert integer to decimal string
            {
                U32 i = 0;
                do {
                    buffer[i++] = (value % 10) + '0';
                    value /= 10;
                } while (value);
                buffer[i] = '\0';
                // Reverse the string
                for (U32 j = 0; j < i / 2; j++) {
                    U8 temp = buffer[j];
                    buffer[j] = buffer[i - j - 1];
                    buffer[i - j - 1] = temp;
                }
            }
            break;
        case 16:
            // Convert integer to hexadecimal string
            {
                U32 i = 0;
                while (value) {
                    U32 digit = value % 16;
                    buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
                    value /= 16;
                }

                // Ensure 2 digits for a byte
                if (i == 1) {
                    buffer[i++] = '0';
                }

                buffer[i] = '\0';
                // Reverse the string
                for (U32 j = 0; j < i / 2; j++) {
                    U8 temp = buffer[j];
                    buffer[j] = buffer[i - j - 1];
                    buffer[i - j - 1] = temp;
                }
            }
            break;
        default:
            // Unsupported base
            return NULL;
    }
    return buffer;
}

U8 TOUPPER(U8 c) {
    if(c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

U8 TOLOWER(U8 c) {
    if(c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

U0 STR_TOUPPER(U8* str) {
    while(*str) {
        *str = TOUPPER(*str);
        str++;
    }
}

U0 STR_TOLOWER(U8* str) {
    while(*str) {
        *str = TOLOWER(*str);
        str++;
    }
}

static U32 is_whitespace(U8 c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// Trim leading whitespace in-place
U8* str_ltrim(U8 *s) {
    if (!s) return NULL;

    U8 *p = s;
    while (*p && is_whitespace(*p)) p++; // skip leading spaces

    // Shift string left if needed
    if (p != s) {
        U8 *dst = s;
        while (*p) *dst++ = *p++;
        *dst = '\0';
    }

    return s;
}

// Trim trailing whitespace in-place
U8* str_rtrim(U8 *s) {
    if (!s) return NULL;

    U8 *end = s;
    while (*end) end++;       // move to null terminator
    if (end == s) return s;   // empty string

    end--; // last character
    while (end >= s && is_whitespace(*end)) end--;
    *(end + 1) = '\0';

    return s;
}

// Trim both leading and trailing whitespace in-place
U8* str_trim(U8 *s) {
    if (!s) return NULL;
    str_rtrim(s);
    str_ltrim(s);
    return s;
}

BOOLEAN STREQ( U8* str1,  U8* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1 == *str2);
}

BOOLEAN STRNEQ( U8* str1,  U8* str2, U32 n) {
    while (n && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    if (n == 0) return TRUE;
    return (*str1 == *str2);
}



size_t STRSPN(PU8 str, PU8 accept) {
    size_t len = 0;
    PU8 p_accept_start = accept;

    if (!str || !accept) return 0;

    while (str[len] != '\0') {
        PU8 p_accept = p_accept_start;
        int found = 0;
        while (*p_accept != '\0') {
            if (str[len] == *p_accept) {
                found = 1;
                break;
            }
            p_accept++;
        }
        if (!found) {
            return len; // Character not in accept set
        }
        len++;
    }
    return len; // Reached end of string
}


PU8 STRPBRK(PU8 str, PU8 accept) {
    PU8 p_str_start = str;
    PU8 p_accept_start = accept;

    if (!str || !accept) return NULLPTR;

    while (*p_str_start != '\0') {
        PU8 p_accept = p_accept_start;
        while (*p_accept != '\0') {
            if (*p_str_start == *p_accept) {
                return (PU8)p_str_start; // Found a match
            }
            p_accept++;
        }
        p_str_start++;
    }
    return NULLPTR; // No match
}



PU8 STRTOK(PU8 str, PU8 delim) {
    // static pointer to store our position
    static PU8 last_token_end = NULLPTR;
    
    PU8 token_start;

    // 1. Check if we're starting a new string
    if (str != NULLPTR) {
        // Yes, so save the start of this new string
        token_start = str;
    } else {
        // No, so check if we have a saved position
        if (last_token_end == NULLPTR) {
            // We don't, and str is NULL, so no tokens left.
            return NULLPTR;
        }
        // Continue from where we left off
        token_start = last_token_end;
    }

    // 2. Skip any *leading* delimiters
    // strspn = "string span"
    // It returns the length of the part of token_start
    // that consists *only* of delimiter characters.
    token_start += STRSPN(token_start, delim);

    // 3. Check if we've reached the end
    if (*token_start == '\0') {
        // We skipped delimiters and hit the end. No token.
        last_token_end = NULLPTR; // Reset for next new string
        return NULLPTR;
    }

    // 4. Find the *end* of the current token
    // strpbrk = "string pointer break"
    // It finds the *first* occurrence of *any* delimiter
    // in the rest of the string.
    PU8 token_end = STRPBRK(token_start, delim);

    if (token_end == NULLPTR) {
        // No delimiter found. This is the last token.
        // Find the null terminator manually
        last_token_end = token_start;
        while (*last_token_end != '\0') {
            last_token_end++;
        }
        // Set our static pointer to NULL so the *next* call returns NULL
        last_token_end = NULLPTR;
        return token_start;
    } else {
        // 5. Found a delimiter. Terminate the token.
        // This is the "destructive" part.
        *token_end = '\0';

        // 6. Save our position for the next call
        // We'll start *after* the null terminator we just wrote
        last_token_end = token_end + 1;
        
        return token_start;
    }
}

PU8 STRDUP(PU8 src) {
    if (!src) return NULLPTR;
    U32 len = STRLEN(src);
    PU8 dup = MAlloc(len + 1);
    if (!dup) return NULLPTR;
    STRCPY(dup, src);
    return dup;
}

PU8 STRNDUP(PU8 src, U32 n) {
    if (!src) return NULLPTR;
    U32 len = STRLEN(src);
    if (len > n) len = n;
    PU8 dup = MAlloc(len + 1);
    if (!dup) return NULLPTR;
    STRNCPY(dup, src, len);
    dup[len] = '\0';
    return dup;
}

BOOLEAN ISALNUM(CHAR c) {
    // Check if character is 0-9, a-z, or A-Z
    if ((c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z')) {
        return TRUE;
    }
    return FALSE;
}
PU8 STRDUPCAT(PU8 a, PU8 b) {
    if (!a && !b) return NULLPTR;
    if (!a) return STRDUP(b);
    if (!b) return STRDUP(a);

    U32 len_a = STRLEN(a);
    U32 len_b = STRLEN(b);
    PU8 result = MAlloc(len_a + len_b + 1);
    if (!result) return NULLPTR;

    MEMCPY(result, a, len_a);
    MEMCPY(result + len_a, b, len_b);
    result[len_a + len_b] = '\0';
    return result;
}
PU8 STRAPPEND(PU8 dest, PU8 src) {
    if (!src || !*src) return dest; // nothing to append

    U32 src_len = STRLEN(src);
    if (!dest) {
        // if dest is null, just duplicate
        PU8 new_str = MAlloc(src_len + 1);
        if (!new_str) return NULLPTR;
        MEMCPY(new_str, src, src_len);
        new_str[src_len] = '\0';
        return new_str;
    }

    U32 dest_len = STRLEN(dest);
    PU8 new_str = ReAlloc(dest, dest_len + src_len + 1);
    if (!new_str) return dest; // fallback, keep old one if realloc fails

    MEMCPY(new_str + dest_len, src, src_len);
    new_str[dest_len + src_len] = '\0';

    return new_str;
}

PU8 STRAPPEND_SEPARATOR(PU8 dest, PU8 src, CHAR separator) {
    if (!src || !*src) return dest; // nothing to append

    U32 src_len = STRLEN(src);

    if (!dest) {
        // if dest is null, just duplicate
        PU8 new_str = MAlloc(src_len + 1);
        if (!new_str) return NULLPTR;
        MEMCPY(new_str, src, src_len);
        new_str[src_len] = '\0';
        return new_str;
    }

    U32 dest_len = STRLEN(dest);
    // +1 for separator, +1 for null terminator
    PU8 new_str = ReAlloc(dest, dest_len + 1 + src_len + 1);
    if (!new_str) return dest; // fallback, keep old one if realloc fails

    new_str[dest_len] = separator;                  // add separator after dest
    MEMCPY(new_str + dest_len + 1, src, src_len);  // copy src after separator
    new_str[dest_len + 1 + src_len] = '\0';        // null terminate

    return new_str;
}

VOID STRSHIFT(U8 *src, U32 index, I32 shiftcount) {
    if (!src) return;
    U32 len = STRLEN(src);

    // clamp index
    if (index > len) index = len;

    // shifting right
    if (shiftcount > 0) {
        // move the tail to the right
        for (I32 i = len; i >= (I32)index; i--) {
            src[i + shiftcount] = src[i];
        }
    }
    // shifting left
    else if (shiftcount < 0) {
        I32 shift = -shiftcount;
        if (index + shift > len) shift = len - index;
        for (U32 i = index; i <= len; i++) {
            src[i] = src[i + shift];
        }
    }
}
VOID STRNSHIFT(U8 *src, U32 index, I32 shiftcount, U32 maxlen) {
    if (!src) return;
    U32 len = STRLEN(src);
    if (index > len) index = len;

    // shifting right
    if (shiftcount > 0) {
        if (len + shiftcount >= maxlen) shiftcount = maxlen - len - 1;
        for (I32 i = len; i >= (I32)index; i--) {
            if ((U32)(i + shiftcount) < maxlen)
                src[i + shiftcount] = src[i];
        }
    }
    // shifting left
    else if (shiftcount < 0) {
        I32 shift = -shiftcount;
        if (index + shift > len) shift = len - index;
        for (U32 i = index; i <= len; i++) {
            src[i] = src[i + shift];
        }
    }
}
VOID STRSHIFTLEFTAT(PU8 src, U32 index) {
    if (!src) return;

    U32 len = STRLEN(src);
    if (index >= len) return; // out of range

    for (U32 i = index; i < len; i++) {
        src[i] = src[i + 1];
    }
}
VOID STRNSHIFTRIGHTAT(PU8 src, U32 index, CHAR ch, U32 maxlen) {
    if (!src) return;
    U32 len = STRLEN(src);
    if (len + 1 >= maxlen) return; // no room
    if (index > len) index = len;

    for (I32 i = len; i >= (I32)index; i--) {
        src[i + 1] = src[i];
    }
    src[index] = ch;
}
PU8 STRTOK_R(PU8 str, PU8 delim, PU8 *saveptr) {
    PU8 token_start;
    if (str != NULL) token_start = str;
    else if (*saveptr != NULL) token_start = *saveptr;
    else return NULLPTR;

    // Skip leading delimiters
    token_start += STRSPN(token_start, delim);
    if (*token_start == '\0') {
        *saveptr = NULLPTR;
        return NULLPTR;
    }

    // Find end of token
    PU8 token_end = STRPBRK(token_start, delim);
    if (!token_end) {
        *saveptr = NULLPTR;
        return token_start;
    }

    *token_end = '\0';
    *saveptr = token_end + 1;
    return token_start;
}
BOOL IS_DIGIT(CHAR c) {
    return (c >= '0' && c <= '9');
}

BOOL IS_DIGIT_STR(PU8 str) {
    if (!str || !*str) return FALSE; // empty string is not a number
    for (; *str; str++) {
        if (!IS_DIGIT(*str))
            return FALSE;
    }
    return TRUE;
}
PU8 STRSTR(PU8 a, PU8 b) {
    if(!a || !b) return NULLPTR;
    U32 len = STRLEN(b);
    if(len == 0) return a;
    PU8 tmp = a;
    while(*tmp) {
        if(STRNCMP(tmp, b, len) == 0) return tmp;
        tmp++;
    }

    return NULLPTR;
}

PU8 STRISTR(PU8 a, PU8 b) {
    if(!a || !b) return NULLPTR;
    U32 len = STRLEN(b);
    if(len == 0) return a;
    PU8 tmp = a;
    while(*tmp) {
        if(STRNICMP(tmp, b, len) == 0) return tmp;
        tmp++;
    }
    return NULLPTR;
}


VOID VFORMAT(VOID (*putch)(CHAR, VOID*), VOID *ctx, CHAR *fmt, va_list args) {
    while (*fmt) {
        if (*fmt != '%') {
            putch(*fmt++, ctx);
            continue;
        }

        fmt++; // skip '%'
        if (!*fmt) break;

        I32 width = 0;
        BOOL pad_zero = FALSE;

        // Parse optional zero-padding and width
        if (*fmt == '0') {
            pad_zero = TRUE;
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        switch (*fmt) {
            case 'c': {
                CHAR c = (CHAR)va_arg(args, I32);
                putch(c, ctx);
                break;
            }

            case 's': {
                CHAR *s = va_arg(args, CHAR*);
                if (!s) s = "(null)";
                while (*s) putch(*s++, ctx);
                break;
            }

            case 'd': {
                I32 val = va_arg(args, I32);
                CHAR buf[32];
                ITOA(val, buf, 10);

                I32 len = STRLEN(buf);
                for (I32 i = len; i < width; i++)
                    putch(pad_zero ? '0' : ' ', ctx);
                for (CHAR *p = buf; *p; p++)
                    putch(*p, ctx);
                break;
            }

            case 'u': {
                U32 val = va_arg(args, U32);
                CHAR buf[32];
                ITOA_U(val, buf, 10);

                I32 len = STRLEN(buf);
                for (I32 i = len; i < width; i++)
                    putch(pad_zero ? '0' : ' ', ctx);
                for (CHAR *p = buf; *p; p++)
                    putch(*p, ctx);
                break;
            }

            case 'x':
            case 'X': {
                U32 val = va_arg(args, U32);
                CHAR buf[32];
                ITOA_U(val, buf, 16);

                I32 len = STRLEN(buf);
                for (I32 i = len; i < width; i++)
                    putch(pad_zero ? '0' : ' ', ctx);
                for (CHAR *p = buf; *p; p++)
                    putch(*p, ctx);
                break;
            }

            case '%':
                putch('%', ctx);
                break;

            default:
                putch('%', ctx);
                putch(*fmt, ctx);
                break;
        }

        fmt++;
    }
}

/* ---------------------------------- */
/* Internal putch handlers for output */
/* ---------------------------------- */

// For SPRINTF
VOID buffer_putch(CHAR c, VOID *ctx) {
    CHAR **buf = (CHAR **)ctx;
    *(*buf)++ = c;
}




VOID SPRINTF(CHAR *buffer, CHAR *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    CHAR *out = buffer;
    VFORMAT(buffer_putch, &out, fmt, args);
    *out = '\0'; // null-terminate
    va_end(args);
}

PU8 STR_REPLACE_FIRST(PU8 src, PU8 repl, PU8 with) {
    if (!src || !repl) return NULLPTR;
    if (!with) with = "";

    U32 src_len  = STRLEN(src);
    U32 repl_len = STRLEN(repl);
    U32 with_len = STRLEN(with);

    if (src_len == 0 || repl_len == 0) return NULLPTR;

    PU8 pos = STRSTR(src, repl);
    if (!pos) return STRDUP(src); // nothing to replace, return copy

    U32 new_len = src_len - repl_len + with_len;
    PU8 res = MAlloc(new_len + 1); // +1 for null terminator
    if (!res) return NULLPTR;

    U32 index = 0;

    // Copy characters before match
    for (PU8 p = src; p != pos; p++)
        res[index++] = *p;

    // Copy replacement string
    for (U32 i = 0; i < with_len; i++)
        res[index++] = with[i];

    // Copy characters after match
    for (PU8 p = pos + repl_len; *p; p++)
        res[index++] = *p;

    res[index] = '\0';
    return res;
}

PU8 STR_REPLACE(PU8 src, PU8 repl, PU8 with) {
    if (!src || !repl) return NULLPTR;
    if (!with) with = "";

    PU8 result = STRDUP(src); // start with a copy
    if (!result) return NULLPTR;

    PU8 pos;
    while ((pos = STRSTR(result, repl)) != NULLPTR) {
        PU8 tmp = STR_REPLACE_FIRST(result, repl, with);
        MFree(result);
        result = tmp;
        if (!result) return NULLPTR;
    }

    return result;
}
