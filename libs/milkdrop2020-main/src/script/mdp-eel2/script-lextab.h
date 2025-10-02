#pragma once

namespace Script { namespace mdpx {

    struct  lextab {
        int     llendst;                /* Last state number            */
        const char    *lldefault;             /* Default state table          */
        const char    *llnext;                /* Next state table             */
        const char    *llcheck;               /* Check table                  */
        const int     *llbase;                /* Base table                   */
        int     llnxtmax;               /* Last in next table           */
        int     (*llmove)(const lextab *, int, int);            /* Move between states          */
        const char     *llfinal;               /* Final state descriptions     */
        int     (*llactr)(struct yycontext *, int);            /* Action routine               */
        const int     *lllook;                /* Look ahead vector if != NULL */
        const char    *llign;                 /* Ignore char vec if != NULL   */
        const char    *llbrk;                 /* Break char vec if != NULL    */
        const char    *llill;                 /* Illegal char vec if != NULL  */
    };
    
const lextab *GetDefaultLexTab();


}}
