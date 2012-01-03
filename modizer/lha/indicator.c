/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              indicator.c -- put indicator                                */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/*              Separated from append.c         2003.07.21  Koji Arai       */
/* ------------------------------------------------------------------------ */
#include "lha.h"

#define MAX_INDICATOR_COUNT     58

static size_t reading_size;

#ifdef NEED_INCREMENTAL_INDICATOR
static size_t indicator_count;
static long indicator_threshold;
#endif

#define ALIGN(size, threshold) (((size) + ((threshold)-1))/(threshold))

static void
carriage_return()
{
    static int tty = -1;
    if (tty == -1) {
        if (isatty(1))          /* stdout */
            tty = 1;
        else
            tty = 0;
    }

    if (tty)
        fputs("\r", stdout);
    else
        fputs("\n", stdout);
}

void
start_indicator(name, size, msg, def_indicator_threshold)
    char           *name;
    size_t          size;
    char           *msg;
    long            def_indicator_threshold;
{
#ifdef NEED_INCREMENTAL_INDICATOR
    long i;
    int m;
#endif

    if (quiet)
        return;

#ifdef NEED_INCREMENTAL_INDICATOR
    switch (quiet_mode) {
    case 0:
        m = MAX_INDICATOR_COUNT - strlen(name);
        if (m < 1)      /* Bug Fixed by N.Watazaki */
            m = 3;      /* (^_^) */
        carriage_return();
        printf("%s\t- %s :  ", name, msg);

        indicator_threshold =
            ALIGN(size, m*def_indicator_threshold) * def_indicator_threshold;

        if (indicator_threshold)
            i = ALIGN(size, indicator_threshold);
        else
            i = 0;

        while (i--)
            putchar('.');
        indicator_count = 0;
        carriage_return();
        printf("%s\t- %s :  ", name, msg);
        break;
    case 1:
        carriage_return();
        printf("%s :", name);
        break;
    }
#else
    printf("%s\t- ", name);
#endif
    fflush(stdout);
    reading_size = 0L;
}

#ifdef NEED_INCREMENTAL_INDICATOR
void
put_indicator(count)
    long int        count;
{
    reading_size += count;
    if (!quiet && indicator_threshold) {
        while (reading_size > indicator_count) {
            putchar('o');
            fflush(stdout);
            indicator_count += indicator_threshold;
        }
    }
}
#endif

void
finish_indicator2(name, msg, pcnt)
    char           *name;
    char           *msg;
    int             pcnt;
{
    if (quiet)
        return;

    if (pcnt > 100)
        pcnt = 100; /* (^_^) */
#ifdef NEED_INCREMENTAL_INDICATOR
    carriage_return();
    printf("%s\t- %s(%d%%)\n", name,  msg, pcnt);
#else
    printf("%s\n", msg);
#endif
    fflush(stdout);
}

void
finish_indicator(name, msg)
    char           *name;
    char           *msg;
{
    if (quiet)
        return;

#ifdef NEED_INCREMENTAL_INDICATOR
    carriage_return();
    printf("%s\t- %s\n", name, msg);
#else
    printf("%s\n", msg);
#endif
    fflush(stdout);
}
