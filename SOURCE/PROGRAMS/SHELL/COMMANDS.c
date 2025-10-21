#include "COMMANDS.h"
#include <PROGRAMS/SHELL/SHELL.h>
#include <STD/STRING.h>
#include <CPU/SYSCALL/SYSCALL.h>

#define CMP_CMD(cmd, str) (STRNCMP((U8*)cmd, (U8*)str, STRLEN(str)) == TRUE)

U0 HANDLE_COMMAND(U8 *line) {
    OutputHandle cursor = GetOutputHandle();
    cursor->CURSOR_VISIBLE = FALSE;
    // For now, just echo the command back
    if(CMP_CMD(line, "help")) {
        PRINTNEWLINE();
        PUTS((U8*)"Available commands:"LEND);
        PUTS((U8*)"help - Show this help message" LEND);
        PUTS((U8*)"clear - Clear the screen" LEND);
        PUTS((U8*)"cls - Clear the screen" LEND);
        PUTS((U8*)"echo [text] - Echo the provided text" LEND);
        PUTS((U8*)"tone <freqHz> <ms> [amp] [rate] - Play a square tone" LEND);
        PUTS((U8*)"soundoff - Stop AC97 playback" LEND);
        PUTS((U8*)"exit - Exit the shell" LEND);
        PUTS((U8*)"version - Show shell version" LEND);
    } else if(CMP_CMD(line, "clear") || CMP_CMD(line, "cls")) {
        LE_CLS();
    } else if(CMP_CMD(line, "version")) {
        PRINTNEWLINE();
        PUTS((U8*)"atOS Shell version 1.0.0" LEND);
    } else if(CMP_CMD(line, "exit")) {
        PRINTNEWLINE();
        PUTS((U8*)"Exiting shell..." LEND);
        // In a real shell, you would terminate the shell process here
        // For now, just print a message
    } else if(CMP_CMD(line, "echo ")) {
        // Echo the rest of the line after "echo "
        PRINTNEWLINE();
        U32 echo_start = STRLEN("echo ");
        if(STRLEN(line) > echo_start) {
            PUTS(&line[echo_start]);
            PRINTNEWLINE();
        } else {
            PRINTNEWLINE();
        }
    } else if(CMP_CMD(line, "tone ")) {
        PRINTNEWLINE();
        U8 *s = line + STRLEN("tone ");
        U32 freq = ATOI(s);
        while (*s && *s != ' ') s++;
        if (*s == ' ') s++;
        U32 ms = ATOI(s);
        while (*s && *s != ' ') s++;
        U32 amp = 8000; // default
        if (*s == ' ') { s++; amp = ATOI(s); }
        while (*s && *s != ' ') s++;
        U32 rate = 48000; // default
        if (*s == ' ') { s++; rate = ATOI(s); }

        if (freq == 0 || ms == 0) {
            PUTS((U8*)"Usage: tone <freqHz> <ms> [amp] [rate]" LEND);
        } else {
            U32 res = SYSCALL4(SYSCALL_AC97_TONE, freq, ms, amp, rate);
            if (!res) {
                PUTS((U8*)"tone: AC97 not available or failed" LEND);
            } else {
                PUTS((U8*)"tone: playing..." LEND);
            }
        }
    } else if(CMP_CMD(line, "soundoff")) {
        PRINTNEWLINE();
        SYSCALL0(SYSCALL_AC97_STOP);
        PUTS((U8*)"AC97: stopped" LEND);
    } else if(STRLEN(line) == 0) {
        //
        PRINTNEWLINE();
    } else {
        PRINTNEWLINE();
        PUTS((U8*)"Unknown command: ");
        PUTS(line);
        PRINTNEWLINE();
    }
    // Move to new line and place prompt
    cursor->CURSOR_VISIBLE = TRUE;
}
