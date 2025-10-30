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

INT STRCMP(CONST U8* str1, CONST U8* str2);
INT STRNCMP(CONST U8* str1, CONST U8* str2, U32 n);
INT STRICMP(U8 *a, U8 *b);
INT STRNICMP(U8 *a, U8 *b, U32 n);

U0 *STRCHR(CONST U8* str, U8 c);
U0 *STRRCHR(CONST U8* str, U8 c);

BOOLEAN STREQ(U8* str1, U8* str2);
BOOLEAN STRNEQ(U8* str1, U8* str2, U32 n);

BOOLEAN ISALNUM(CHAR c);

PU8 STRDUP(PU8 src);
PU8 STRNDUP(PU8 src, U32 n);
PU8 STRDUPCAT(PU8 a, PU8 b); // Duplicates a string into a and adds b
PU8 STRAPPEND(PU8 dest, PU8 src); // HEAP. Appends src to dest
PU8 STRAPPEND_SEPARATOR(PU8 dest, PU8 src, CHAR separator); // Append strings with separator

VOID STRSHIFT(U8 *src, U32 index, I32 shiftcount);
VOID STRNSHIFT(U8 *src, U32 index, I32 shiftcount, U32 maxlen);
VOID STRSHIFTLEFTAT(PU8 src, U32 index);
VOID STRNSHIFTRIGHTAT(PU8 src, U32 index, CHAR ch, U32 maxlen);

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

/**
 * @brief Finds the first character in 'str' that matches
 * *any* character specified in 'accept'.
 * @param str    The string to scan.
 * @param accept The string containing characters to search for.
 * @return A pointer to the byte in 'str' that matches,
 * or NULLPTR if no such byte is found.
 */
PU8 STRPBRK(PU8 str, PU8 accept);

/**
 * @brief Breaks a string into a sequence of tokens.
 *
 * This function is stateful and not thread-safe.
 * It modifies the input string 'str' by writing null terminators.
 *
 * @param str    The string to tokenize on the first call.
 * Pass NULLPTR on subsequent calls to continue
 * tokenizing the same string.
 * @param delim  A string containing all delimiter characters.
 * @return A pointer to the beginning of the next token,
 * or NULLPTR if no more tokens are found.
 */
PU8 STRTOK(PU8 str, PU8 delim);
PU8 STRTOK_R(PU8 str, PU8 delim, PU8 *saveptr);

/**
 * @brief Finds the length of the initial segment of 'str'
 * that consists *only* of characters from 'accept'.
 * @param str    The string to scan.
 * @param accept The string containing acceptable characters.
 * @return The number of bytes in the initial segment of 'str'
 * which are in 'accept'.
 */
size_t STRSPN(PU8 str, PU8 accept);

U8* str_ltrim(U8 *s);
U8* str_rtrim(U8 *s);
U8* str_trim(U8 *s);
#endif // STRING_H