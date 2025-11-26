#include "utility.h"
#include <conio.h>

void pause(const char* message)
{
    if(message)
        cputs(message);
    while(kbhit()) cgetc();
    cgetc();
}

void internal_to_atascii(char* buff, byte len) {}
void atascii_to_internal(char* buff, byte len) {}
void show_error(const char* message) { cputs(message); }
void show_error_pause(const char* message) { show_error(message); cgetc(); }
