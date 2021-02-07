/* ------------------------------------------------------------------------ */
/* LHa for UNIX    Directory access routine                                 */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/* Emulate opendir(), readdir(), closedir() function for LHa                */
/*                                                                          */
/*  Ver. 1.14   Soruce All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */

#ifndef DIRBLKSIZ
#define DIRBLKSIZ   512
#endif

/* ------------------------------------------------------------------------ */
/*  Type Definition                                                         */
/* ------------------------------------------------------------------------ */
struct direct {
    int             d_ino;
    int             d_namlen;
    char            d_name[256];
};

typedef struct {
    int             dd_fd;
    int             dd_loc;
    int             dd_size;
    char            dd_buf[DIRBLKSIZ];
}               DIR;

/* ------------------------------------------------------------------------ */
/*  Functions                                                               */
/* ------------------------------------------------------------------------ */
extern DIR           *opendir();
extern struct direct *readdir();
extern int           closedir();
