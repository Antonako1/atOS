#include <STD/TYPEDEF.h>
#include <CPU/SYSCALL/SYSCALL.h>
#include <DRIVERS/VIDEO/VBE.h>
#include <STD/ASM.h>
#include <CPU/INTERRUPTS/INTERRUPTS.h>
#include <DRIVERS/PS2/KEYBOARD.h>
#include <HEAP/KHEAP.h>
#include <STD/MEM.h>
#include <PROC/PROC.h>

#define SYSCALL_ENTRY(id, fn) [id] = fn,
static SYSCALL_HANDLER syscall_table[SYSCALL_MAX] = {
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
};
#undef SYSCALL_ENTRY

U32 SYS_VBE_UPDATE_VRAM(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    update_current_framebuffer();
    VBE_UPDATE_VRAM();
    return 0;
}
U32 SYS_VBE_DRAW_CHARACTER(U32 x, U32 y, U32 ch, U32 fg, U32 bg) {
    update_current_framebuffer();
    return (U32)VBE_DRAW_CHARACTER(x, y, (U8)ch, (VBE_PIXEL_COLOUR)fg, (VBE_PIXEL_COLOUR)bg);
}
U32 SYS_VBE_DRAW_STRING(U32 x, U32 y, U32 str, U32 fg, U32 bg) {
    if (!str) return 0;
    update_current_framebuffer();
    return (U32)VBE_DRAW_STRING(x, y, (U8*)str, (VBE_PIXEL_COLOUR)fg, (VBE_PIXEL_COLOUR)bg);
}
U32 SYS_VBE_CLEAR_SCREEN(U32 colour, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    update_current_framebuffer();
    return (U32)VBE_CLEAR_SCREEN((VBE_PIXEL_COLOUR)colour);
}
U32 SYS_VBE_DRAW_PIXEL(U32 x, U32 y, U32 colour, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return (U32)VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x, y, (VBE_PIXEL_COLOUR)colour));
}
U32 SYS_VBE_DRAW_FRAMEBUFFER(U32 pos, U32 colour, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    update_current_framebuffer();
    return (U32)VBE_DRAW_FRAMEBUFFER(pos, (VBE_PIXEL_COLOUR)colour);
}
U32 SYS_VBE_DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, U32 colour) {
    update_current_framebuffer();
    return (U32)VBE_DRAW_ELLIPSE(x, y, rx, ry, (VBE_PIXEL_COLOUR)colour);
}
U32 SYS_VBE_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, U32 colour) {
    update_current_framebuffer();
    U32 retval = (U32)VBE_DRAW_LINE(x1, y1, x2, y2, (VBE_PIXEL_COLOUR)colour);
    return retval;
}
U32 SYS_VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, U32 colour) {
    update_current_framebuffer();
    return (U32)VBE_DRAW_RECTANGLE(x, y, width, height, (VBE_PIXEL_COLOUR)colour);
}

U32 SYS_PS2_KEYBOARD_RESET(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return PS2_KEYBOARD_RESET();
}
U32 SYS_GET_CURRENT_KEY_PRESSED(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    KEYPRESS *key = (KEYPRESS *)KMALLOC(sizeof(KEYPRESS));
    KEYPRESS retval = GET_CURRENT_KEY_PRESSED();
    MEMCPY(key, &retval, sizeof(KEYPRESS));
    return (U32)key;
}
U32 SYS_GET_LAST_KEY_PRESSED(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return (U32)(GET_LAST_KEY_PRESSED());
}
U32 SYS_KEYPRESS_TO_CHARS(U32 kcode, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    U8 *chars = (U8 *)KEYPRESS_TO_CHARS(kcode);
    return (U32)chars;
}
U32 SYS_GET_KEYBOARD_MODIFIERS(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    MODIFIERS *mod = (MODIFIERS *)KMALLOC(sizeof(MODIFIERS));
    if (!mod) return 0;
    MODIFIERS *retval = GET_KEYBOARD_MODIFIERS();
    MEMCPY(mod, retval, sizeof(MODIFIERS));
    Free(retval);
    return (U32)mod;
}


U32 SYS_MESSAGE_AMOUNT(U32 pid, U32 msg_ptr, U32 length, U32 signal, U32 unused5) {
    (void)unused5;
    TCB *t = get_current_tcb();
    if (!t) return 0;
    if (t->info.pid != pid) return 0; // can only get your own message amount
    U32 *res = KMALLOC(sizeof(U32));
    if(!res) return 0;
    MEMCPY(res, &t->msg_count, sizeof(U32));
    return (U32)res;
}
U32 SYS_GET_MESSAGE(U32 pid, U32 msg_ptr, U32 length, U32 signal, U32 unused5) {
    (void)unused5;
    TCB *t = get_current_tcb();
    if (!t) return 0;
    if (t->info.pid != pid) return 0; // can only get your own messages
    if (t->msg_count == 0) return 0; // no messages
    PROC_MESSAGE *msg = &t->msg_queue[t->msg_queue_head];
    PROC_MESSAGE *msg_copy = KMALLOC(sizeof(PROC_MESSAGE));
    if (!msg_copy) return 0;
    MEMCPY(msg_copy, msg, sizeof(PROC_MESSAGE));
    // Advance head
    t->msg_queue_head = (t->msg_queue_head + 1) % PROC_MSG_QUEUE_SIZE;
    t->msg_count--;
    return (U32)msg_copy;
}
U32 SYS_SEND_MESSAGE(U32 msg_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!msg_ptr) return (U32)-1;

    // Allocated in kheap, we have access to it
    PROC_MESSAGE *msg = (PROC_MESSAGE *)msg_ptr;
    send_msg(msg);
    return 0;
}

U32 SYS_GET_CURRENT_TCB(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    TCB *t = get_current_tcb();
    if(t) {
        TCB *copy = (TCB *)KMALLOC(sizeof(TCB));
        if(!copy) return 0;
        MEMCPY(copy, t, sizeof(TCB));
        return (U32)copy;
    }
    return 0;
}
U32 SYS_GET_MASTER_TCB(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    TCB *t = get_master_tcb();
    if(t) {
        TCB *master = (TCB *)KMALLOC(sizeof(TCB));
        if(!master) return 0;
        MEMCPY(master, t, sizeof(TCB));
        return (U32)master;
    }
    return 0;
}
U32 SYS_GET_TCB_BY_PID(U32 pid, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    TCB *t = get_tcb_by_pid(pid);
    if(t) {
        TCB *copy = (TCB *)KMALLOC(sizeof(TCB));
        if(!copy) return 0;
        MEMCPY(copy, t, sizeof(TCB));
        return (U32)copy;
    }
    return 0;
}
U32 SYS_GET_PARENT_TCB(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    TCB *t = get_current_tcb()->parent;
    if(t) {
        TCB *parent = (TCB *)KMALLOC(sizeof(TCB));
        if(!parent) return 0;
        MEMCPY(parent, t, sizeof(TCB));
        return (U32)parent;
    }
    return 0;
}
U32 SYS_PROC_GETPID_BY_NAME(U32 name_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!name_ptr) return (U32)-1;
    U32 pid = get_tcb_by_name((U8 *)name_ptr);
    KFREE((void *)name_ptr);
    return pid;
}

U32 SYS_KMALLOC(U32 size, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    static U32 rki_row = 0;
    U32 ptr = (U32)KMALLOC(size);
    return ptr;
}
U32 SYS_KFREE(U32 pointer, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    KFREE((void *)pointer);
    return 0;
}
U32 SYS_KREALLOC(U32 pointer, U32 oldSize, U32 newSize, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return (U32)KREALLOC((void *)pointer, oldSize, newSize);
}
U32 SYS_KCALLOC(U32 num, U32 size, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    return (U32)KCALLOC(num, size);
}

U32 syscall_dispatcher(U32 num, U32 a1, U32 a2, U32 a3, U32 a4, U32 a5) {
    if (num >= SYSCALL_MAX) return (U32)-1;

    SYSCALL_HANDLER h = syscall_table[num];
    if (!h) return (U32)-1;

    return h(a1, a2, a3, a4, a5);
}



__attribute__((naked)) void isr_syscall(void) {
    asm volatile(
        "pusha\n\t"                  // save all general-purpose regs
        "movl %esp, %eax\n\t"        // save current esp
        "pushl %eax\n\t"             // push regs* argument
        "pushl $0\n\t"               // fake error code
        "pushl $0x80\n\t"            // vector
        "call isr_dispatch_c\n\t"
        "addl $12, %esp\n\t"         // pop arguments
        "popa\n\t"                   // restore registers
        "iret\n\t"
    );
}


