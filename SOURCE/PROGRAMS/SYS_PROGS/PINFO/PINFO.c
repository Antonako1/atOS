#include <STD/TYPEDEF.h>
#include <STD/IO.h>
#include <STD/PROC_COM.h>
#include <STD/TIME.h>
#include <LIBRARIES/ARGHAND/ARGHAND.h>

VOID PRINT_HELP() {
    printf("Usage: pinfo [options]\n"
           "Options:\n"
           "  -h, --help       Show this help\n"
           "  -c, --count      Show total process count\n"
           "  -v, --verbose    Detailed info\n");
}

U32 main(U32 argc, PPU8 argv) {
    BOOL8 show_count = FALSE;
    BOOL8 verbose = FALSE;

    // Define Arguments
    #define argamount 3
    PU8 help_a[] = ARGHAND_ARG("-h", "--help");
    PU8 count_a[] = ARGHAND_ARG("-c", "--count");
    PU8 verb_a[]  = ARGHAND_ARG("-v", "--verbose");

    // Must have all 3 in these arrays to match argamount!
    PPU8 allAliases[] = { help_a, count_a, verb_a };
    U32 aliasCounts[] = { 
        ARGHAND_ALIAS_COUNT(help_a), 
        ARGHAND_ALIAS_COUNT(count_a), 
        ARGHAND_ALIAS_COUNT(verb_a) 
    };
    
    ARGHANDLER ah;
    ARGHAND_INIT(&ah, argc, argv, allAliases, aliasCounts, argamount);

    if (ARGHAND_IS_PRESENT(&ah, "-h")) {
        PRINT_HELP();
        ARGHAND_FREE(&ah);
        return 0;
    }
    show_count = ARGHAND_IS_PRESENT(&ah, "-c");
    verbose    = ARGHAND_IS_PRESENT(&ah, "-v");
    ARGHAND_FREE(&ah);

    TCB *current = GET_CURRENT_TCB();
    TCB *master = GET_MASTER_TCB();
    TCB *next_proc = master;
    U32 total_procs = 0;
    U32 last_pid = 0;

    RTC_DATE_TIME dt = GET_DATE_TIME();
    printf("\nProcess Information as of %02d:%02d:%02d\n", GET_HOURS(&dt), GET_MINUTES(&dt), GET_SECONDS(&dt));
    
    // Header
    if (verbose) {
        printf("%-5s | %-15s | %-8s | %-5s | %-8s | %-8s | %-10s\n", "PID", "NAME", "STATE", "PPID", "TICKS", "SWITCH", "MEM(PG)");
        printf("--------------------------------------------------------------------------\n");
    } else {
        printf("%-5s | %-15s | %-8s | %-10s\n", "PID", "NAME", "STATE", "NOTES");
        printf("----------------------------------------------------------\n");
    }

    do {
        total_procs++;
        TaskInfo *i = &next_proc->info;
        U32 ppid = (next_proc->parent) ? next_proc->parent->info.pid : 0;
        U32 total_pages = next_proc->binary_pages + next_proc->heap_pages + next_proc->stack_pages;

        if (verbose) {
            printf("%-5d | %-15s | %-8d | %-5d | %-8d | %-8d | %-10d\n", 
                i->pid, i->name, i->state, ppid, i->cpu_time, i->num_switches, total_pages);
        } else {
            printf("%-5d | %-15s | %-8d | %-10s\n", 
                i->pid, i->name, i->state, (i->pid == current->info.pid) ? "<SELF>" : "");
        }

        last_pid = i->pid;
        next_proc = next_proc->next;
    } while (next_proc != master && next_proc->info.pid > last_pid);

    if (show_count) printf("\nTotal running processes: %d\n", total_procs);
    
    return 0;
}