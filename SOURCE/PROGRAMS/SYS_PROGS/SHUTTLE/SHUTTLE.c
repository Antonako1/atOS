#include <ARGHAND/ARGHAND.h>
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/MEM.h>
#include <STD/COMPORT.h>
#include <STD/FS_DISK.h>

#define FAIL(MSG) do { \
    printf(MSG "\n"); \
    goto cleanup; \
} while(0)

U32 main(U32 argc, PPU8 argv)
{
    PU8 help[]    = ARGHAND_ARG("-h", "--help");
    PU8 name[]    = ARGHAND_ARG("-p", "--port");
    PU8 send[]    = ARGHAND_ARG("-s", "--send");
    PU8 receive[] = ARGHAND_ARG("-r", "--receive");
    PU8 file[]    = ARGHAND_ARG("-f", "--file");
    PU8 string[]  = ARGHAND_ARG("-S", "--string");
    PU8 console[] = ARGHAND_ARG("-c", "--console");

    PPU8 allAliases[] = ARGHAND_ALL_ALIASES(
        help, name, send, receive, file, string, console
    );

    U32 aliasCounts[] = {
        ARGHAND_ALIAS_COUNT(help),
        ARGHAND_ALIAS_COUNT(name),
        ARGHAND_ALIAS_COUNT(send),
        ARGHAND_ALIAS_COUNT(receive),
        ARGHAND_ALIAS_COUNT(file),
        ARGHAND_ALIAS_COUNT(string),
        ARGHAND_ALIAS_COUNT(console)
    };

    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, argv, allAliases, aliasCounts, 7);

    /* ---------- HELP ---------- */
    if(ARGHAND_IS_PRESENT(&ah,(PU8)"-h") || argc < 2)
    {
        printf(
            #include <SHUTTLE/SHUTTLE.HELP>
        );
        goto cleanup_ok;
    }

    /* ---------- RESULT VARIABLES ---------- */

    U16  port        = 0;
    U16  portBase    = 0;
    BOOL8 sendMode   = FALSE;
    BOOL8 receiveMode= FALSE;
    BOOL8 consoleMode= FALSE;
    PU8  filePath    = NULLPTR;
    PU8  stringData  = NULLPTR;

    /* ---------- PORT ---------- */

    if(!ARGHAND_IS_PRESENT(&ah,(PU8)"-p"))
        FAIL("Error: -p <port> is required.");

    PU8 portStr = ARGHAND_GET_VALUE(&ah,(PU8)"-p");
    if(!portStr)
        FAIL("Error: Missing value after -p.");

    port = (U16)ATOI(portStr);

    if(port < 2 || port > 4)
        FAIL("Error: Only COM ports 2, 3, and 4 are supported.");

    switch (port)
    {
        case 2: portBase = COM2_PORT; break;
        case 3: portBase = COM3_PORT; break;
        case 4: portBase = COM4_PORT; break;
    default:
        FAIL("Error: Invalid port number. Only COM2, COM3, and COM4 are supported.");
        break;
    }
    /* ---------- MODES ---------- */

    sendMode    = ARGHAND_IS_PRESENT(&ah,(PU8)"-s");
    receiveMode = ARGHAND_IS_PRESENT(&ah,(PU8)"-r");
    consoleMode = ARGHAND_IS_PRESENT(&ah,(PU8)"-c");

    /* only one mode allowed */
    U32 modeCount =
        (sendMode ? 1 : 0) +
        (receiveMode ? 1 : 0) +
        (consoleMode ? 1 : 0);

    if(modeCount == 0)
        FAIL("Error: Must specify -s, -r, or -c.");

    if(modeCount > 1)
        FAIL("Error: Modes -s, -r, and -c are mutually exclusive.");

    /* ---------- SEND MODE ---------- */

    if(sendMode)
    {
        BOOL8 hasString = ARGHAND_IS_PRESENT(&ah,(PU8)"-S");
        BOOL8 hasFile   = ARGHAND_IS_PRESENT(&ah,(PU8)"-f");

        if(hasString && hasFile)
            FAIL("Error: Use only one of -S or -f.");

        if(!hasString && !hasFile)
            FAIL("Error: Send mode requires -S or -f.");

        if(hasString)
        {
            stringData = ARGHAND_GET_VALUE(&ah,(PU8)"-S");
            if(!stringData)
                FAIL("Error: Missing string after -S.");

            COM_PORT_WRITE_DATA(portBase, stringData, STRLEN(stringData)); 
        }

        if(hasFile)
        {
            filePath = ARGHAND_GET_VALUE(&ah,(PU8)"-f");
            if(!filePath)
                FAIL("Error: Missing file path after -f.");
            FILE *file = FOPEN(filePath, MODE_FR);
            if(!file) 
                FAIL("Error: Failed to open file.");
            COM_PORT_WRITE_DATA(portBase, file->data, file->sz);
            FCLOSE(file);
        }
    }

    /* ---------- RECEIVE MODE ---------- */

    if(receiveMode)
    {
        if(!ARGHAND_IS_PRESENT(&ah,(PU8)"-f"))
            FAIL("Error: Receive mode requires -f <file>.");

        filePath = ARGHAND_GET_VALUE(&ah,(PU8)"-f");
        if(!filePath)
            FAIL("Error: Missing file path after -f.");

        U32 outLen;
        PU8 buffer = COM_PORT_READ_BUFFER_HEAP(portBase, &outLen);
        if(!buffer) 
            FAIL("Error: Failed to read from COM port.");

        FILE *file = FOPEN(filePath, MODE_FW);
        if(!file) {
            if(!FILE_CREATE(filePath)) {
                MFree(buffer);
                FAIL("Error: Failed to create file.");
            }
            printf("File created successfully. Writing data...\n");
            file = FOPEN(filePath, MODE_FW);
            if(!file) {
                MFree(buffer);
                FAIL("Error: Failed to open file after creating.");
            }
        }
        FWRITE(file, buffer, outLen);
        MFree(buffer);
        FCLOSE(file);
    }

    /* ---------- CONSOLE MODE ---------- */

    if(consoleMode)
    {
        /* nothing else required yet */
        printf("SHUTTLE entering console mode.\n");
        while(1) {
            PS2_KB_DATA *kp = kb_poll();
            if(kp && kp->cur.pressed) {
                // Send the keycode to the COM port
                U8 c = keypress_to_char(kp->cur.keycode);
                COM_PORT_WRITE_BYTE(portBase, c);
            }
        }
    }


    // printf("Parsed successfully:\n");
    // printf("  Port: COM%u\n", port);
    // printf("  Send: %u\n", sendMode);
    // printf("  Receive: %u\n", receiveMode);
    // printf("  Console: %u\n", consoleMode);

    // if(filePath)
    //     printf("  File: %s\n", filePath);

    // if(stringData)
    //     printf("  String: %s\n", stringData);

cleanup_ok:
    ARGHAND_FREE(&ah);
    printf("SHUTTLE executed successfully.\n");
    return 0;

cleanup:
    ARGHAND_FREE(&ah);
    printf("SHUTTLE failed to execute.\n");
    return 1;
}