// Commodore 64 implementation of the YAIL platform entry point.
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
    cputs("YAIL (YET ANOTHER IMAGE LOADER)\r\n");
    cputs("THE C64 PORT IS NOT AVAILABLE YET.\r\n");
    cputs("SEE THE PROJECT README.\r\n");
    cputs("\r\nPRESS ANY KEY TO EXIT.\r\n");
    cgetc();

    return 0;
}
