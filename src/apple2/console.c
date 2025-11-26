#include "console.h"
#include <conio.h>

void show_console(void) { cputs("Console Shown\r\n"); }
void hide_console(void) { cputs("Console Hidden\r\n"); }
void start_console(char initialChar) { cputs("Console Started\r\n"); }
