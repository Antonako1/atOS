/* CMD/AC97.c — Audio commands for TSHELL */

VOID CMD_TONE(U8 *line) {
    U8 *s = line + STRLEN("tone ");
    U32 freq = ATOI(s);
    while (*s && *s != ' ') s++;
    if (*s == ' ') s++;
    U32 ms = ATOI(s);
    while (*s && *s != ' ') s++;

    U32 amp = 8000;
    if (*s == ' ') { s++; amp = ATOI(s); }
    while (*s && *s != ' ') s++;

    U32 rate = 48000;
    if (*s == ' ') { s++; rate = ATOI(s); }

    PRINTNEWLINE();
    if (freq == 0 || ms == 0) {
        PUTS("Usage: tone <freqHz> <ms> [amp] [rate]" LEND);
        return;
    }

    U32 res = AUDIO_TONE(freq, ms, amp, rate);
    PUTS(res ? "tone: playing..." LEND : "tone: AC97 not available" LEND);
}

VOID CMD_SOUNDOFF(U8 *line) {
    (void)line;
    AUDIO_STOP();
    PUTS("AC97: stopped" LEND);
}
