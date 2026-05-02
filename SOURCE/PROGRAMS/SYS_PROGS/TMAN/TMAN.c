/**
 * TMAN - Task Manager
 * Part of atOS
 */
#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/PROC_COM.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>

PATGL_NODE task_list;
PATGL_NODE header_label;
PATGL_NODE proc_header;
PATGL_NODE perf_panel;
PATGL_NODE perf_heap_label;
PATGL_NODE perf_tasks_label;

PATGL_NODE btn_tab_proc;
PATGL_NODE btn_tab_perf;
PATGL_NODE refresh_btn;
PATGL_NODE kill_btn;

TCB *master_tcb;

#define ACTIVE_PID_MAX_COUNT 256

U32 active_pids[ACTIVE_PID_MAX_COUNT];
U32 active_pid_count = 0;

typedef enum {
    TAB_PROCESSES,
    TAB_PERFORMANCE
} TMAN_TAB;

TMAN_TAB current_tab = TAB_PROCESSES;

static VOID refresh_tasks(VOID)
{
    if (current_tab == TAB_PROCESSES) {
        ATGL_LISTBOX_CLEAR(task_list);
    }
    active_pid_count = 0;
    
    if (!master_tcb) return;

    TCB *current = master_tcb;
    U32 total_alloc_pages = 0;

    U32 last_pid = 0;
    
    do {
        U32 mem_pages = current->binary_pages + current->heap_pages + current->stack_pages + current->framebuffer_pages;
        total_alloc_pages += mem_pages;
        
        if (current_tab == TAB_PROCESSES) {
            CHAR buf[256];
            PU8 state_str = (PU8)"Unknown";
            if(current->info.state & TCB_STATE_INFO_CHILD_PROC_HANDLER) state_str = (PU8)"Shell";
            else if (current->info.state & TCB_STATE_ACTIVE) state_str = (PU8)"Active";
            else if (current->info.state & TCB_STATE_WAITING) state_str = (PU8)"Waiting";
            else if (current->info.state & TCB_STATE_SLEEPING) state_str = (PU8)"Sleeping";
            else if (current->info.state & TCB_STATE_ZOMBIE) state_str = (PU8)"Zombie";
            else if(current->info.state & TCB_STATE_LIBRARY) state_str = (PU8)"Library";
            else if(current->info.state & TCB_STATE_KILL) state_str = (PU8)"Marked for Kill";
            else if(current->info.state & TCB_STATE_IMMORTAL) state_str = (PU8)"Immortal";

            U32 mem_kb = mem_pages * 4;

            U32 heap_allocated = current->info.heap_allocated;
            
            SNPRINTF(buf, sizeof(buf), " %-5d | %-20s | %-8s | %5d KB, %5d KB | %-8d | %d |", 
                     current->info.pid, current->info.name, state_str, mem_kb, heap_allocated / 1024, current->info.cpu_time, current->info.num_switches);
            
            ATGL_LISTBOX_ADD_ITEM(task_list, (PU8)buf);
        }
        
        if (active_pid_count < ACTIVE_PID_MAX_COUNT) {
            active_pids[active_pid_count++] = current->info.pid;
        }
        
        last_pid = current->info.pid;
        current = current->next;
    } while(current != master_tcb && current != NULL && current->info.pid > last_pid); // Added check to prevent infinite loop in case of corrupted list

    if (current_tab == TAB_PERFORMANCE) {
        KHeap *kheap = GET_KERNEL_HEAP_INFO();
        if (kheap) {
            CHAR hbuf[128];
            SNPRINTF(hbuf, sizeof(hbuf), "Kernel Heap: %d KB Free / %d KB Total", kheap->freeSize / 1024, kheap->totalSize / 1024);
            ATGL_NODE_SET_TEXT(perf_heap_label, (PU8)hbuf);
        }
        
        CHAR tbuf[128];
        SNPRINTF(tbuf, sizeof(tbuf), "Processes: %d | App Memory Alloc: %d KB", active_pid_count, total_alloc_pages * 4);
        ATGL_NODE_SET_TEXT(perf_tasks_label, (PU8)tbuf);
    }
}

static VOID set_tab(TMAN_TAB tab) {
    current_tab = tab;
    
    if (tab == TAB_PROCESSES) {
        ATGL_NODE_SET_VISIBLE(proc_header, TRUE);
        ATGL_NODE_SET_VISIBLE(task_list, TRUE);
        ATGL_NODE_SET_VISIBLE(kill_btn, TRUE);
        
        ATGL_NODE_SET_VISIBLE(perf_panel, FALSE);
    } else {
        ATGL_NODE_SET_VISIBLE(proc_header, FALSE);
        ATGL_NODE_SET_VISIBLE(task_list, FALSE);
        ATGL_NODE_SET_VISIBLE(kill_btn, FALSE);
        
        ATGL_NODE_SET_VISIBLE(perf_panel, TRUE);
    }
    refresh_tasks();
}

static VOID on_tab_proc(PATGL_NODE node) {
    (void)node;
    set_tab(TAB_PROCESSES);
}

static VOID on_tab_perf(PATGL_NODE node) {
    (void)node;
    set_tab(TAB_PERFORMANCE);
}

static VOID on_refresh(PATGL_NODE node)
{
    (void)node;
    refresh_tasks();
}

static VOID on_kill(PATGL_NODE node)
{
    (void)node;
    if (current_tab != TAB_PROCESSES) return;
    
    PU8 txt = ATGL_LISTBOX_GET_TEXT(task_list);
    if (!txt) return;
    
    PU8 p = txt;
    while(*p == ' ') p++;
    U32 sel_pid = ATOI(p);

    DEBUG_PRINTF("[TMAN] Attempting to kill PID: %d, %s\n", sel_pid, txt);
    if (sel_pid != 0xFFFFFFFF && sel_pid != KERNEL_PID) {
        if(sel_pid == PROC_GETPID()) {
            DEBUG_PRINTF("[TMAN] Attempting to kill self, exiting instead.\n");
            ATGL_QUIT();
        } else {
            KILL_PROCESS_INSTANCE(sel_pid);
        }
    }
    refresh_tasks();
}

CMAIN()
{
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);
    
    ATGL_INIT();
    ATGL_CREATE_SCREEN(ATGL_SA_NONE);
    
    master_tcb = GET_MASTER_TCB(); // prefetch master TCB

    U32 width = ATGL_GET_SCREEN_WIDTH();
    U32 height = ATGL_GET_SCREEN_HEIGHT();
    
    PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();
    
    // Top Tabs
    header_label = ATGL_CREATE_PANEL(root, (ATGL_RECT){0, 0, width, 40}, ATGL_LAYOUT_HORIZONTAL, 4, 8);
    ATGL_NODE_SET_COLORS(header_label, RGB(255, 255, 255), RGB(40, 40, 40));
    
    btn_tab_proc = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 100, 24}, (PU8)"Processes", on_tab_proc);
    btn_tab_perf = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 100, 24}, (PU8)"Performance", on_tab_perf);
    
    refresh_btn = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 80, 24}, (PU8)"Refresh", on_refresh);
    kill_btn = ATGL_CREATE_BUTTON(header_label, (ATGL_RECT){0, 0, 80, 24}, (PU8)"End Task", on_kill);
    
    // Processes Column Header
    proc_header = ATGL_CREATE_PANEL(root, (ATGL_RECT){0, 40, width, 24}, ATGL_LAYOUT_HORIZONTAL, 4, 8);
    ATGL_NODE_SET_COLORS(proc_header, RGB(255, 255, 255), RGB(80, 80, 80));
    ATGL_CREATE_LABEL(proc_header, (ATGL_RECT){0, 0, width, 24}, (PU8)" PID   | Name                 | State    | ProcMem, HeapMem   | CPU Time | Switches", RGB(220, 220, 220), RGB(80, 80, 80));

    // Processes Listbox
    task_list = ATGL_CREATE_LISTBOX(root, (ATGL_RECT){0, 64, width, height - 64}, 20);
    
    // Performance Tab Panel
    perf_panel = ATGL_CREATE_PANEL(root, (ATGL_RECT){0, 40, width, height - 40}, ATGL_LAYOUT_VERTICAL, 10, 10);
    perf_heap_label = ATGL_CREATE_LABEL(perf_panel, (ATGL_RECT){0, 0, 400, 24}, (PU8)"Heap info...", RGB(255, 255, 255), VBE_SEE_THROUGH);
    perf_tasks_label = ATGL_CREATE_LABEL(perf_panel, (ATGL_RECT){0, 0, 400, 24}, (PU8)"Tasks info...", RGB(255, 255, 255), VBE_SEE_THROUGH);
    
    set_tab(TAB_PROCESSES);
    
    return 0;
}

void ATGL_EVENT_LOOP(ATGL_EVENT *ev)
{
    if (ev->type == ATGL_EVT_KEY_DOWN && ev->key.keycode == KEY_ESC) {
        ATGL_QUIT();
        return;
    }
    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

void ATGL_GRAPHICS_LOOP(U32 ticks)
{
    static U32 last_tick = 0;
    if (ticks - last_tick >= 1000) { // Refresh every 1 second
        // refresh_tasks();
        last_tick = ticks;
    }
    ATGL_RENDER_TREE(ATGL_GET_SCREEN_ROOT_NODE());
}
