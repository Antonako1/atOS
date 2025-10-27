#include <STD/PROC_COM.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <PROC/PROC.h>
#include <STD/DEBUG.h>

static STDOUT *stdout = NULLPTR;
static TCB process ATTRIB_DATA = {0};
static BOOLEAN process_fetched ATTRIB_DATA = FALSE;
static TCB *master ATTRIB_DATA = NULL;
static BOOLEAN master_fetched ATTRIB_DATA = FALSE;
static TCB *parent ATTRIB_DATA = NULL;
static BOOLEAN parent_fetched ATTRIB_DATA = FALSE;

STDOUT *GET_PROC_STDOUT() {
    return stdout;
}

U32 MESSAGE_AMOUNT() {
    U32 res  = (U32 *)SYSCALL(SYSCALL_MESSAGE_AMOUNT, 0, 0, 0, 0, 0);
    return (U32)res;
}

VOID SEND_MESSAGE(PROC_MESSAGE *msg) {
    if (!msg) return;
    PROC_MESSAGE *msg_copy = MAlloc(sizeof(PROC_MESSAGE));
    if (!msg_copy) return;
    MEMCPY(msg_copy, msg, sizeof(PROC_MESSAGE));
    SYSCALL1(SYSCALL_SEND_MESSAGE, (U32)msg_copy);
    MFree(msg_copy);
}
    
PROC_MESSAGE *GET_MESSAGE() {
    return (PROC_MESSAGE *)SYSCALL0(SYSCALL_GET_MESSAGE);
}

VOID FREE_MESSAGE(PROC_MESSAGE *msg) {
    if (!msg) return;
    if (msg->data_provided && msg->data) {
        MFree(msg->data);
    } 
    msg->read = TRUE;
    MFree(msg);
}

U32 PROC_GETPID(U0) {
    return GET_CURRENT_TCB()->info.pid;
}
U8 *PROC_GETNAME(U0) {
    return GET_CURRENT_TCB()->info.name;
}

U32 PROC_GETPID_BY_NAME(U8 *arg) {
    if (!arg) return (U32)-1;
    U8 *name = MAlloc(STRLEN(arg) + 1);
    if (!name) return (U32)-1;
    MEMCPY(name, arg, STRLEN(arg) + 1);
    return (U32)SYSCALL(SYSCALL_PROC_GETPID_BY_NAME, (U32)name, 0, 0, 0, 0);
}
U32 PROC_GETPPID(U0) {
    if (!GET_PARENT_TCB()) return (U32)-1;
    return GET_PARENT_TCB()->info.pid;
}
U8 *PROC_GETPARENTNAME(U0) {
    TCB *p = GET_PARENT_TCB();
    if (p) {
        return p->info.name;
    }
    return NULL;
}
TCB *GET_CURRENT_TCB(void) {
    if(process_fetched) {
        return &process;
    }
    TCB *t = (TCB *)SYSCALL(SYSCALL_GET_CUR_TCB, 0, 0, 0, 0, 0);
    
    if(t) {
        MEMCPY(&process, t, sizeof(TCB));
        MFree(t);
        process_fetched = TRUE;
        return &process;
    }
}
TCB *GET_MASTER_TCB(void) {
    if(master_fetched) {
        return master;
    }
    TCB *t = (TCB *)SYSCALL(SYSCALL_GET_MASTER_TCB, 0, 0, 0, 0, 0);
    if(t) {
        master = (TCB *)MAlloc(sizeof(TCB));
        if(!master) {
            MFree(t);
            return NULL;
        }
        MEMCPY(master, t, sizeof(TCB));
        MFree(t);
        master_fetched = TRUE;
        return master;
    }
    return NULL;
}
TCB *GET_TCB_BY_PID(U32 pid) {
    if (!pid) return NULL;
    TCB *t = (TCB *)SYSCALL(SYSCALL_GET_TCB_BY_PID, pid, 0, 0, 0, 0);
    if(t) {
        TCB *copy = (TCB *)MAlloc(sizeof(TCB));
        if(!copy) {
            MFree(t);
            return NULL;
        }
        MEMCPY(copy, t, sizeof(TCB));
        MFree(t);
        return copy;
    }
    return NULL;
}
TCB *GET_PARENT_TCB(void) {
    if(parent_fetched) {
        return parent;
    }
    TCB *t = (TCB *)SYSCALL(SYSCALL_GET_PARENT_TCB, 0, 0, 0, 0, 0);
    if(t) {
        parent = (TCB *)MAlloc(sizeof(TCB));
        if(!parent) {
            MFree(t);
            return NULL;
        }
        MEMCPY(parent, t, sizeof(TCB));
        MFree(t);
        parent_fetched = TRUE;
        return parent;
    }
    return NULL;
}
void FREE_TCB(TCB *tcb) {
    if (!tcb) return;
    MFree(tcb);
}

U32 GET_PIT_TICKS() {
    return SYSCALL0(SYSCALL_GET_PIT_TICK);
}

U32 GET_SYS_SECONDS() {
    return SYSCALL0(SYSCALL_GET_SECONDS);
}

U32 CPU_SLEEP(U32 ms) {
    return SYSCALL1(SYSCALL_PIT_SLEEP, ms);
}

VOID EXIT(U32 n) {
    PROC_MESSAGE msg;

    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_SET_FOCUS, NULL, 0, PROC_GETPPID());
    SEND_MESSAGE(&msg);
    msg = CREATE_PROC_MSG(PROC_GETPPID(), SHELL_CMD_ENDED_MYSELF, NULL, 0, n);
    SEND_MESSAGE(&msg);
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_TERMINATE_SELF, NULL, 0, n);
    SEND_MESSAGE(&msg);
    for(;;); // Loop here until safe to continue
}

static U8 ___keyboard_access_granted = FALSE;
static U8 ___draw_access_granted = FALSE;
static U8 ___STDOUT_CREATED = FALSE;
void PRIC_INIT_GRAPHICAL() {
    PROC_INIT_CONSOLE();
}

void PROC_INIT_CONSOLE() {
    PROC_MESSAGE msg;

    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_GET_FRAMEBUFFER, NULL, 0, 0);
    SEND_MESSAGE(&msg);

    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_GET_KEYBOARD_EVENTS, NULL, 0, 0);
    SEND_MESSAGE(&msg);

    msg = CREATE_PROC_MSG(PROC_GETPPID(), SHELL_CMD_CREATE_STDOUT, NULL, 0, 0);
    SEND_MESSAGE(&msg);
    ___keyboard_access_granted = FALSE;
    ___draw_access_granted = FALSE;
    ___STDOUT_CREATED = FALSE;
}
BOOLEAN IS_PROC_INITIALIZED() {
    U32 msg_count = MESSAGE_AMOUNT();
    for(U32 i = 0; i <= msg_count; i++) {
        PROC_MESSAGE *msg = GET_MESSAGE(); // Gets last message. Marked as read
        if (!msg) break;
        switch(msg->type) {
            case PROC_KEYBOARD_EVENTS_GRANTED: ___keyboard_access_granted = TRUE; break;
            case PROC_FRAMEBUFFER_GRANTED: ___draw_access_granted = TRUE; break;
            case SHELL_RES_STDOUT_CREATED: 
                if(msg->raw_data && msg->raw_data_size == sizeof(STDOUT*)) {
                    stdout = msg->raw_data;
                    ___STDOUT_CREATED = TRUE; 
                } else 
                    EXIT(1);
                break;
            case SHELL_RES_STDOUT_FAILED: EXIT(1); break;
        }
        FREE_MESSAGE(msg);
    }
    return (
        ___keyboard_access_granted && 
        ___draw_access_granted &&
        ___STDOUT_CREATED 
    ) ? TRUE : FALSE;
}

BOOLEAN START_PROCESS(
    U8 *proc_name, 
    VOIDPTR file, 
    U32 bin_size, 
    U32 initial_state, 
    U32 parent_pid,
    PPU8 argv,
    U32 argc
) {
    RUN_BINARY_STRUCT *sc = MAlloc(sizeof(RUN_BINARY_STRUCT)); // Will be freed by kernel
    if(!sc) return FALSE;
    STRNCPY(sc->proc_name, proc_name, 255);
    sc->file = file; 
    sc->bin_size = bin_size; 
    sc->initial_state = initial_state; 
    sc->parent_pid = parent_pid;
    sc->argv = argv;
    sc->argc = argc;
    PROC_MESSAGE msg;
    msg = CREATE_PROC_MSG(KERNEL_PID, PROC_MSG_CREATE_PROCESS, sc, sizeof(RUN_BINARY_STRUCT), 0xDEADBEEF);
    SEND_MESSAGE(&msg);
    return TRUE;
}


VOID KILL_SELF() {
    EXIT(0);
}
VOID START_HALT() {
    for(;;) ;
}