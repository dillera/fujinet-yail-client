// Apple II implementation of the YAIL platform entry point.
//
// Placeholder: builds and links against fujinet-lib so the target stays
// healthy, but the image viewer itself has not been ported yet.

#include "platform.h"

#include <conio.h>

int platform_run(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    clrscr();
    cputs("YAIL (Yet Another Image Loader)\r\n");
    cputs("The Apple II port is not available\r\n");
    cputs("yet. See the project README.\r\n");
    cputs("\r\nPress any key to exit.\r\n");
    cgetc();

    return 0;
}
