#include "graphics.h"
#include "types.h"

byte framebuffer[DISPLAYLIST_BLOCK_SIZE];
ImageData image = { {0, 0, 0, 0}, framebuffer };

void setGraphicsMode(byte mode) {}
void saveCurrentGraphicsState(void) {}
void restoreGraphicsState(void) {}
void clrscr(void) {}
