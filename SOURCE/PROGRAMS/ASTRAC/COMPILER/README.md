AC language

case-insensitive.
c-subset with extensions for low-level programming and hardware access.


## Technical Constraints & ABI
- **Word Size:** 32-bit (Native types are 32-bit wide).
- **Alignment:** 4-byte alignment for structs by default.
- **Calling Convention:** CDECL - arguments pushed right-to-left, caller cleans stack.
- **Preprocessor:** Shared with ASTRAC ASM; supports standard #directives.
- **Pointer Arithmetic:** Follows `sizeof(T)` scaling.
- **Case Insensitivity:** `IF`, `if`, `If` and `iF` are identical tokens. Same applies to keywords, identifiers, function and variable names.
- **One input file:** Due to binary constraints, only one source file is compiled at a time. Use `#include` for modularity.

## Preprocessor
- Supported directives: `#include`, `#define`, `#undef`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `#error`, `#warning`.
- Macro expansion: simple textual replacement. Preprocessor outputs temp files to `/TMP/` for the parser.

## Pointer & Memory Model
- Pointer size: 32-bit. `NULLPTR` is the null pointer constant.
- Pointer arithmetic scales by `sizeof(T)`. Casting between integer and pointer types is allowed for hardware access.
- Strict aliasing: none enforced; the implementation may allow type-punning for low-level access.

## ABI Details
- Calling convention: `CDECL` — arguments pushed right-to-left, caller cleans stack.
- Return values: scalar values returned in the native return register (32-bit: commonly `EAX`).
- Structure return: caller-allocated buffer passed by hidden pointer if needed.

## Language Limitations (Initial Implementation)
- No variable-length arrays, no complex C99+ features, and limited struct bitfields.
- `F32` may be treated as a reserved keyword initially; software float support can be added later.
- No variable type checking, only size-based and signature type compatibility for initial implementation. Meaning `U32` and `I32` are interchangeable at the type system level, but their sizes are respected for pointer arithmetic and memory layout. `VOIDPTR`, `U32`, `U32*` are all distinct types but have the same size and alignment, thus no compatibility issues for memory operations.
- All variables must be declared at the beginning of a block (no mixed declarations and code).
- No support for `goto` or computed jumps as is. Use ASM{} blocks for label creation and jumping.
- No typedef or function pointers in the initial implementation. These can be added later once the core parsing and code generation are stable.

## Parsing / Lexer Notes
- Case-insensitive tokenization: identifiers are normalized (e.g., to upper-case) during lexing.
- Integer literals: decimal, hex (`0x`), binary (`0b`); default integer size fits the smallest type that holds the value.
- Operator precedence follows standard C where practical; document specifics as parser is implemented.

## Implementation Notes
- Start with lexer → parser (recursive-descent) → semantic checks → codegen (emit assembly to `tmp_file`).
- Reuse shared `PREPROCESS.c` for preprocessing.

```
Keywords:
    Data types:
        F32
        U32/I32
        U16/I16
        U8/I8
        U0
        BOOL
        TRUE
        FALSE
        NULLPTR

        Pointer types:
            VOIDPTR
            P<type>
            PP<type>
            Meaning PU8 = U8*, PPU8 = U8**

    Others:
        break
        case
        continue
        default
        do
        for
        return
        switch
        while
        goto
        sizeof
        if
        else
        union
        enum
        struct

Symbols:
    Arithmetic:
        a + b
        a++, ++a
        +a
        
        a - b
        a--, --a
        -a

        a * b
        a / b
        a % b
    
    Relational:
        ==
        !=
        <
        >
        <=
        >=
    Logical:
        &&
        ||
        !
    Bitwise:
        &
        |
        ^
        ~
        <<
        >>
    Assignment:
        =, +=, -=, *=, /=, %=
        &=, |=, ^=, <<=, >>=
    Members and pointers:
        *a, &a
        a[n]
        a.b
        a->b
        a.*b
        a->*b

    Others:
        a()
        a, b
        a ? b : c
        sizeof(a)
        (type)a

    (, )
    {, }
    [, ]
    ;


    Function declarations:
        return_type function_name(parameter_list) {
            // function body
        }

    Control flow:
        if (condition) {
            // code
        } else if (condition) {
            // code
        } else {
            // code
        }

        switch (expression) {
            case 1...n:
                // code
                break;
            case value1:
                // code
                break;
            case value2:
                // code
                break;
            default:
                // code
        }

         for (initialization; condition; increment) {
             // code
         }

         while (condition) {
             // code
         }

         do {
             // code
         } while (condition);

    Constants:
        123
        0x1A
        0b1010
        3.14
        "Hello, World!"
        'A'
    
    Escape sequences:
        \n  - newline
        \t  - horizontal tab
        \\  - backslash
        \"  - double quote
        \'  - single quote

    Example program:
        U32 factorial(U32 n) {
            if (n == 0) {
                return 1;
            } else {
                return n * factorial(n - 1);
            }
        }

        U32 main(U32 argc, PPU8 argv) {
            U32 result = 0;
            U32 i = 0;
            result = factorial(5);
            if(result > 100) {
                result = 100;
            }
            for(i = 0; i < 10; i++) {
                result += i;
            }
            do {
                result--;
            } while(result > 0);

            return result;
        }
```