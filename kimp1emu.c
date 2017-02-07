
#include <stdio.h>
#include <string.h>
#include <ncurses.h>

#include "kimp1emu.h"

#include "pit.h"
#include "fdc.h"
#include "rtc.h"

void kimp_init(KIMP_CONTEXT *context)
{
    // we really only need to initialize registers
    //  that have a reset. every other value is random
    //  on the real thing, too
    context->tccr = 0;
    context->ebcr = 0;

    context->has_extension = 0;
    context->stopped = 0;
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

void ui_init()
{

    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

}

void usage()
{
    printf(
        "Usage kimp1emu [options]* romfile \n"
        "Options are: \n"
        "    -h    Shows this message \n"
        "    -e    Emulate with extension board \n"
    );
}


int main(int argc, const char **argv)
{
    KIMP_CONTEXT context;
    kimp_init(&context);

    const char *romfilename = NULL;
    for(int32_t i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "-h"))
        {
            usage();
            return 0;

        }else if(!strcmp(argv[i], "-e"))
        {
            context.has_extension = 1;

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
        uint32_t ticksElapsed = Z80Emulate(&context.state, 50, &context);

        double usElapsed = (double)ticksElapsed / CPU_SPEED_MHZ;

        fdc_tick(usElapsed, &context);
        pit_tick(usElapsed, &context);
        rtc_tick(usElapsed, &context);
    }

    endwin();

    return 0;
}


