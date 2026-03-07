#include <STD/TYPEDEF.h>
#include <STD/AUDIO.h>
#include <STD/STRING.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/DEBUG.h>

VOID HELP_MESSAGE() {
    printf(
        #include <METRONOME.HELP>
    );
}

U32 NOTE_INTERVAL(U32 bpm, U32 num, U32 den)
{
    U32 quarter = 6000 / bpm;
    return (quarter * 4 * num) / den;
}

CMAIN() {

    if(argc != 3) {
        HELP_MESSAGE();
        return 1;
    }

    if(STRCMP(argv[1], "-h") == 0 || STRCMP(argv[1], "--help") == 0) {
        HELP_MESSAGE();
        return 0;
    }

    U32 bpm = ATOI(argv[1]);
    U32 note = ATOI(argv[2]);

    if(bpm == 0 || note == 0) {
        printf("Invalid BPM or note interval\n");
        return 1;
    }

    U32 interval = NOTE_INTERVAL(bpm, 1, note);

    printf("Press ESC to quit %s\n", argv[0]);

    DISABLE_SHELL_KEYBOARD();

    U32 last_tick = GET_PIT_TICKS();
    U32 cur_tick = last_tick;

    BOOL8 running = TRUE;
    BOOL8 lo = FALSE;

    U32 tick = 0;
    U32 ticks_per_beat = note / 2;

    while(running) {

        cur_tick = GET_PIT_TICKS();

        if(cur_tick - last_tick >= interval) {

            last_tick = cur_tick;

            if(tick % ticks_per_beat == 0)
                AUDIO_TONE(880, 75, 48000, 44100);
            else
                AUDIO_TONE(660, 75, 48000, 44100);

            tick++;
        }

        PS2_KB_DATA *kp = kb_poll();

        if(kp && kp->cur.pressed) {
            if(kp->cur.keycode == KEY_ESC)
                running = FALSE;
        }
    }

    AUDIO_STOP();
    ENABLE_SHELL_KEYBOARD();

    return 0;
}