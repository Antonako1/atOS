#include "./STRING.h"

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
BOOLEAN STRCMP(CONST U8* str1, CONST U8* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (*str1 == *str2);
}
BOOLEAN STRNCMP(CONST U8* str1, CONST U8* str2, U32 n) {
    while (n && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        n--;
    }
    return (n == 0) || (*str1 == *str2);
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
BOOLEAN STRCASECMP(U8 *a, U8 *b) {
    if (!a || !b) return -1;
    while (*a && *b) {
        CHAR ca = *a;
        CHAR cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return (int)(ca - cb);
        a++;
        b++;
    }
    return (int)(*a - *b);
}
BOOLEAN STRCASENCMP(U8 *a, U8 *b, U32 n) {
    if (!a || !b) return -1;
    while (*a && *b && n--) {
        CHAR ca = *a;
        CHAR cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return (int)(ca - cb);
        a++;
        b++;
    }
    return (int)(*a - *b);
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
