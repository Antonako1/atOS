#include <LIBRARIES/ATHASH/BASE64.h>

void BASE64_ENCODE(PU8 input, U32 input_len, PU8 output) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const char padding_char = '=';
    U32 output_len = 4 * ((input_len + 2) / 3); // 4 chars for every 3 bytes
    U32 i, j;
    for (i = 0, j = 0; i < input_len;) {
        U32 octet_a = i < input_len ? input[i++] : 0;
        U32 octet_b = i < input_len ? input[i++] : 0;
        U32 octet_c = i < input_len ? input[i++] : 0;

        U32 triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = encoding_table[(triple >> 18) & 0x3F];
        output[j++] = encoding_table[(triple >> 12) & 0x3F];
        output[j++] = (i > input_len + 1) ? padding_char : encoding_table[(triple >> 6) & 0x3F];
        output[j++] = (i > input_len) ? padding_char : encoding_table[triple & 0x3F];
    }
    output[output_len] = '\0'; // Null-terminate the output string
}
U32 BASE64_DECODE(PU8 input, PU8 output) {
    /*
     * Full 128-entry table indexed by ASCII value.
     * 64 = invalid / padding placeholder.
     * Correct mapping:
     *   '+' (43) -> 62,  '/' (47) -> 63
     *   '0'-'9'  (48-57) -> 52-61
     *   'A'-'Z'  (65-90) ->  0-25
     *   'a'-'z'  (97-122)-> 26-51
     */
    static const U8 decoding_table[128] = {
        /*  0- 15 */ 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        /* 16- 31 */ 64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
        /* 32- 47 */ 64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
        /* 48- 63 */ 52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,
        /* 64- 79 */ 64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        /* 80- 95 */ 15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,
        /* 96-111 */ 64,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        /*112-127 */ 41,42,43,44,45,46,47,48,49,50,51,64,64,64,64,64
    };

    /* Count total length including '=' padding */
    U32 input_len = 0;
    while (input[input_len]) input_len++;

    if (input_len == 0 || (input_len & 3) != 0) {
        output[0] = '\0';
        return 0;
    }

    /* Determine number of padding chars */
    U32 padding = 0;
    if (input[input_len - 1] == '=') padding++;
    if (input[input_len - 2] == '=') padding++;

    U32 output_len = (input_len / 4) * 3 - padding;
    U32 i, j;

    for (i = 0, j = 0; i < input_len; i += 4) {
        U32 a = (input[i]   == '=') ? 0 : decoding_table[(U32)input[i]];
        U32 b = (input[i+1] == '=') ? 0 : decoding_table[(U32)input[i+1]];
        U32 c = (input[i+2] == '=') ? 0 : decoding_table[(U32)input[i+2]];
        U32 d = (input[i+3] == '=') ? 0 : decoding_table[(U32)input[i+3]];

        U32 triple = (a << 18) | (b << 12) | (c << 6) | d;

        if (j < output_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) output[j++] = (triple >>  8) & 0xFF;
        if (j < output_len) output[j++] =  triple        & 0xFF;
    }
    return output_len;
}
