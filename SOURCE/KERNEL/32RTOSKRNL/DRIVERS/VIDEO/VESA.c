/*+++
    Source/KERNEL/32RTOSKRNL/DRIVERS/VIDEO/VESA.c - VESA implementation

    Part of atOS-RT

    Licensed under the MIT License. See LICENSE file in the project root for full license information.

DESCRIPTION
    VESA (Video Electronics Standards Association) implementation for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/08/18 - Antonako1
        Initial version. Contains VESA info display function.

REMARKS
    None

DESCRIPTION
    VESA (Video Electronics Standards Association) implementation for atOS-RT.

AUTHORS
    Antonako1

REVISION HISTORY
    2025/05/26 - Antonako1
        Initial version. Contains VESA info display function.

REMARKS
    None
---*/

#include "./VESA.h"


BOOL vesa_check(void) {
    VESA_INFO* vesa = (VESA_INFO*)(VESA_LOAD_ADDRESS_PHYS);
    if (vesa->Signature[0] != 'V' || vesa->Signature[1] != 'E' ||
        vesa->Signature[2] != 'S' || vesa->Signature[3] != 'A') {
        return FALSE;
    }
    
    if (vesa->Version < 0x0200) {
        return FALSE;
    }
    
    U16 far_off_ptr = (U16)(vesa->VideoModePtr & 0xFFFF);
    U16 far_seg_ptr = (U16)((vesa->VideoModePtr >> 16) & 0xFFFF);
    U32 linear_addr = RM2LA(far_seg_ptr, far_off_ptr);
    U16 *mode_list = (U16*)linear_addr;
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        if(mode_list[i] == VESA_TARGET_MODE) {
            return TRUE;
            break;
        }
    }
    return FALSE;
}