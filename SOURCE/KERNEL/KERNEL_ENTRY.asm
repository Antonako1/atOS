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
;     2025/08/25 - Antonako1
;         Fixed GDT and IDT initialization
; REMARKS
;     None


[BITS 16]
[ORG 0x1000:0x0000]

start:
    mov [drive_number], dl

    mov si, msg_greeting_1
    call PRINTLN

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

    ; %%%%%%%%%%%%%%%%%%%%%%%%%%%
    ; VESA/VBE initialization
    ; %%%%%%%%%%%%%%%%%%%%%%%%%%%
    ; Get VESA controller info
    mov ax, VESA_LOAD_SEGMENT
    mov es, ax
    mov di, VESA_LOAD_OFFSET
    mov ax, 4F00h
    int 10h
    cmp al, 4Fh
    jne VESA_ERROR2
    cmp ah, 0
    jne VESA_ERROR1

    ; Set VBE mode

    ; Get VBE mode info
    mov cx, VESA_TARGET_MODE
    mov ax, VESA_LOAD_SEGMENT
    mov es, ax
    mov di, VBE_MODE_OFFSET
    mov ax, 4F01h
    int 10h
    cmp al, 4Fh
    jne VESA_ERROR2
    cmp ah, 0
    jne VESA_ERROR1

    ; mov ax, 4F02h              ; VBE: Set VBE Mode
    ; mov bx, 0x4000 | VESA_TARGET_MODE  ; Bit 14 = 1, mode = 0x117
    ; int 10h
    ; cmp al, 4Fh
    ; jne VESA_ERROR2              ; Check for error
    ; cmp ah, 0
    ; jne VESA_ERROR1              ; Check for error

    ; mov ax, 4F03h
    ; int 10h
    ; cmp al, 4Fh
    ; jne VESA_ERROR2
    ; cmp ah, 0
    ; jne VESA_ERROR1

    ; ; BX now has mode + flags
    ; mov dx, bx
    ; and dx, 1FFFh          ; keep only bits 0–12 = pure mode number
    ; cmp dx, VESA_TARGET_MODE
    ; jne VESA_ERROR1


    ;%%%%%%%%%%%%%%%%%%%%%%%%%
    ; Set up memory
    ;%%%%%%%%%%%%%%%%%%%%%%%%%
    mov byte [E820_present], 0
    xor ebx, ebx
    mov word [num_of_e820_entries], 0

    mov word [e820_entries_ptr], E820_ENTRY_OFFSET
    mov word [e820_entries_ptr+2], E820_ENTRY_SEGMENT

    xor di, di
    mov es, di
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

    pusha
    call PRINT__
    popa

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
    call PRINT_DEC
    call PRINT__
    call PRINT_LINEFEED



    ; reset E820 entry pointer
    mov word [e820_entries_ptr], E820_ENTRY_OFFSET
    mov word [e820_entries_ptr+2], E820_ENTRY_SEGMENT




;     xor ecx, ecx
;     mov cx, [num_of_e820_entries]

; .loop_print_e820:
;     ; --------------------------------------
;     ; 1. Calculate linear address of entry
;     ; --------------------------------------
;     mov bx, [e820_entries_ptr]         ; Offset
;     mov ax, [e820_entries_ptr+2]       ; Segment
;     shl eax, 4                         ; Segment * 16
;     add eax, ebx                       ; Final linear address
    
;     ; --------------------------------------
;     push eax
;     mov ebx, eax                      ; Copy for splitting
;     mov eax, [ebx+16]                ; Type field
;     call PRINT_HEX
;     call PRINT_LINEFEED
;     pop eax
;     ; --------------------------------------
;     ; 3. Advance to next entry
;     add eax, MEM_STRUCT_SIZE
;     mov ebx, eax
;     and bx, 0x000F                     ; offset
;     shr eax, 4                         ; segment
;     mov [e820_entries_ptr], bx
;     mov [e820_entries_ptr+2], ax

;     loop .loop_print_e820              ; dec cx / jnz loop

;     ; reset E820 entry pointer
;     mov word [e820_entries_ptr], E820_ENTRY_OFFSET
;     mov word [e820_entries_ptr+2], E820_ENTRY_SEGMENT

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
    ;
    ; Logic copied from bootloader. Status saved in krnl_status

    ;mov ebx, [num_of_e820_entries]
    ;mov ecx, MEM_STRUCT_SIZE
    ;mov edx, E820_ENTRY_OFFSET
    ;call CALCULATE_KRNL_SEGMENT
    ;jc MEM_ERROR5
    ;call PRINT_HEX
    ;call PRINT_LINEFEED

    mov byte [krnl_status], 0

    mov ax, PVD_OFFSET
    mov cx, 1
    mov bx, BUFFER_OFFSET
    mov dx, BUFFER_SEGMENT
    call READ_DISK
    cmp eax, 0
    jne DISK_ERROR1

    
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    xor eax, eax
    mov al, byte [si]
    cmp al, 0x01
    jne ISO_ERROR1

    ; Read lba and size of root dir
    ; 156: Root directory record
    ; +2 LBA location
    ; +10 Data length
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156+10
    mov ax, word [si]
    mov ax, word [si+2]
    mov [extentLengthLE+2], ax
    mov ax, word [si]
    mov [extentLengthLE], ax

    cmp word [extentLengthLE], 0x800
    jb ISO_ERROR2

    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, 156+2
    mov ax, word [si]
    mov [extentLocationLE_LBA], ax
    mov ax, word [si+2]
    mov [extentLocationLE_LBA+2], ax

    cmp word [extentLocationLE_LBA], 29
    jb ISO_ERROR3

    ; load the root directory record
    mov eax, [extentLocationLE_LBA] ;lba
    mov cx, 1                   ; 1 sector
    mov bx, BUFFER_OFFSET
    mov dx, BUFFER_SEGMENT      ;dx:bx=buffer
    call READ_DISK              
    cmp ax, 0
    jne DISK_ERROR1

    mov dword [offset_var], 0

.main_loop_start:
;     ;2.     while(offset_var < extentLengthLE)
    mov ax, [offset_var]
    mov bx, [extentLengthLE]
    cmp ax, bx
    jge .main_loop_end
;     ;3.     if ((record)(buffer_var+offset_var).length == 0) jmp ISO_ERROR
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, [offset_var]
    mov al, byte [si]
    cmp al, 0
    je ISO_ERROR
;     ;5.     if(!((record)(buffer_var+offset_var).fileFlags & 0b00000010)) 
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    add si, [offset_var]
    add si, 25
    mov al, byte [si]
    and al, 0b00000010
    jne .to_loop_increment

;     ;4.     strncopy(name_buf, record->fileIdentifier, record->fileNameLength);
    mov ax, BUFFER_SEGMENT
    mov es, ax
    mov di, BUFFER_OFFSET

    add di, [offset_var]
    add di, 32                  ; skip to filename length
    xor cx, cx
    mov cl, byte [di]           ; string length
    push cx
    inc di                   ; skip length byte

    pusha
    mov si, di
    call PRINTNLN
    popa
;     if (strcmp(name_buf, "KRNL.BIN") == 0)
        mov si, krnl_str    ;source1
        mov cx, KRNL_STR_LEN;source1 length
        call strncmp
        cmp ax, 1
        jne .to_loop_increment
        
        ; TODO: Load values to save at extenLocationLE_LBA_KRNL and extentLengthLE_KRNL
        mov ax, BUFFER_SEGMENT
        mov ds, ax
        add si, [offset_var]
        add si, 2
        mov ax, word [si]   ; lba
        mov [extenLocationLE_LBA_KRNL], ax
        mov ax, word [si+2]
        mov [extenLocationLE_LBA_KRNL+2], ax
        add si, 10
        mov ax, word [si]   ; length
        mov [extentLengthLE_KRNL], ax
        mov ax, word [si+2]
        mov [extentLengthLE_KRNL+2], ax

        mov byte [krnl_status], 1
        jmp KRNL_TO_MEMORY

.to_loop_increment:
    ;11.    If nothing happens: offset_var += (record)(buffer_var+offset_var).length
    mov ax, BUFFER_SEGMENT
    mov ds, ax
    mov si, BUFFER_OFFSET
    xor ax, ax
    add si, [offset_var]
    mov al, byte [si]   ; .length

    add ax, [offset_var]
    mov [offset_var], ax
    
    jmp .main_loop_start
.main_loop_end:




KRNL_TO_MEMORY:
    cmp byte [krnl_status], 1
    je .kernel_found
    jmp KRNL_NOT_FOUND



.kernel_found:
    ; Read the kernel from disk to memory
    mov ax, [extentLengthLE]
    ; If kernel length > 64KB, use 32-bit registers
    ; Add 511 and divide by 512 (sector size)
    mov eax, [extentLengthLE]
    add eax, 511
    shr eax, 9
    mov cx, ax ; sectors to read
    xor eax, eax
    mov eax, [extenLocationLE_LBA_KRNL]

    ; offset = bx
    mov bx, KERNEL_LOAD_OFFSET
    ; segment = dx
    mov dx, KERNEL_LOAD_SEGMENT
    call READ_DISK
    cmp eax, 0
    jne DISK_ERROR1


    pusha
    mov si, msg_kernel_end
    call PRINTLN
    popa 

START_32BIT_PROTECTED_MODE:
    cli 
    ; --- Load GDT ---
    lgdt [gdtr]

    ; --- Enter protected mode ---
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 1           ; set PE bit
    mov cr0, eax

    ; --- Far jump to reload CS with protected mode selector ---
    jmp 08h:PModeMain       ; 08h = code segment in GDT

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
PModeMain:
    ; --- Load IDT ---
    lidt [IDTR]



    mov ax, 0x10     ; data segment selector (GDT entry #2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax       ; stack segment

    cli
    mov esp, 0x90000      ; Stack at 576KB (adjust if you want)

    ; Jump to kernel at 0x2000 (flat)
    jmp KERNEL_LOAD_ADDRESS

    jmp hang32
hang32:
    ;print q into video memory
    jmp $
