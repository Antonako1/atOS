; KERNEL\KERNEL_ENTRY.asm - Kernel entry point
;      
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     16-bit kernel entry point. 
;     This file will load KRNL.BIN to memory, switch to 32-bit protected mode and jump to KRNL.BIN
;
; AUTHORS
;     Antonako1
; 
; REVISION HISTORY
;     2025/02/09 - Antonako1
;         Initial version. Prints a welcome message.
;     2025/5/11  - Antonako1
;         Transforms into 32-bit mode, sets up idt and gdtr
;     2025/5/15  - Antonako1
;         Sets up memory using E820h
;     2025/5/21  - Antonako1
;         Jumps to KRNL.BIN;1
;     2025/08/18 - Antonako1
;         Contains VESA info display function.
;         VESA initialization and mode setting.
;         Reads E820 memory map and stores it in a table.
;         Initializes GDT and IDT into memory correctly
;     2025/08/27 - Antonako1
;         Fixed a bug with kernel load address
;
; REMARKS
;     None

%define VBE_ACTIVATE 1
[BITS 16]
[ORG 0x2000]

start:
    cli
    mov [drive_number], dl


    ; Set up the stack
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Set up the data segment
    mov ax, 0x0000
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    cli

    mov si, msg_greeting_1
    call PRINTLN

    ;%%%%%%%%%%%%%%%%%%%%%%%%%
    ; Set up memory
    ;%%%%%%%%%%%%%%%%%%%%%%%%%
    mov byte [E820_present], 0
    xor ebx, ebx
    mov word [num_of_e820_entries], 0

    mov word [e820_entries_ptr], E820_ENTRY_OFFSET
    mov word [e820_entries_ptr+2], E820_ENTRY_SEGMENT

.do_mem:
    mov ax, E820R
    shr ax, 4
    mov es, ax

    mov di, E820R
    and di, 0x0F


    mov ecx, MEM_STRUCT_SIZE
    mov edx, SMAP
    mov eax, 0xE820
    int 15h
    jc MEM_ERROR1


    ; Compares "SMAP" to EAX
    cmp eax, SMAP
    jne MEM_ERROR2
    
    ; if ecx < MEM_STRUCT_SIZE goto: MEM_ERROR3
    cmp ecx, MEM_STRUCT_SIZE
    jb MEM_ERROR3
    ; IF ecx > MEM_STRUCT_SIZE goto: MEM_ERROR4
    cmp ecx, MEM_STRUCT_SIZE_MAX
    ja MEM_ERROR4

    ; if ecx == MEM_STRUCT_SIZE_MAX; ACPI entry
    ; TODO: ACPI handlings
    
    mov byte [E820_present], 1  ; E820 is present


    ;-----------------------------------
    ; Store entry from mem_buf into E820 table
    ;-----------------------------------
    pusha
    mov eax, [E820R+16]   ; Type
    call PRINT_HEX
    call PRINT_LINEFEED
    popa

;    cmp eax, E820_TYPE_ARM
;    je .ARM
;    cmp eax, E820_TYPE_ARR
;    je .ARR
;    cmp eax, E820_TYPE_ACPI
;    je .ACPI
;    cmp eax, E820_TYPE_NVS
;    je .NVS
;    cmp eax, E820_TYPE_ARU
;    je .ARU
;.ARM:
;.ARR:
;.ACPI:
;.NVS:
;.ARU:

    ; ------------------------------------
    ; 1. Calculate E820 entry linear address
    ; ------------------------------------
    push ebx
    mov bx, [e820_entries_ptr]         ; Offset
    mov ax, [e820_entries_ptr+2]       ; Segment
    shl eax, 4                         ; Segment * 16
    add eax, ebx                       ; Final linear address

    ;mov bx, [e820_entries_ptr]         ; Offset
    ;mov ax, [e820_entries_ptr+2]       ; Segment
    ;shl eax, 4
    ;add eax, ebx                       ; eax = linear address

    ; ------------------------------------
    ; 2. Store the E820 entry at [eax]
    ; ------------------------------------
    
    xor edx, edx
    mov edx, [E820R]                  
    mov [eax], edx                    ; BaseAddressLow
    mov edx, [E820R+4]
    mov [eax+4], edx                  ; BaseAddressHigh
    mov edx, [E820R+8]
    mov [eax+8], edx                  ; LengthLow
    mov edx, [E820R+12]
    mov [eax+12], edx                 ; LengthHigh
    mov edx, dword [E820R+16]
    mov [eax+16], edx                 ; Type

%ifdef E820_ACPI
    mov edx, [E820R+20]
    mov [eax+20], edx                 ; ACPI (optional)
%endif


    ; ------------------------------------
    ; 3. Advance e820_entries_ptr (segment:offset)
    ; ------------------------------------
    add eax, MEM_STRUCT_SIZE          ; move to next slot
    mov ebx, eax                      ; Copy for splitting
    and bx, 0x000F                    ; new offset = linear & 0xF
    shr eax, 4                        ; new segment = linear >> 4
    mov [e820_entries_ptr], bx        ; Store new offset
    mov [e820_entries_ptr+2], ax      ; Store new segment
    
    pop ebx

    inc word [num_of_e820_entries]

    
    ; while ebx != 0 goto: .do_mem
    cmp ebx, 0
    jne .do_mem
;%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


    ; If E820 is not present, use AX=E801h
    cmp byte [E820_present], 1
    jne .TRY_E801h

    ; Print the E820 entries
    mov ax, word [num_of_e820_entries]
    call PRINT_HEX
    ; call PRINT__
    call PRINT_LINEFEED



    ; reset E820 entry pointer
    mov word [e820_entries_ptr], E820_ENTRY_OFFSET
    mov word [e820_entries_ptr+2], E820_ENTRY_SEGMENT

    mov si, msg_e820_done
    call PRINTLN
    
    jmp .FIND_KERNEL

.TRY_E801h:
    ;TODO.
    ; If E801h is not present, use AH=88h
    cmp byte [E801_present], 0
    jne .FIND_KERNEL
.TRY_AH88h:
    ;TODO.


.FIND_KERNEL:
    ; Read KRNL.BIN from disk, load it into memory and jump to it
    mov byte [krnl_status], 0

    ; Read Primary Volume Descriptor (sector 16, 2048 bytes)
    mov ax, PVD_OFFSET
    mov cx, 1
    mov bx, BUFFER_OFFSET
    mov dx, BUFFER_SEGMENT
    call READ_DISK
    cmp eax, 0
    jne DISK_ERROR1

    ; Check "CD001" signature at bytes 1..5
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    mov al, [si+1]
    cmp al, 0x43         ; 'C'
    jne ISO_ERROR1
    mov al, [si+2]
    cmp al, 0x44         ; 'D'
    jne ISO_ERROR1
    mov al, [si+3]
    cmp al, 0x30         ; '0'
    jne ISO_ERROR1
    mov al, [si+4]
    cmp al, 0x30         ; '0'
    jne ISO_ERROR1
    mov al, [si+5]
    cmp al, 0x31         ; '1'
    jne ISO_ERROR1
    pop ds

    ; Read root directory record fields from PVD (byte 156)
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156+10
    mov eax, dword [si]             ; extentLengthLE (bytes)
    pop ds
    mov [extentLengthLE], eax
    cmp dword [extentLengthLE], 0x800
    jb ISO_ERROR2

    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156+2
    mov eax, dword [si]             ; extentLocationLE (LBA, 2048-byte blocks)
    pop ds
    mov dword [extentLocationLE_LBA], eax
    cmp dword [extentLocationLE_LBA], 29
    jb ISO_ERROR3

    ; Load first sector of root directory (2048B)
    push ds
    push es
    mov eax, dword [extentLocationLE_LBA]
    mov cx, 1
    mov bx, BUFFER_OFFSET
    mov dx, BUFFER_SEGMENT
    call READ_DISK
    cmp eax, 0
    pop es
    pop ds
    jne DISK_ERROR1

    mov dword [offset_var], 0

.main_loop_start:
    ; while (offset_var < extentLengthLE)
    mov eax, [offset_var]
    mov ebx, [extentLengthLE]
    cmp eax, ebx
    jge .main_loop_end

    ; record length byte
    mov ecx, [offset_var]
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, cx
    movzx eax, byte [si]           ; record->length
    pop ds

    test eax, eax
    jz .advance_to_next_sector     ; zero-length = padding to end of sector

.have_record:
    ; if (fileFlags & 0x02) -> directory, skip
    mov ecx, [offset_var]
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, cx
    mov al, byte [si+25]
    pop ds
    test al, 00000010b
    jnz .to_loop_increment

    ; Extract filename
    mov ecx, [offset_var]
    push ds
    push es

    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, cx           ; si -> record start
    add si, 33          ; si -> fileIdentifier
    mov cx, KRNL_STR_LEN
    mov ax, cs
    mov es, ax
    lea di, name_buf
    call strncpy   ; copy filename to name_buf
    
    pop es
    pop ds

    ; push ds
    ; mov ax, cs
    ; mov ds, ax
    ; mov cx, KRNL_STR_LEN
    ; lea si, name_buf
    ; call PRINTNLN
    ; lea si, krnl_str
    ; call PRINTNLN
    ; pop ds
    
    ; Compare to "KRNL.BIN;1"
    push ds
    push es
    mov ax, cs
    mov ds, ax
    mov ax, cs
    mov es, ax
    mov cx, 8
    dec cx
    lea si, krnl_str
    lea di, name_buf
    call strncmp                   ; AX==1 (as per your convention) if equal
    pop es
    pop ds
    cmp ax, 1
    jne .to_loop_increment


    ; Matched: capture LBA and length for KRNL
    mov ecx, [offset_var]
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, cx

    mov ecx, dword [si+2]          ; extentLocationLE_LBA (2048B blocks)
    mov edx, dword [si+10]         ; extentLengthLE (bytes)
    pop ds

    mov [extentLocationLE_LBA_KRNL], ecx
    mov [extentLengthLE_KRNL], edx

    mov byte [krnl_status], 1
    jmp KRNL_TO_MEMORY

.to_loop_increment:
    ; offset_var += record->length
    mov ecx, [offset_var]
    push ds
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, cx
    movzx eax, byte [si]           ; length
    pop ds
    add eax, ecx
    mov [offset_var], eax
    jmp .main_loop_start

.advance_to_next_sector:
    ; Jump to next 2048-byte boundary (skip padding)
    mov eax, [offset_var]
    add eax, SECTOR_SIZE-1
    and eax, ~(SECTOR_SIZE-1)
    mov [offset_var], eax
    jmp .main_loop_start

.main_loop_end:
    ; not found in this sector; if directory spans multiple sectors, read next
    ; Calculate next LBA within root directory if any remains
    ; bytes_consumed = min(extentLengthLE, SECTOR_SIZE), for simplicity we only scanned 1 sector buffer.
    ; In a full implementation, you'd iterate all sectors. Here: fall through to not found.
    jmp KRNL_NOT_FOUND


KRNL_TO_MEMORY:
    cmp byte [krnl_status], 1
    jne KRNL_NOT_FOUND
    jmp kernel_found


; Not optimal, but otherwise corrupts stack...
; reads via DAP, DS:SI must point to DAP in our data/code segment
READ_DISK_SAFE:
    mov byte  [DAP], DAP_SIZE
    mov byte  [DAP+1], 0
    mov word  [DAP+2], cx
    mov word  [DAP+4], bx
    mov word  [DAP+6], dx
    mov dword [DAP+8], eax
    mov dword [DAP+12], 0

    push ds
    mov ax, cs
    mov ds, ax
    lea si, [DAP]
    mov ah, 42h
    mov dl, [drive_number]
    int 13h
    pop ds
    jc DISK_ERROR1
    jmp read_done_for_kernel


kernel_found:
    ; sectors = ceil(length / 2048)
    mov eax, [extentLengthLE_KRNL]
    add eax, SECTOR_SIZE-1
    shr eax, 11                    ; /2048
    mov ecx, eax                   ; sector count (2048B sectors)
    mov eax, [extentLocationLE_LBA_KRNL]
    mov bx, KERNEL_LOAD_OFFSET
    mov dx, KERNEL_LOAD_SEGMENT
    jmp READ_DISK_SAFE

read_done_for_kernel:
    mov si, msg_kernel_end
    call PRINTLN

    ; %%%%%%%%%%%%%%%%%%%%%%%%%%%
    ; VESA/VBE initialization
    ; %%%%%%%%%%%%%%%%%%%%%%%%%%%
    ; Get VESA controller info
    push es
    mov ax, VESA_LOAD_SEGMENT
    mov es, ax
    mov di, VESA_LOAD_OFFSET
    mov ax, 4F00h
    int 10h
    pop es
    cmp al, 4Fh
    jne VESA_ERROR2
    cmp ah, 0
    jne VESA_ERROR1
    ; Set VBE mode

%ifdef VBE_ACTIVATE
    ; Get VBE mode info
    push es
    mov cx, VESA_TARGET_MODE
    mov ax, VESA_LOAD_SEGMENT
    mov es, ax
    mov di, VBE_MODE_OFFSET
    mov ax, 4F01h
    int 10h
    pop es
    cmp al, 4Fh
    jne VESA_ERROR2
    cmp ah, 0
    jne VESA_ERROR1

    mov ax, 4F02h              ; VBE: Set VBE Mode
    mov bx, 0x4000 | VESA_TARGET_MODE  ; Bit 14 = 1, mode = 0x118
    int 10h
    cmp al, 4Fh
    jne VESA_ERROR2              ; Check for error
    cmp ah, 0
    jne VESA_ERROR1              ; Check for error

    mov ax, 4F03h
    int 10h
    cmp al, 4Fh
    jne VESA_ERROR2
    cmp ah, 0
    jne VESA_ERROR1


    ; BX now has mode + flags
    mov dx, bx
    and dx, 1FFFh          ; keep only bits 0–12 = pure mode number
    cmp dx, VESA_TARGET_MODE
    jne VESA_ERROR1
%endif ; VBE_ACTIVATE

START_32BIT_PROTECTED_MODE:
    cli
    ; --- Load GDT ---
build_gdtr:
    ; limit
    mov word [gdtr], gdt_end - gdt_start - 1

    ; base = CS << 4 + offset(gdt_start)
    xor eax, eax
    mov ax, cs
    shl eax, 4
    mov bx, gdt_start        ; 16-bit offset is fine if within segment
    add eax, ebx
    mov [gdtr+2], eax        ; write 32-bit linear base

    lgdt [gdtr]

    ; --- Enable A20 line ---
    in al, 0x92
    or al, 00000010b
    out 0x92, al

    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 1           ; set PE bit
    mov cr0, eax

    ; --- Far jump to reload CS with protected mode selector ---
    jmp dword 08h:PModeMain       ; 08h = code segment in GDT

hang:
    mov si, msg_hng_1
    call PRINTLN
    jmp $

MEM_ERROR1:
    mov si, msg_mem_err1
    call PRINTLN
    jmp hang
MEM_ERROR2:
    mov si, msg_mem_err2
    call PRINTLN
    jmp hang
MEM_ERROR3:
    mov si, msg_mem_err3
    call PRINTLN
    jmp hang
MEM_ERROR4:
    mov si, msg_mem_err4
    call PRINTLN
    jmp hang
MEM_ERROR5:
    mov si, msg_mem_err5
    call PRINTLN
    jmp hang
MEM_ERROR6:
    mov si, msg_mem_err6
    call PRINTLN
    jmp hang
DISK_ERROR1:
    mov si, msg_disk_err1
    call PRINTLN
    jmp hang
ISO_ERROR1:
    mov si, msg_iso_err1
    call PRINTLN
    jmp hang
ISO_ERROR2:
    mov si, msg_iso_err2
    call PRINTLN
    jmp hang
ISO_ERROR3:
    mov si, msg_iso_err3
    call PRINTLN
    jmp hang
ISO_ERROR:
    mov si, msg_iso_errg
    call PRINTLN
    jmp hang
KRNL_NOT_FOUND:
    mov si, msg_krnl_not_found
    call PRINTLN
    jmp hang
VESA_ERROR1:
    mov si, msg_vesa_err1
    call PRINTLN
    jmp hang
VESA_ERROR2:
    mov si, msg_vesa_err2
    call PRINTLN
    jmp hang

%include "SOURCE/KERNEL/16-BIT-BIOS/KERNEL_ENTRY_DATA.inc"
%include "SOURCE/KERNEL/16-BIT-BIOS/BIOS.inc"


; Protected mode main 
[BITS 32]
isr_stub:
    cli                 ; disable interrupts immediately
halt_loop:
    hlt
    jmp halt_loop


PModeMain:
    cli
    mov ax, 0x10     ; data segment selector (GDT entry #2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax       ; stack segment
    mov esp, STACK_ADDR
; optionally set EBP too
    mov ebp, esp

    ; --- Load IDT ---
    %define KCODE_SEL 0x08
    mov ecx, 256               ; loop through all vectors
    mov ebx, isr_stub          ; ISR address
    lea edi, [IDT]             ; start of IDT

    mov ecx, 256
    mov edi, IDT
.fill_loop:
    mov ebx, isr_stub
    mov ax, bx
    mov [edi], ax
    shr ebx, 16
    mov ax, bx
    mov [edi+6], ax
    mov word [edi+2], KCODE_SEL
    mov byte [edi+4], 0
    mov byte [edi+5], 0x8E
    add edi, 8
    dec ecx
    jnz .fill_loop


    lidt [IDTR]

    cld
    ; mov edi, 0xB8000
    ; mov eax, (' ' | (0x1A << 8))  ; space with attribute 0x1A
    ; mov ecx, 80*25/2              ; number of dwords to fill (2 chars per dword)
    ; rep stosd
    ; mov dword [0xB8000], ('H' | (0x1A << 8))


    jmp KERNEL_LOAD_ADDRESS
    hlt    

    jmp hang32
hang32:
    ;print q into video memory
    jmp hang32