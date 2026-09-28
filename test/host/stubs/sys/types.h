#ifndef _HOSTTEST_SYS_TYPES_H
#define _HOSTTEST_SYS_TYPES_H
/* Minimal SVR4 <sys/types.h> shim for the ide.c host compile: only caddr_t
 * is needed (rico.h supplies the u* short names).  Seen only by the driver
 * translation unit (-Istubs); the harness files use the real system headers. */
typedef char *caddr_t;
#endif
