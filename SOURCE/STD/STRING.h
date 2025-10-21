#ifndef STRING_H
#define STRING_H

#include <STD/TYPEDEF.h>

U32 STRLEN(CONST U8* str);
U32 STRNLEN(CONST U8* str, U32 maxlen);
U0 *STRCPY(U8* dest, CONST U8* src);
U0 *STRNCPY(U8* dest, CONST U8* src, U32 maxlen);
U0 *STRCAT(U8* dest, CONST U8* src);
U0 *STRNCAT(U8* dest, CONST U8* src, U32 maxlen);
U8* STRNCONCAT(U8 *dest, U32 dest_pos, U8 *src, U32 max_len);

// these return TRUE if equal
BOOLEAN STRCMP(CONST U8* str1, CONST U8* str2);
BOOLEAN STRNCMP(CONST U8* str1, CONST U8* str2, U32 n);

// returns 0 on success
BOOLEAN STRCASECMP(U8 *a, U8 *b);
BOOLEAN STRCASENCMP(U8 *a, U8 *b, U32 n);

U0 *STRCHR(CONST U8* str, U8 c);
U0 *STRRCHR(CONST U8* str, U8 c);

U32 ATOI(CONST U8* str);
U32 ATOI_HEX(CONST U8* str);
U32 ATOI_BIN(CONST U8* str);
U0 *ITOA(S32 value, I8* buffer, U32 base);
U0 *ITOA_U(U32 value, U8* buffer, U32 base);
U8 TOUPPER(U8 c);
U8 TOLOWER(U8 c);
U0 STR_TOUPPER(U8* str);
U0 STR_TOLOWER(U8* str);
U32 FIRST_INDEX_OF(CONST U8* str, U8 c);
#endif // STRING_H