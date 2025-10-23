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