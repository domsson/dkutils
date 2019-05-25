#ifndef SCRIPT_CFG_H
#define SCRIPT_CFG_H

#ifdef MSDOS
#define ABORT 34		       /* scan code for ^G */
#else
#define ABORT 7			       /* character code for ^G */
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define EVER ;;

#if defined (unix) && !defined(GO32)
extern volatile int safe_update, update_required;
extern void update (void);
#endif

#endif
