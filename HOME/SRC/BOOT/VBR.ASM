; HOME/SRC/BOOT/VBR.ASM - FAT32 Volume Boot Record
;
; Licensed under the MIT License. See LICENSE file in the project root for full license information.
;
; DESCRIPTION
;     FAT32 Volume Boot Record.
;     Reads VBR.BIN from the root directory of a FAT32 partition,
;     loads it to 0x0000:0x2000 and jumps to it.
;
; LAYOUT (512-byte boot sector)
;     Bytes   0-2   : EB 58 90  (JMP SHORT to code_start + NOP)
;     Bytes   3-89  : FAT32 BPB  — filled in by mkfs.fat / disk formatter
;     Bytes  90-509 : Boot code and data
;     Bytes 510-511 : 0x55 0xAA boot signature
;
; AUTHORS
;     Antonako1
;
; REVISION HISTORY
;     2026/04/04 - Rewritten for FAT32

.use16
.org 0x7C00
.code

; ── Load target ───────────────────────────────────────────────────────────────
#define LOAD_SEG    0x0000
#define LOAD_OFF    0x2000

; ── Buffer segment for FAT / directory reads (0x0500:0000 = 0x5000 physical) ─
#define IO_SEG      0x0500

; ── FAT32 BPB field absolute addresses (DS = 0x0000, VBR loaded at 0x7C00) ───
#define BPB_SECT_PER_CLUST  0x7C0D   ; byte  - sectors per cluster
#define BPB_RESERVED        0x7C0E   ; word  - reserved sectors (FAT region start)
#define BPB_NUM_FATS        0x7C10   ; byte  - number of FATs (typically 2)
#define BPB_HIDDEN_SECTS    0x7C1C   ; dword - hidden sectors = partition start LBA
#define BPB_FAT_SIZE32      0x7C24   ; dword - sectors per FAT
#define BPB_ROOT_CLUSTER    0x7C2C   ; dword - first cluster of root directory

; ── FAT32 VBR header ─────────────────────────────────────────────────────────
; Byte 0-2  : EB 58 90  →  JMP SHORT to code_start (+0x5A from VBR base) + NOP
; Byte 3-89 : FAT32 BPB (87 bytes) — overwritten by mkfs.fat

; ─────────────────────────────────────────────────────────────────────────────
;  code_start  (byte offset 0x5A = 90 from the VBR base = 0x7C5A physical)
; ─────────────────────────────────────────────────────────────────────────────
code_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [drive_num], dl          ; BIOS passes drive number in DL

    ; ── fat_lba = hidden_sectors + reserved_sectors ───────────────────────────
    mov ax, [BPB_HIDDEN_SECTS]
    mov dx, [BPB_HIDDEN_SECTS + 2]   ; DX:AX = partition start LBA
    xor bx, bx
    mov bx, [BPB_RESERVED]           ; zero-extended word
    add ax, bx
    adc dx, 0
    mov [fat_lba],     ax
    mov [fat_lba + 2], dx

    ; ── data_lba = fat_lba + num_fats * fat_size32 ───────────────────────────
    xor ax, ax
    xor dx, dx
    xor bx, bx
    mov bl, [BPB_NUM_FATS]
@@accum_fats:
    add ax, [BPB_FAT_SIZE32]
    adc dx, [BPB_FAT_SIZE32 + 2]
    dec bl
    jnz @@accum_fats
    add ax, [fat_lba]
    adc dx, [fat_lba + 2]
    mov [data_lba],     ax
    mov [data_lba + 2], dx

    ; ── root_lba = data_lba + (root_cluster - 2) * spc ───────────────────────
    mov ax, [BPB_ROOT_CLUSTER]
    mov dx, [BPB_ROOT_CLUSTER + 2]   ; DX:AX = root cluster number
    sub ax, 2
    sbb dx, 0
    xor bx, bx
    mov bl, [BPB_SECT_PER_CLUST]
    mul bx                           ; DX:AX = (root_cluster - 2) * spc
    add ax, [data_lba]
    adc dx, [data_lba + 2]
    mov [root_lba],     ax
    mov [root_lba + 2], dx

    ; ── Load root directory cluster into IO_SEG:0000 (0x5000 physical) ───────
    mov ax, [root_lba]
    mov dx, [root_lba + 2]
    xor bx, bx
    mov bl, [BPB_SECT_PER_CLUST]
    mov cx, bx
    mov bx, IO_SEG
    call read_disk

    ; ── dir_size = spc * 512 (bytes of directory data loaded) ────────────────
    xor bx, bx
    mov bl, [BPB_SECT_PER_CLUST]
    shl bx, 9                        ; BX = spc * 512
    mov [dir_size], bx

    ; ── Search root directory for 8.3 filename "VBR     BIN" ─────────────────
    xor ax, ax
    mov [dir_off], ax

@@dir_loop:
    mov bx, [dir_off]
    cmp bx, [dir_size]
    jge @@not_found

    ; Directory entry at physical address 0x5000 + bx  (DS=0, IO_SEG*16=0x5000)
    mov si, 0x5000
    add si, bx
    mov [entry_ptr], si              ; save for after cmpsb modifies SI

    mov al, [si]
    cmp al, 0x00
    je @@not_found                   ; 0x00 = end-of-directory marker

    mov al, [si + 11]                ; attribute byte
    cmp al, 0x0F
    je @@dir_next                    ; skip LFN entries
    test al, 0x10
    jnz @@dir_next                   ; skip subdirectories

    ; Compare 11-byte 8.3 name: DS:SI = entry, ES:DI = target_name (ES = 0)
    lea di, target_name
    mov cx, 11
    repe
    cmpsb
    je @@file_found

@@dir_next:
    add word [dir_off], 32
    jmp @@dir_loop

@@not_found:
    mov si, err_msg
    call print_str
    jmp $

@@file_found:
    mov si, [entry_ptr]
    mov ax, [si + 26]                ; start cluster low  (FAT32 dir entry +26)
    mov dx, [si + 20]                ; start cluster high (FAT32 dir entry +20)

    ; ── file_lba = data_lba + (cluster - 2) * spc ────────────────────────────
    sub ax, 2
    sbb dx, 0
    xor bx, bx
    mov bl, [BPB_SECT_PER_CLUST]
    mul bx                           ; DX:AX = (cluster - 2) * spc
    add ax, [data_lba]
    adc dx, [data_lba + 2]

    ; ── Load one cluster into LOAD_SEG:LOAD_OFF ───────────────────────────────
    ; (Covers files ≤ one cluster; a full FAT chain walk belongs in stage 2.)
    xor bx, bx
    mov bl, [BPB_SECT_PER_CLUST]
    mov cx, bx
    mov bx, LOAD_SEG
    call read_disk

    mov dl, [drive_num]              ; pass drive number to loaded code
    jmp far LOAD_SEG:LOAD_OFF

; ─────────────────────────────────────────────────────────────────────────────
;  read_disk — INT 13h extended read (LBA)
;  In:  DX:AX = 32-bit absolute LBA
;       CX    = sector count
;       BX    = destination segment  (offset 0x0000)
; ─────────────────────────────────────────────────────────────────────────────
read_disk:
    push ax
    push bx
    push cx
    push dx
    push si

    mov si, dap
    mov byte [si + 0],  0x10
    mov byte [si + 1],  0
    mov word [si + 2],  cx
    mov word [si + 4],  0x0000
    mov word [si + 6],  bx
    mov word [si + 8],  ax
    mov word [si + 10], dx
    mov word [si + 12], 0
    mov word [si + 14], 0

    mov ah, 0x42
    mov dl, [drive_num]
    int 0x13
    jc @@rd_err

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

@@rd_err:
    mov si, err_msg
    call print_str
    jmp $

; ─────────────────────────────────────────────────────────────────────────────
;  print_str — BIOS teletype (AH=0Eh)
;  In : SI = ASCIIZ string pointer
; ─────────────────────────────────────────────────────────────────────────────
print_str:
    lodsb
    or al, al
    jz @@ps_done
    mov ah, 0x0E
    int 0x10
    jmp print_str
@@ps_done:
    ret


; filename in 8.3 FAT format: 8-char name (space-padded) + 3-char extension
target_name  db "VKR     BIN", 11

err_msg      db "VBR_ERR", 0
drive_num    db 0

fat_lba      dd 0
data_lba     dd 0
root_lba     dd 0

dir_off      dw 0
dir_size     dw 0
entry_ptr    dw 0

; Disk Address Packet (16 bytes)
dap db 0
.times 15 db 0

; ── Pad to 510 bytes then boot signature ──────────────────────────────────────
; .times ($-$$)-510 db 0

; boot signature done by kernel
; 0xAA55
