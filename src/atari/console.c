// Copyright (C) 2021 Brad Colbert
#include "netimage.h"
#include "readNetPBM.h"
#include "graphics.h"
#include "console.h"
#include "files.h"
#include "consts.h"
#include "settings.h"
#include "types.h"
//#include "fujinet-io.h"
#include "utility.h"

#include <atari.h>
#include <conio.h>
#include <peekpoke.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//
#define WORKING_BUFF_SIZE 200

// Externs
extern char version[];
extern bool done;
extern byte IMAGE_FILE_TYPE;
extern void* ORG_SDLIST;
extern void graphics_8_console_dl[];
extern void graphics_9_console_dl[];
extern Settings settings;
extern byte buff[];

// Globals
bool console_state = false;
uint8_t SAVED_MODE = GRAPHICS_8;

#ifdef CONSOLE_USE_LOCAL_BUFFER
char CONSOLE_BUFF[GFX_0_MEM_LINE * CONSOLE_LINES];
#else
#define CONSOLE_BUFF ((byte*)((ushort*)ORG_SDLIST)[2])
#endif
char* tokens[NUM_TOKENS];

void reset_console(void)
{
    memset(CONSOLE_BUFF, 0, GFX_0_MEM_LINE * CONSOLE_LINES); // wipe all of the console mem
    gotoxy(0,0);                                             // start at the begining of the input line
}

#define SEARCHING_FOR_TOKEN 1
#define START_OF_TOKEN 2
#define END_OF_TOKEN 3
byte get_tokens(byte* buff, byte endx)
{
    byte count = 0;
    byte state = SEARCHING_FOR_TOKEN;
    byte i;
    for(i = 0; i < endx && count < NUM_TOKENS; ++i)
    {
        switch(state)
        {
            case SEARCHING_FOR_TOKEN:
                if(buff[i] != 0x0)
                {
                    tokens[count] = (char*)&buff[i];
                    state = START_OF_TOKEN;
                    ++count;
                }
                break;
            case START_OF_TOKEN:
                if(buff[i] == 0x0)
                {
                    state = SEARCHING_FOR_TOKEN;
                }
                break;
        }
    }

    // Fix last token.  For some strange reason, the last token is ending with 0x80 (ENTER key?)
    i = strlen(tokens[count-1]);
    tokens[count-1][i-1] = 0x0;

    #ifdef DEBUG_CONSOLE
    gotoxy(0,2);
    cprintf("%p  ", &tokens[0]);
    for(i = 0; i < count; ++i)
        cprintf("%s,", tokens[i]);
    cputs("\n\r");
    cgetc();
    #endif

    return count;
}

char process_command(byte ntokens)
{
    if(!ntokens)
        return 0x0;

    if(strncmp(tokens[0], "help", 4) == 0)
    {
        //byte SAVED_MODE = settings.gfx_mode;

        setGraphicsMode(GRAPHICS_0);
        //const char help[] =
        cputs(
        "quit - Exit this utility\n\r"
        "cls  - Clear the image display\n\r"
        "gfx  - [0,8,9,11,15,20] graphics mode\n\r"
        "       0 : Graphics 0 text\n\r"
        "       8 : Graphics 8 320x220 mono\n\r"
        "       9 : Graphics 9 GTIA 16 shades\n\r"
        "       11: Graphics 11 GTIA 16 hues\n\r"
        "       15: Graphics 15 160x220 4 shd\n\r"
        "       20: VBXE 320x240 256 colors\n\r"
        "set  - Saved settings\n\r"
        "       server [url] (N:TCP://blah.duh/)\n\r"
        "       model [ai model name] (dall-e-3)\n\r"
        #ifdef YAIL_BUILD_FILE_LOADER
        "load - [filename] Load and display file\n\r"
        "save - [filename] Save image to YAI file\n\r"
        #else
        "search - [arg0...argN] Find images\n\r"
        "generate - [prompt] AI images\n\r"
        "video  - Stream video\n\r"
        #endif
        );

        cgetc();                                  // Wait
        setGraphicsMode(SAVED_MODE);              // Restore the graphics mode
    }

    if(strncmp(tokens[0], "quit", 4) == 0)
        done = TRUE;

    if(strncmp(tokens[0], "gfx", 3) == 0)
    {
        if(ntokens > 1)
        {
            if (strncmp(tokens[1], "0", 2) == 0)
                setGraphicsMode(GRAPHICS_0);
            else if (strncmp(tokens[1], "2", 2) == 0)
            {
                byte new_mode = (settings.gfx_mode & 0x0F) | GRAPHICS_BUFFER_TWO;
                setGraphicsMode(new_mode);
            }
            else if (strncmp(tokens[1], "8", 2) == 0)
                setGraphicsMode(GRAPHICS_8);
            else if (strncmp(tokens[1], "9", 2) == 0)
                setGraphicsMode(GRAPHICS_9);
            else if (strncmp(tokens[1], "10", 2) == 0)
                setGraphicsMode(GRAPHICS_10);
            else if (strncmp(tokens[1], "11", 2) == 0)
                setGraphicsMode(GRAPHICS_11);
            else if (strncmp(tokens[1], "15", 2) == 0)
                setGraphicsMode(GRAPHICS_15);
            else if (strncmp(tokens[1], "20", 2) == 0)
            {
                setGraphicsMode(GRAPHICS_0);  // VBXE renders both the ANTIC and it's own output.  ANTIC is the console.
                setGraphicsMode(GRAPHICS_20);
            }
            else if (strncmp(tokens[1], "21", 2) == 0)
            {
                setGraphicsMode(GRAPHICS_0);  // VBXE renders both the ANTIC and it's own output.  ANTIC is the console.
                setGraphicsMode(GRAPHICS_21);
            }
            else if (strncmp(tokens[1], "*", 2) == 0)
                settings.gfx_mode = '*';
        }

        // Save the graphics mode if it is a real display mode (not text or
        // the '*' sentinel; GRAPHICS_15 is 0x40 so a range check won't do)
        switch(settings.gfx_mode)
        {
            case GRAPHICS_8:
            case GRAPHICS_9:
            case GRAPHICS_10:
            case GRAPHICS_11:
            case GRAPHICS_15:
            case GRAPHICS_20:
            case GRAPHICS_21:
                put_settings(SETTINGS_GFX);  // save the graphics mode on the FN
            break;
        }
    }

    if(strncmp(tokens[0], "cls", 3) == 0)
    {
        clearFrameBuffer();
    }

    if(strncmp(tokens[0], "load", 4) == 0)
    {
        #ifdef YAIL_BUILD_FILE_LOADER
        if(ntokens > 1)
        {
            load_image_file(tokens[1]);
        }
        else
        {
            show_error_pause("ERROR: File not specified");
        }
        #else
            show_error_pause("ERROR: File loading not supported");
        #endif
    }

    if(strncmp(tokens[0], "save", 4) == 0)
    {
        #ifdef YAIL_BUILD_FILE_LOADER
        if(ntokens > 1)
            saveFile(tokens[1]);

        else
        {
            show_error_pause("ERROR: File not specified");
        }
        #else
            show_error_pause("ERROR: File saving not supported");
        #endif
    }

    if(strncmp(tokens[0], "set", 3) == 0)
    {
        if(ntokens < 3)
        {
            //show_error_pause("ERROR: Must specify a setting and value");
            setGraphicsMode(GRAPHICS_0);

            print_settings(SAVED_MODE, settings.url, settings.ai_model_name);

            cgetc();                                  // Wait
            setGraphicsMode(SAVED_MODE);              // Restore the graphics mode
            }
        else
        {
            if(strncmp(tokens[1], "server", 3) == 0)
            {
                strncpy(settings.url, tokens[2], SERVER_URL_SIZE);
                put_settings(SETTINGS_URL);  // save the URL on the FN
            }
            else if(strncmp(tokens[1], "mode", 3) == 0)
            {
                if(strncmp(tokens[2], "random", 3) == 0)
                {
                    // This is a bit of a cheat.  The stored mode is * and this doesn't
                    // change the rendered mode without calling setGraphicsMode (which we aren't).
                    // This should get sent to the server during the stream command handling.
                    // The server will use this setting to randomize the graphics per image.
                    settings.gfx_mode = '*';
                }
            }
            else if(strncmp(tokens[1], "model", 5) == 0)
            {
                strncpy(settings.ai_model_name, tokens[2], AI_MODEL_NAME_SIZE);
                put_settings(SETTINGS_AI_MODEL);  // save the model name on the FN
            }
        }
    }

    if(0 == strncmp(tokens[0], "search", 3))
    {
        return stream_image(tokens);
    }

    if(0 == strncmp(tokens[0], "gen", 3))
    {
        return stream_image(tokens);
    }

    if(0 == strncmp(tokens[0], "video", 3))
    {
        return stream_image(tokens);
    }

    if(strncmp(tokens[0], "showurl", 3) == 0)
    {
        if(ntokens < 2)
        {
            show_error_pause("ERROR: URL not specified");
        }
        else
        {
            #ifdef DEBUG_CONSOLE
            byte i;
            for(i = 0; tokens[i] != 0x0; i++)
                cprintf("*%s ", tokens[i]);

            cputs("\n\r");
            #endif

            return stream_image(tokens);
        }
    }

    return 0x0;
}

void start_console(char first_char)
{
    byte* gfx8_console_dl = (byte*)graphics_8_console_dl;
    byte* gfx9_console_dl = (byte*)graphics_9_console_dl;
    byte* console_buff = (byte*)CONSOLE_BUFF;
    uint8_t x = 0;   // Char position in the input line

    // Fix addresses for graphics display lists for the console buffer
    // (graphics_8_console_dl doubles as the Graphics 15 console DL; only
    // the mode nibbles differ and setGraphicsMode retargets those)
    POKEW(gfx8_console_dl + 2, (ushort)version);
    POKEW(gfx8_console_dl + 185, (ushort)console_buff);
    POKEW(gfx9_console_dl + 2, (ushort)version);
    POKEW(gfx9_console_dl + 185, (ushort)console_buff);

    reset_console();

    cursor(1);

    while(true)
    {
        byte input = first_char?first_char:cgetc();

        // Already used so reset
        first_char = 0x00;

        // Handle quit
        if(input == CH_ESC)
        {
            #ifdef DEBUG_CONSOLE
            cputs("ESC hit... quiting\n\r");
            cgetc();
            #endif
            return;
        }
   
        // Handle command
        switch(input)
        {
            case CH_ENTER:
            {
                // process the tokens
                byte work_buff[WORKING_BUFF_SIZE];  // five lines of data
                byte ntokens = 0;

                memset(work_buff, 0, WORKING_BUFF_SIZE);  // clear five lines of data
                memcpy(work_buff, CONSOLE_BUFF, WORKING_BUFF_SIZE);
                internal_to_atascii((char*)work_buff, WORKING_BUFF_SIZE);

                #ifdef DEBUG_CONSOLE
                gotoxy(0,1);
                cprintf("%s\n\r", CONSOLE_BUFF);
                cprintf("%s", work_buff);
                cgetc();
                #endif

                ntokens = get_tokens(work_buff, x);

                #ifdef DEBUG_CONSOLE
                gotoxy(0,4);
                cprintf("%d  ", ntokens);
                #endif

                #ifdef DEBUG_CONSOLE
                // For debugging purposes, should display below the input
                // line which will not be shown when the system is active.
                {
                    byte i;

                    //memcpy(&line[40], line, 40);

                    cclearxy(0, 4, 40);
                    // gotoxy(0,4);
                    // for(i = 0; i < x; i++)
                    //     cprintf("%02x ", line[i]);

                    #define TOKEN_DBG_LINE 1
                    gotoxy(0,TOKEN_DBG_LINE);
                    cprintf("%d\n\r", ntokens);
                    for(i = 0; i < ntokens; i++)
                    {
                        cclearxy(0, i+TOKEN_DBG_LINE, 40);
                        gotoxy(0,i+TOKEN_DBG_LINE);
                        cprintf("%d: %s", i, tokens[i]);
                    }

                    cgetc();
                }
                #endif

                if(ntokens > 0)
                {
                    byte i;
                    reset_console();
                    first_char = process_command(ntokens);

                    // Clear the tokens for the next command
                    for(i = 0; i < NUM_TOKENS; ++i)
                        tokens[i] = 0x0;
                }

                if(0x0 == first_char)  // the user had no input while processing the command, let's reset
                {
                    reset_console();
                    hide_console();
                    cursor(0);
                    return;
                }
            }
            break;

            case CH_DEL:
                if(x > 0)
                {
                    --x;
                    CONSOLE_BUFF[x] = 0x0;
                    gotoxy(x%40, x/40);
                }
            break;

            default:
                #ifdef DEBUG_CONSOLE
                {
                    byte i;
                    for(i=0; i<x; ++i)
                    {
                        gotoxy(i*2, 1);
                        cprintf("%02x ", line[i]);
                    }
                    gotoxy(x, 0);
                }
                #endif
                if(!console_state)
                {
                    console_state = true;
                    show_console();
                }

                ++x;
                if (x >= WORKING_BUFF_SIZE)
                {
                    x = WORKING_BUFF_SIZE - 1;
                }
                else
                    cputc(input);
        }
    }
}
