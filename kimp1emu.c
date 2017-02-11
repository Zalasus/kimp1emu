
#include <stdio.h>
#include <stdarg.h>
#define __USE_POSIX199309
#include <time.h>
#include <string.h>
#include <ncurses.h>

#include "kimp1emu.h"

#include "pit.h"
#include "fdc.h"
#include "rtc.h"
#include "usart.h"

void kimp_init(KIMP_CONTEXT *context)
{
    // we really only need to initialize registers
    //  that have a reset. every other value is random
    //  on the real thing, too
    context->tccr = 0;
    context->ebcr = 0x80; // opl irq normally high

    context->has_extension = FALSE;
    context->stopped = FALSE;
}

int loadRomfile(const char *romfilename, KIMP_CONTEXT *context)
{
    FILE *romfile = fopen(romfilename, "rb");
    if(romfile == NULL)
    {
        printf("Error: Can't open romfile '%s'\n", romfilename);
        return -1;
    }

    size_t c = fread(context->rom, 1, ROM_SIZE, romfile);
    fclose(romfile);
    if(c == 0)
    {
        printf("Error: Failed to read romfile\n");
        return -1;
    }

    printf("Read %i bytes to ROM\n", (int)c);

    return 0;
}

void kimp_debug(const char *msg, ...)
{
    attron(COLOR_PAIR(1));

    va_list args;
    va_start(args, msg);
    vwprintw(stdscr, msg, args);
    printw("\n");
    va_end(args);

    attroff(COLOR_PAIR(1));
}

void ui_init()
{
    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);
    idlok(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
}

void usage()
{
    printf(
        "Usage kimp1emu [options]* romfile \n"
        "Options are: \n"
        "    -h    Shows this message \n"
        "    -e    Emulate with extension board \n"
        "    -r    Emulate in realtime\n"
    );
}


int main(int argc, const char **argv)
{
    KIMP_CONTEXT context;
    kimp_init(&context);

    const char *romfilename = NULL;
    int realtime = FALSE;
    for(int32_t i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-h"))
        {
            usage();
            return 0;

        }else if(!strcmp(argv[i], "-e"))
        {
            context.has_extension = TRUE;

        }else if(!strcmp(argv[i], "-r"))
        {
            realtime = TRUE;

        }else
        {
            if(romfilename != NULL)
            {
                printf("Error: Can't handle multiple romfiles \n");
                usage();
                return -1;
            }
    
            romfilename = argv[i];
        }
    }

    if(romfilename == NULL)
    {
        printf("Error: No romfile given \n");
        usage();
        return -1;
    }

    if(loadRomfile(romfilename, &context))
    {
        return -1;
    }

    ui_init();

    Z80Reset(&context.state);

    while(!context.stopped)
    {
        uint32_t ticksElapsed = Z80Emulate(&context.state, 1, &context);

        double usElapsed = (double)ticksElapsed / CPU_SPEED_MHZ;

        ticksElapsed += fdc_tick(usElapsed, &context);
        ticksElapsed += pit_tick(usElapsed, &context);
        ticksElapsed += rtc_tick(usElapsed, &context);
        ticksElapsed += usart_tick(usElapsed, &context);

        usElapsed = (double)ticksElapsed / CPU_SPEED_MHZ;

        if(usart_hasTxChar())
        {
            addch(usart_getTxChar());
        }

        int c = getch();
        if(c != ERR)
        {
            usart_rxChar(c);
        }

        if(realtime)
        {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = usElapsed * 1000;
            nanosleep(&ts, NULL);
        }
    }

    endwin();

    return 0;
}


