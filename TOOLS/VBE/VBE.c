
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t U8;
typedef uint32_t U32;
#define VBE_MAX_CHARS 255
#define VBE_FONT_HEIGHT_8x8 8
#define VBE_FONT_WIDTH_8x8 8

#define EMPTY_CHAR \
    (U8)0b11111000, \
    (U8)0b11111111, \
    (U8)0b00000111, \
    (U8)0b00001100, \
    (U8)0b00011000, \
    (U8)0b00111111, \
    (U8)0b11111111, \
    (U8)0b00000000
#define CHARACTERIZE(x) (U8)x

U8 VBE_LETTERS_8x8[VBE_MAX_CHARS][VBE_FONT_HEIGHT_8x8] = {
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 0-3
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 4-7
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 8-11
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 12-15
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 16-19
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 20-23
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 24-27
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, // 28-31
    {EMPTY_CHAR}, // Spacebar               (   ), 32
    {EMPTY_CHAR}, // Exclamation mark       ( ! ), 33
    {EMPTY_CHAR}, // Quotation mark         ( " ), 34
    {EMPTY_CHAR}, // Number sign            ( # ), 35
    {EMPTY_CHAR}, // Dollar sign            ( $ ), 36
    {EMPTY_CHAR}, // Percent sign           ( % ), 37
    {EMPTY_CHAR}, // Ampersand              ( & ), 38
    {EMPTY_CHAR}, // Apostrophe             ( ' ), 39
    {EMPTY_CHAR}, // Left parenthesis       ( ( ), 40
    {EMPTY_CHAR}, // Right parenthesis      ( ) ), 41
    {EMPTY_CHAR}, // Asterisk               ( * ), 42
    {EMPTY_CHAR}, // Plus                   ( + ), 43
    {EMPTY_CHAR}, // Comma                  ( , ), 44
    {EMPTY_CHAR}, // Hyphen                 ( - ), 45
    {EMPTY_CHAR}, // Period                 ( . ), 46
    {EMPTY_CHAR}, // Slash                  ( / ), 47
    {EMPTY_CHAR}, // Zero                   ( 0 ), 48
    {EMPTY_CHAR}, // One                    ( 1 ), 49
    {EMPTY_CHAR}, // Two                    ( 2 ), 50
    {EMPTY_CHAR}, // Three                  ( 3 ), 51
    {EMPTY_CHAR}, // Four                   ( 4 ), 52
    {EMPTY_CHAR}, // Five                   ( 5 ), 53
    {EMPTY_CHAR}, // Six                    ( 6 ), 54
    {EMPTY_CHAR}, // Seven                  ( 7 ), 55
    {EMPTY_CHAR}, // Eight                  ( 8 ), 56
    {EMPTY_CHAR}, // Nine                   ( 9 ), 57
    {EMPTY_CHAR}, // Colon                  ( : ), 58
    {EMPTY_CHAR}, // Semicolon              ( ; ), 59
    {EMPTY_CHAR}, // Less than              ( < ), 60
    {EMPTY_CHAR}, // Equal sign             ( = ), 61
    {EMPTY_CHAR}, // Greater than           ( > ), 62
    {EMPTY_CHAR}, // Question mark          ( ? ), 63
    {EMPTY_CHAR}, // At sign                ( @ ), 64
    {
    (U8)0b00111100,
    (U8)0b01100110,
    (U8)0b11000011,
    (U8)0b11000011,
    (U8)0b11111111,
    (U8)0b11000011,
    (U8)0b11000011,
    (U8)0b00000000,
    }, // A                       ( A ), 65
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
    {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR}, {EMPTY_CHAR},
};

void print_letter(uint8_t c, uint8_t fg, uint8_t bg) {
    for (uint8_t row = 0; row < VBE_FONT_HEIGHT_8x8; row++) {
        uint8_t row_data = VBE_LETTERS_8x8[c][row];
        for (uint8_t col = 0; col < VBE_FONT_WIDTH_8x8; col++) {
            uint8_t colour = bg;
            if ((row_data & (1 << (7 - col))) != 0) colour = fg;
            printf("%c", colour);
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    for (int i = 64, j = 0; j < 2; i++, j++) {
        print_letter(i, 'X', ' ');
    }
    return 0;
}