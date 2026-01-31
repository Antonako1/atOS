#include <LIBRARIES/ATGL/ATGL_NODES.h>
#include <LIBRARIES/ATGL/ATGL_GRAPHICS.h>
#include <STD/MEM.h>
#include <STD/STRING.h>
#include <STD/BINARY.h>
#include <STD/GRAPHICS.h>

VOID ATGL_FILL_RECT(ATGL_RECT rect, VBE_COLOUR colour) {
    // Clip to screen bounds
    if(rect.x < 0) rect.x = 0;
    if(rect.y < 0) rect.y = 0;
    if(rect.x + rect.width > SCREEN_WIDTH) rect.width = SCREEN_WIDTH - rect.x;
    if(rect.y + rect.height > SCREEN_HEIGHT) rect.height = SCREEN_HEIGHT - rect.y;

    for(I32 y = rect.y; y < rect.y + rect.height; y++) {
        for(I32 x = rect.x; x < rect.x + rect.width; x++) {
            DRAW_PIXEL((VBE_PIXEL_INFO){
                .Colour = colour,
                .X = x,
                .Y = y,
            });
        }
    }
}