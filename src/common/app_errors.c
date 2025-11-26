#include "app_errors.h"
#include "utility.h"
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>

uint8_t err = 0;

void handle_err(const char *reason) {
    if (err) {
        cputs("Error: ");
        cputs(reason);
        cputs("\r\n");
        
        // Using utility function to pause/wait
        pause("Press any key to exit...");
        exit(1);
    }
}
