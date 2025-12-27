#ifndef PIT_H
#define PIT_H
#include "../../../STD/TYPEDEF.h"
#define PIT_VECTOR 0x20 // IRQ0
#define PIT_TICKS_HZ 100 // default 100 Hz
#define PIT_TICK_MS (1000 / PIT_TICKS_HZ) // in milliseconds (10 ms for 100 Hz)


// Time conversion helpers
#define TICKS_PER_SECOND    PIT_TICKS_HZ
#define MS_TO_TICKS(ms)     ((ms) / PIT_TICK_MS)
#define SECONDS_TO_TICKS(s) ((s) * PIT_TICKS_HZ)

// Check if current tick aligns with an interval (e.g., 30 Hz screen updates)
#define EVERY_TICKS(tcks, interval_ticks)  ((tcks % (interval_ticks)) == 0)
#define EVERY_MS(tcks, ms)                 EVERY_TICKS(tcks, MS_TO_TICKS(ms))
#define EVERY_HZ(tcks, hz)                 EVERY_TICKS(tcks, PIT_TICKS_HZ / (hz))
#define REFRESH_HZ 75 // Screen refresh
#ifdef __RTOS__
#include <PROC/PROC.h> // for TrapFrame struct


// Time conversion helpers
#define TICKS_PER_SECOND    PIT_TICKS_HZ
#define MS_TO_TICKS(ms)     ((ms) / PIT_TICK_MS)
#define SECONDS_TO_TICKS(s) ((s) * PIT_TICKS_HZ)

// Check if current tick aligns with an interval (e.g., 30 Hz screen updates)
#define EVERY_TICKS(tcks, interval_ticks)  ((tcks % (interval_ticks)) == 0)
#define EVERY_MS(tcks, ms)                 EVERY_TICKS(tcks, MS_TO_TICKS(ms))
#define EVERY_HZ(tcks, hz)                 EVERY_TICKS(tcks, PIT_TICKS_HZ / (hz))

U0 PIT_INIT(U0);
U32 *PIT_GET_TICKS_PTR();
U32 *PIT_GET_HZ_PTR();
void pit_set_frequency(U32 freq);
U0 PIT_WAIT_MS(U32 ms);

void set_next_task_esp_val(U32 esp);
void set_next_task_cr3_val(U32 cr3);
U32 get_current_task_esp(void);
void set_next_task_pid(U32 pid);
void set_next_task_num_switches(U32 num_switches);

void update_next_cr3();

void isr_pit(void);

void SCHEDULER_YIELD(void);
#endif


#endif // PIT_H