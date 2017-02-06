

#include "kimp1emu.h"


void kimp_init(KIMP_CONTEXT *context)
{
    // we really only need to initialize registers
    //  that have a reset. every other value is random
    //  on the real thing, too
    context->tccr = 0;
    context->ebcr = 0;

    context->stopped = 0;
}


void usage()
{
    printf(
        "Usage kimp1emu [options]* romfile \n"
        "Options are: \n"
        "    -h    Shows this message \n"
    );
}


int main(int argc, const char **argv)
{
    const char *romfilename = NULL;
    for(int32_t i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "-h")
        {
            usage();
            return 0;

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

    FILE *romfile = fopen(romfilename, "rb");
    if(romfile == NULL)
    {
        printf("Error: Can't open romfile '%s'", romfilename);
        return -1;
    }

    size_t romfilesize;
    fseek(romfile, 0, SEEK_END);
    romfilesize = ftell(romfile);
    fseek(romfile, 0, SEEK_SET);

    if(romfilesize > ROM_SIZE)
    {
        romfilesize = ROM_SIZE;

        printf("Warning: Truncated romfile to maximum rom size");
    }

    KIMP_CONTEXT context;
    kimp_init(&context);

    size_t c = fread(context.rom, 1, romfilesize, romfile);
    fclose(romfile);
    if(c != romfilesize)
    {
        printf("Error: Failed to read romfile");
        return -1;
    }

    
    Z80Reset(&context.state);

    while(!context.stopped)
    {
        Z80Emulate(&context.state, 100, &context);
    }

    return 0;
}


