#include <STD/PROC_COM.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <STD/MEM.h>
#include <STD/STRING.h>

static TCB process ATTRIB_DATA = {0};
static BOOLEAN process_fetched ATTRIB_DATA = FALSE;
static TCB *master ATTRIB_DATA = NULL;
static BOOLEAN master_fetched ATTRIB_DATA = FALSE;
static TCB *parent ATTRIB_DATA = NULL;
static BOOLEAN parent_fetched ATTRIB_DATA = FALSE;

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
    Free(msg_copy);
}
    
PROC_MESSAGE *GET_MESSAGE() {
    return (PROC_MESSAGE *)SYSCALL0(SYSCALL_GET_MESSAGE);
}

VOID FREE_MESSAGE(PROC_MESSAGE *msg) {
    if (!msg) return;
    if (msg->data_provided && msg->data) {
        Free(msg->data);
    } 
    msg->read = TRUE;
    Free(msg);
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
        Free(t);
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
            Free(t);
            return NULL;
        }
        MEMCPY(master, t, sizeof(TCB));
        Free(t);
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
            Free(t);
            return NULL;
        }
        MEMCPY(copy, t, sizeof(TCB));
        Free(t);
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
            Free(t);
            return NULL;
        }
        MEMCPY(parent, t, sizeof(TCB));
        Free(t);
        parent_fetched = TRUE;
        return parent;
    }
    return NULL;
}
void FREE_TCB(TCB *tcb) {
    if (!tcb) return;
    Free(tcb);
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