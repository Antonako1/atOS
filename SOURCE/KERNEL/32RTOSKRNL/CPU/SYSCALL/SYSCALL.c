#include <STD/TYPEDEF.h>
#include <STD/ASM.h>
#include <STD/MEM.h>

#include <PROC/PROC.h>

#include <CPU/SYSCALL/SYSCALL.h>
#include <CPU/INTERRUPTS/INTERRUPTS.h>

#include <MEMORY/HEAP/KHEAP.h>

#include <FS/ISO9660/ISO9660.h>
#include <FS/FAT/FAT.h>

#include <DRIVERS/VIDEO/VBE.h>
#include <DRIVERS/PS2/KEYBOARD.h>
#include <DRIVERS/ATA_PIO/ATA_PIO.h>
#include <DRIVERS/AC97/AC97.h>

#define SYSCALL_ENTRY(id, fn) [id] = fn,
static SYSCALL_HANDLER syscall_table[SYSCALL_MAX] = {
    #include <CPU/SYSCALL/SYSCALL_LIST.h>
};
#undef SYSCALL_ENTRY

U32 SYS_HDD_READ_SECTOR(U32 lba, U32 sector_count, U32 buf, U32 unused4, U32 unused5) {
    return ATA_PIO_READ_SECTORS(lba, sector_count, buf);
}
U32 SYS_HDD_WRITE_SECTOR(U32 lba, U32 sector_count, U32 buf, U32 unused4, U32 unused5) {
    return ATA_PIO_WRITE_SECTORS(lba, sector_count, buf);
}

U32 SYS_VBE_UPDATE_VRAM(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;

    VBE_UPDATE_VRAM();
    return 0;
}
U32 SYS_VBE_DRAW_CHARACTER(U32 x, U32 y, U32 ch, U32 fg, U32 bg) {

    return (U32)VBE_DRAW_CHARACTER(x, y, (U8)ch, (VBE_PIXEL_COLOUR)fg, (VBE_PIXEL_COLOUR)bg);
}
U32 SYS_VBE_DRAW_STRING(U32 x, U32 y, U32 str, U32 fg, U32 bg) {
    if (!str) return 0;

    return (U32)VBE_DRAW_STRING(x, y, (U8*)str, (VBE_PIXEL_COLOUR)fg, (VBE_PIXEL_COLOUR)bg);
}
U32 SYS_VBE_CLEAR_SCREEN(U32 colour, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;

    U32 retval = (U32)VBE_CLEAR_SCREEN((VBE_PIXEL_COLOUR)colour);
    return retval;
}
U32 SYS_VBE_DRAW_PIXEL(U32 x, U32 y, U32 colour, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;

    return (U32)VBE_DRAW_PIXEL(CREATE_VBE_PIXEL_INFO(x, y, (VBE_PIXEL_COLOUR)colour));
}
U32 SYS_VBE_DRAW_FRAMEBUFFER(U32 pos, U32 colour, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;

    return (U32)VBE_DRAW_FRAMEBUFFER(pos, (VBE_PIXEL_COLOUR)colour);
}
U32 SYS_VBE_DRAW_ELLIPSE(U32 x, U32 y, U32 rx, U32 ry, U32 colour) {

    return (U32)VBE_DRAW_ELLIPSE(x, y, rx, ry, (VBE_PIXEL_COLOUR)colour);
}
U32 SYS_VBE_DRAW_LINE(U32 x1, U32 y1, U32 x2, U32 y2, U32 colour) {

    U32 retval = (U32)VBE_DRAW_LINE(x1, y1, x2, y2, (VBE_PIXEL_COLOUR)colour);
    return retval;
}
U32 SYS_VBE_DRAW_RECTANGLE(U32 x, U32 y, U32 width, U32 height, U32 colour) {

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
    U8 chars = KEYPRESS_TO_CHARS(kcode);

    return (U32)chars;
}
U32 SYS_GET_KEYBOARD_MODIFIERS(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    MODIFIERS *mod = (MODIFIERS *)KMALLOC(sizeof(MODIFIERS));
    if (!mod) return 0;
    MODIFIERS *retval = GET_KEYBOARD_MODIFIERS();
    MEMCPY(mod, retval, sizeof(MODIFIERS));
    KFREE(retval);
    return (U32)mod;
}

U32 SYS_GET_PIT_TICK(U32 unused1, U32 unusef2, U32 unused3, U32 unused4, U32 unused5) {
    return get_ticks();
}
U32 SYS_GET_SECONDS(U32 unused1, U32 unusef2, U32 unused3, U32 unused4, U32 unused5) {
    return get_uptime_sec();
}
U32 SYS_MESSAGE_AMOUNT(U32 pid, U32 msg_ptr, U32 length, U32 signal, U32 unused5) {
    (void)unused5;
    TCB *t = get_current_tcb();
    if (!t) return 0;
    U32 res = t->msg_count;
    return (U32)res;
}
U32 SYS_GET_MESSAGE(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused5;
    TCB *t = get_current_tcb();
    if (!t) return 0;
    if (t->msg_count == 0) return 0; // no messages

    PROC_MESSAGE *msg = &t->msg_queue[t->msg_queue_head];
    PROC_MESSAGE *msg_copy = KMALLOC(sizeof(PROC_MESSAGE));
    if (!msg_copy) return 0;

    MEMCPY(msg_copy, msg, sizeof(PROC_MESSAGE));

    if (msg->data_provided && msg->data) {
        // Determine how much data needs to be copied.
        // This should be stored in the message (you can add msg->data_size field)
        U32 data_size = msg->signal; // or another field that holds the size
        if (data_size == 0) data_size = sizeof(KEYPRESS) + sizeof(MODIFIERS); // fallback
        VOIDPTR data_copy = KMALLOC(data_size);
        if (data_copy) {
            MEMCPY(data_copy, msg->data, data_size);
            msg_copy->data = data_copy;
        } else {
            msg_copy->data_provided = FALSE;
            msg_copy->data = NULL;
        }
    }

    // Advance queue head
    t->msg_queue_head = (t->msg_queue_head + 1) % PROC_MSG_QUEUE_SIZE;
    t->msg_count--;

    return (U32)msg_copy;
}

U32 SYS_SEND_MESSAGE(U32 msg_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!msg_ptr) return (U32)-1;

    // Allocated in kheap, we have access to it. 
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

U32 SYS_GET_KP_DATA(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return GET_KP_DATA();
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
U32 SYS_KREALLOC(U32 pointer, U32 newSize, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return (U32)KREALLOC((void *)pointer, newSize);
}
U32 SYS_KCALLOC(U32 num, U32 size, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    return (U32)KCALLOC(num, size);
}

U32 SYS_CDROM_READ(U32 lba, U32 sectors, U32 buf_ptr, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    if (!buf_ptr || sectors == 0) return 0;
    U8 *buf = (U8 *)buf_ptr;
    U32 res = READ_CDROM(INITIALIZE_ATAPI(), lba, sectors, buf);
    return res;
}

U32 SYS_ISO9660_READ_ENTRY(U32 path_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!path_ptr) return 0;
    IsoDirectoryRecord *out = ISO9660_FILERECORD_TO_MEMORY(path_ptr);
    return (U32)out;
}
U32 SYS_ISO9660_FILECONTENTS(U32 record_ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!record_ptr) return 0;
    IsoDirectoryRecord *fileptr = ISO9660_FILERECORD_TO_MEMORY((IsoDirectoryRecord *)record_ptr);
    VOIDPTR data = ISO9660_READ_FILEDATA_TO_MEMORY(fileptr);
    return (U32)data;
}
U32 SYS_ISO9660_FREE_MEMORY(U32 ptr, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    ISO9660_FREE_MEMORY((VOIDPTR)ptr);
    return 0;
}

// Audio syscalls
U32 SYS_AC97_TONE(U32 freq, U32 ms, U32 amp, U32 rate, U32 unused5) {
    (void)unused5;
    BOOLEAN ok = AC97_TONE(freq, ms, rate, (U16)amp);
    return ok ? 1 : 0;
}
U32 SYS_AC97_STOP(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    AC97_STOP();
    return 0;
}


U32 SYS_GET_ROOT_CLUSTER(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1;(void)unused2;(void)unused3;(void)unused4;(void)unused5;
    return GET_ROOT_CLUSTER();
}
U32 SYS_FIND_DIR_BY_NAME_AND_PARENT(U32 parent, U32 name_ptr, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3;(void)unused4;(void)unused5;
    return FIND_DIR_BY_NAME_AND_PARENT(parent, (PU8)name_ptr);
}
U32 SYS_FIND_FILE_BY_NAME_AND_PARENT(U32 parent, U32 name_ptr, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3;(void)unused4;(void)unused5;
    return FIND_FILE_BY_NAME_AND_PARENT(parent, (PU8)name_ptr);
}
U32 SYS_FIND_DIR_ENTRY_BY_NAME_AND_PARENT(U32 out_ptr, U32 parent, U32 name_ptr, U32 unused4, U32 unused5) {
    (void)unused4;(void)unused5;
    return FIND_DIR_ENTRY_BY_NAME_AND_PARENT((DIR_ENTRY *)out_ptr, parent, (PU8)name_ptr);
}
U32 SYS_READ_LFNS(U32 dir_ent_ptr, U32 lfn_out_ptr, U32 size_out_ptr, U32 unused4, U32 unused5) {
    (void)unused4;(void)unused5;
    return READ_LFNS((DIR_ENTRY*)dir_ent_ptr, (LFN*)lfn_out_ptr, (PU32)size_out_ptr);
}
U32 SYS_CREATE_CHILD_DIR(U32 parent_cluster, U32 name_ptr, U32 attrib, U32 cluster_out, U32 unused5) {
    (void)unused5;
    BOOL res = CREATE_CHILD_DIR(parent_cluster, (PU8)name_ptr, (U8)attrib, (PU32)cluster_out);
    return res;
}
U32 SYS_GET_ROOT_DIR_ENTRY(U32 unused1, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused1;(void)unused2;(void)unused3;(void)unused4;(void)unused5;
    DIR_ENTRY e = GET_ROOT_DIR_ENTRY();
    DIR_ENTRY *ep = KMALLOC((sizeof(DIR_ENTRY)));
    if(!ep) return NULLPTR;
    return (U32)ep;
}
U32 SYS_CREATE_CHILD_FILE(U32 parent_cluster, U32 name_ptr, U32 attrib, U32 filedata_ptr, U32 filedata_size) {
    U32 cluster_out = 0;
    BOOL res = CREATE_CHILD_FILE(parent_cluster, (PU8)name_ptr, (U8)attrib, (PU8)filedata_ptr, filedata_size, &cluster_out);
    if (res) {
        MEMCPY((PU32)attrib, &cluster_out, sizeof(U32)); // Write cluster_out back to user space
    }
    return res;
}

U32 SYS_DIR_ENUMERATE_LFN(U32 dir_cluster, U32 out_entries, U32 max_count_ptr, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return DIR_ENUMERATE_LFN(dir_cluster, (FAT_LFN_ENTRY*)out_entries, (PU32)max_count_ptr);
}

U32 SYS_DIR_ENUMERATE(U32 dir_cluster, U32 out_entries, U32 max_count_ptr, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return DIR_ENUMERATE(dir_cluster, (DIR_ENTRY*)out_entries, (PU32)max_count_ptr);
}

U32 SYS_DIR_REMOVE_ENTRY(U32 entry_ptr, U32 name_ptr, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    return DIR_REMOVE_ENTRY((DIR_ENTRY*)entry_ptr, (const char*)name_ptr);
}

U32 SYS_READ_FILE_CONTENTS(U32 entry_ptr, U32 size_out_ptr, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    return (U32)READ_FILE_CONTENTS((PU32)size_out_ptr, (DIR_ENTRY*)entry_ptr);
}

U32 SYS_FILE_WRITE(U32 entry_ptr, U32 data_ptr, U32 size, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return FILE_WRITE((DIR_ENTRY*)entry_ptr, (const U8*)data_ptr, size);
}

U32 SYS_FILE_APPEND(U32 entry_ptr, U32 data_ptr, U32 size, U32 unused4, U32 unused5) {
    (void)unused4; (void)unused5;
    return FILE_APPEND((DIR_ENTRY*)entry_ptr, (const U8*)data_ptr, size);
}

U32 SYS_PATH_RESOLVE_ENTRY(U32 path_ptr, U32 out_entry_ptr, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused3; (void)unused4; (void)unused5;
    return PATH_RESOLVE_ENTRY((U8*)path_ptr, (FAT_LFN_ENTRY*)out_entry_ptr);
}



U32 syscall_dispatcher(U32 num, U32 a1, U32 a2, U32 a3, U32 a4, U32 a5) {
    if (num >= SYSCALL_MAX) return (U32)-1;
    SYSCALL_HANDLER h = syscall_table[num];
    if (!h) return (U32)-1;

    return h(a1, a2, a3, a4, a5);
}


U32 SYS_PIT_SLEEP(U32 ms, U32 unused2, U32 unused3, U32 unused4, U32 unused5) {
    (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (ms == 0) return 1;
    PIT_WAIT_MS(ms);
    return 0;
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

