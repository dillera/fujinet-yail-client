// Copyright (C) 2021 Brad Colbert
#include "graphics.h"
#include "console.h"
#include "files.h"
#include "utility.h"
#include "settings.h"
#include "consts.h"
#include "types.h"
#include "vbxe.h"

#include <atari.h>
#include <peekpoke.h>
#include <conio.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma data-name (push,"FRAMEBUFFER")
#include "welcome_splash.h"
#pragma data-name (pop)
#pragma optimize(push, off)
#pragma data-name (push,"GFX8_DL")
#include "graphics_8_dl.h"
//#pragma data-name (pop)
//#pragma data-name (push,"GFX8_CONSOLE_DL")
#include "graphics_8_console_dl.h"
//#pragma data-name (pop)
//#pragma data-name (push,"GFX9_CONSOLE_DL")
#include "graphics_9_console_dl.h"
#include "graphics_8_s2_dl.h"
#pragma data-name (pop)
#pragma optimize(pop)

#define FRAMEBUFFER_BlOCK_SIZE 0x1000
#define IS_LMS(x) (x & (byte)64)

//
extern char* console_buff;
extern byte console_lines;
extern Settings settings;
extern struct __vbxe* VBXE;

// Globals
void* ORG_SDLIST = 0;
void* VDSLIST_STATE = 0;
byte ORG_GPRIOR = 0x0;
byte NMI_STATE = 0x0;
byte ORG_COLOR1, ORG_COLOR2, ORG_COLOR4;
byte CURRENT_PRIOR = 0x0;  // PRIOR value for the active mode, used by the console DLI
DLDef dlDef;
ImageData image = { {0, 0, 0, 0}, framebuffer };

// DLI definitions
void disable_9_dli(void);  // prototype for below

// Enable Gfx 9
#pragma optimize(push, off)
void enable_9_dli(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    __asm__("sta %w", WSYNC);
    POKE(PRIOR, CURRENT_PRIOR);
    POKEW(VDSLST, (unsigned)disable_9_dli);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}

// Disable Gfx 9
void disable_9_dli(void) {
    __asm__("pha");
    __asm__("tya");
    __asm__("pha");
    __asm__("txa");
    __asm__("pha");
    __asm__("sta %w", WSYNC);
    POKE(PRIOR, ORG_GPRIOR);
    POKEW(VDSLST, (unsigned)enable_9_dli);
    __asm__("pla");
    __asm__("tax");
    __asm__("pla");
    __asm__("tay");
    __asm__("pla");
    __asm__("rti");
}
#pragma optimize(pop)

// Rewrite the bitmap mode lines of a display list in place.  The ANTIC E
// (Graphics 15) and ANTIC F (Graphics 8) display lists are identical apart
// from the mode nibble, so one DL serves both: walk the instructions,
// retarget $0E/$0F mode lines, skip LMS/jump operand bytes, stop at JMP/JVB.
static void retarget_dl_modes(byte* dl, byte antic_mode)
{
    while(1)
    {
        byte b = *dl;
        byte low = b & 0x0F;

        if(low == DL_JMP || low == DL_JVB)  // jump instructions end the walk
            break;

        if(low == 0x0E || low == 0x0F)
        {
            *dl = (b & 0xF0) | antic_mode;
            if(b & 0x40)   // LMS: skip the screen memory address
                dl += 2;
        }
        else if(low && (b & 0x40))  // LMS on some other mode (console text)
            dl += 2;

        ++dl;
    }
}

void saveCurrentGraphicsState(void)
{
    ORG_SDLIST = OS.sdlst;
    VDSLIST_STATE = OS.vdslst;
    //NMI_STATE = ANTIC.nmien;
    ORG_GPRIOR = OS.gprior;       // Save current priority states
    ORG_COLOR1 = OS.color1;
    ORG_COLOR2 = OS.color2;
    ORG_COLOR4 = OS.color4;
}

void restoreGraphicsState(void)
{
    ANTIC.nmien = NMIEN_VBI;
    OS.vdslst = VDSLIST_STATE;
    OS.sdlst = ORG_SDLIST;
    OS.color1 = ORG_COLOR1;
    OS.color2 = ORG_COLOR2;
    OS.color4 = ORG_COLOR4;
    OS.gprior = ORG_GPRIOR;       // restore priority states
    OS.botscr = 24;
}

void setGraphicsMode(const byte mode)
{
    makeDisplayList(mode);

    OS.color1 = 14;         // Color maximum luminance
    OS.color2 = 0;          // Background black

    if(VBXE)
        clear_vbxe(); // Clear the screen

    switch(mode)
    {
        case GRAPHICS_0:
            OS.sdlst = ORG_SDLIST;
            ANTIC.nmien = NMIEN_VBI; //NMI_STATE;
            OS.vdslst = VDSLIST_STATE;
            OS.botscr = 24;
        break;

        // makeDisplayList (called above) has already retargeted the shared
        // display lists' mode bytes for ANTIC E vs F.
        case GRAPHICS_8:
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        case GRAPHICS_15:
            OS.sdlst = &graphics_8_dl;
        break;

        case GRAPHICS_8_CONSOLE:
        case GRAPHICS_15_CONSOLE:
            OS.sdlst = &graphics_8_console_dl;
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
            OS.sdlst = &graphics_9_console_dl;
        break;

        case GRAPHICS_8_2:
        case GRAPHICS_9_2:
            OS.sdlst = &graphics_8_s2_dl;
        break;

        case GRAPHICS_20:
        case GRAPHICS_21:
            setup_VBXE();  // 320x240x256 or 640x240x16
        break;
    }

    // Set graphics mode specifc things
    switch(mode & (byte)~(GRAPHICS_CONSOLE_EN | GRAPHICS_BUFFER_TWO))
    {
        case GRAPHICS_0:
        case GRAPHICS_8:
            CURRENT_PRIOR = ORG_GPRIOR;       // Return original state of GTIA
            OS.gprior = CURRENT_PRIOR;
            OS.color4 = ORG_COLOR4;
        break;
        case GRAPHICS_9:
            CURRENT_PRIOR = ORG_GPRIOR | GFX_9;   // Enable GTIA
            OS.gprior = CURRENT_PRIOR;
            OS.color4 = 0;                    // COLBK black: pixels are gray luminances
        break;
        case GRAPHICS_10:
            CURRENT_PRIOR = ORG_GPRIOR | GFX_10;   // Enable GTIA
            OS.gprior = CURRENT_PRIOR;
            OS.color4 = 0;
        break;
        case GRAPHICS_11:
            CURRENT_PRIOR = ORG_GPRIOR | GFX_11;   // Enable GTIA
            OS.gprior = CURRENT_PRIOR;
            OS.color4 = 0x08;                 // hue 0 lum 8: mode 11 luminance comes from COLBK
        break;
        case GRAPHICS_15:
            CURRENT_PRIOR = ORG_GPRIOR;       // No GTIA: ANTIC E 4-color bitmap
            OS.gprior = CURRENT_PRIOR;
            // 4-level gray ramp; pixel %00 = COLBK, %01-%11 = PF0-PF2.
            // PF1 carries white and PF2 a dark gray so the mode 0 console
            // (text lum from PF1, background from PF2) stays readable.
            // Order must match the server's GR15_GRAYS quantization targets.
            OS.color4 = 0x00;  // %00 black
            OS.color0 = 0x06;  // %01 mid gray
            OS.color1 = 0x0E;  // %10 white
            OS.color2 = 0x03;  // %11 dark gray
        break;
        case GRAPHICS_20:
        case GRAPHICS_21:
            // The DXL is set up in setup_VBXE.  Nothing to be done here.
        break;
    }

    settings.gfx_mode = mode;
}

void makeDisplayList(byte mode)
{
    switch(mode)
    {
        case GRAPHICS_0:
            OS.sdlst = ORG_SDLIST;
        break;
        case GRAPHICS_8:
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
            retarget_dl_modes((byte*)&graphics_8_dl, DL_MAP320x1x1);
            dlDef.address = &graphics_8_dl;
        break;
        case GRAPHICS_15:
            retarget_dl_modes((byte*)&graphics_8_dl, DL_MAP160x1x4);
            dlDef.address = &graphics_8_dl;
        break;
        case GRAPHICS_15_CONSOLE:
            retarget_dl_modes((byte*)&graphics_8_console_dl, DL_MAP160x1x4);
            dlDef.address = &graphics_8_console_dl;
        break;
        case GRAPHICS_8_CONSOLE:
        //case GRAPHICS_8_CONSOLE | GRAPHICS_BUFFER_TWO:  // Switch to front buffer for console
            retarget_dl_modes((byte*)&graphics_8_console_dl, DL_MAP320x1x1);
            dlDef.address = &graphics_8_console_dl;
        break;
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        //case GRAPHICS_9_CONSOLE | GRAPHICS_BUFFER_TWO:  // Switch to front buffer for console
        //case GRAPHICS_10_CONSOLE | GRAPHICS_BUFFER_TWO: // Switch to front buffer for console
        //case GRAPHICS_11_CONSOLE | GRAPHICS_BUFFER_TWO: // Switch to front buffer for console
            dlDef.address = &graphics_9_console_dl;
        break;
        case GRAPHICS_8_2:
        case GRAPHICS_9_2:
            dlDef.address = &graphics_8_s2_dl;
        break;
        case GRAPHICS_20:
        case GRAPHICS_21:
            // The DXL is set up in setup_VBXE.  Nothing to be done here.
        break;
    } // switch mode
}

void show_console()
{
    switch(settings.gfx_mode)
    {
        case GRAPHICS_0:
            break;
        case GRAPHICS_8:
        case GRAPHICS_15:
        {
            settings.gfx_mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);

            OS.sdlst = dlDef.address;
            ANTIC.nmien = NMIEN_VBI; //0x40;
            OS.botscr = 5;
        }
        break;
        case GRAPHICS_9:
        case GRAPHICS_10:
        case GRAPHICS_11:
        {
            settings.gfx_mode |= GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);

            OS.sdlst = dlDef.address;
            OS.vdslst = disable_9_dli;
            ANTIC.nmien = NMIEN_DLI | NMIEN_VBI; //0x80 | 0x40;
            OS.botscr = 5;
        }
        break;
        case GRAPHICS_20:
        case GRAPHICS_21:
            // ANTIC/GTIA is in gfx 0 so just show the cursor
            cursor(1);
        break;
    }
}

void hide_console()
{
    switch(settings.gfx_mode) // ^ GRAPHICS_CONSOLE_EN)
    {
        case GRAPHICS_0: // {0, DL_CHR40x8x1, 1, 0, CONSOLE_MEM}
            break;
        case GRAPHICS_8_CONSOLE: // {8, DL_MAP320x1x1, 211, 0, 0}
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        case GRAPHICS_15_CONSOLE:
        {
            settings.gfx_mode &= (byte)~GRAPHICS_CONSOLE_EN;

            makeDisplayList(settings.gfx_mode);

            OS.sdlst = dlDef.address;
            ANTIC.nmien = NMIEN_VBI; //0x40;
            OS.vdslst = VDSLIST_STATE;

            OS.botscr = 0;
        }
        break;
        case GRAPHICS_20:
        case GRAPHICS_21:
            // ANTIC/GTIA is in gfx 0 so just hide the cursor
            cursor(0);
        break;
    }
}

void clearFrameBuffer(void)
{
    switch(settings.gfx_mode)
    {
        case GRAPHICS_0:
        case GRAPHICS_8_CONSOLE:
        case GRAPHICS_9_CONSOLE:
        case GRAPHICS_10_CONSOLE:
        case GRAPHICS_11_CONSOLE:
        case GRAPHICS_15_CONSOLE:
        {
            memset(framebuffer, 0, FRAMEBUFFER_SIZE);
        }
        break;
        case GRAPHICS_20:
        case GRAPHICS_21:
            // Not sure what to do here yet.
        break;
    }
}

// Map internal mode defines to the wire values in the server's PROTOCOL.md.
// The internal values are bitmasks (and persisted in appkeys) so they can't
// be sent raw: GRAPHICS_11 (0x10) would collide with the VBXE wire value 16.
uint8_t graphics_mode_to_wire(uint8_t mode)
{
    switch(mode & ~(GRAPHICS_CONSOLE_EN | GRAPHICS_BUFFER_TWO))
    {
        case GRAPHICS_9:
            return 4;
        case GRAPHICS_15:
            return 6;
        case GRAPHICS_11:
            return 8;
        case GRAPHICS_10:
            return 10;
        case GRAPHICS_20:
        case GRAPHICS_21:
            return 16;
        default:
            return 2;
    }
}

char* graphics_mode_to_string(uint8_t mode)
{
    // OS Graphics numbers; 8-11 all use ANTIC mode F lines,
    // 9-11 reinterpreted by GTIA via PRIOR[7:6].
    switch(mode)
    {
        case GRAPHICS_0:
            return "Graphics 0 text";
        case GRAPHICS_8:
            return "Graphics 8 320x220 mono";
        case GRAPHICS_9:
            return "Graphics 9 GTIA 16 shades";
        case GRAPHICS_10:
            return "Graphics 10 GTIA 9 colors";
        case GRAPHICS_11:
            return "Graphics 11 GTIA 16 hues";
        case GRAPHICS_15:
            return "Graphics 15 160x220 4 shades";
        case GRAPHICS_15_CONSOLE:
            return "Graphics 15 + console";
        case GRAPHICS_8_CONSOLE:
            return "Graphics 8 + console";
        case GRAPHICS_9_CONSOLE:
            return "Graphics 9 + console";
        case GRAPHICS_10_CONSOLE:
            return "Graphics 10 + console";
        case GRAPHICS_11_CONSOLE:
            return "Graphics 11 + console";
        case GRAPHICS_20:
            return "VBXE 320x240 256 colors";
        case GRAPHICS_21:
            return "VBXE 640x240 16 colors";
        default:
            return "Unknown";
    }
}

// Shows the contents of a display list.
// name - simply used in the output header so you can tell which DL is which on the console.
// mloc - the location of the DL in memory
#ifdef DEBUG_GRAPHICS
void printDList(const char* name)
{
    byte b = 0x00, low, high;
    unsigned idx = 0;

    cprintf("Displaylist %s at %p (%d)\n\r", name, dlDef.address, dlDef.size);
    while(1)
    {
        if(idx)
            cprintf(", ");

        // Get the instruction
        b = ((byte*)dlDef.address)[idx];
        if((b & 0x40) && (b & 0x0F)) // these have two address bytes following)
        {
            low = ((byte*)dlDef.address)[++idx];
            high = ((byte*)dlDef.address)[++idx];
            cprintf("%02X (%02x%02x)", b, low, high);
            cgetc();
            clrscr();
        }
        else
            cprintf("%02X", b);

        idx++;

        if(b == 0x41) // JVB so done... maybe  have to add code to look at the address
            break;
    }
}
#endif
