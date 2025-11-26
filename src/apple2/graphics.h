#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

// Stub definitions for Apple II
#define DISPLAYLIST_BLOCK_SIZE 1024
extern byte framebuffer[];

void setGraphicsMode(byte mode);
void saveCurrentGraphicsState(void);
void restoreGraphicsState(void);
void clrscr(void);

#endif
