#ifndef _HOSTTEST_SYS_INLINE_H
#define _HOSTTEST_SYS_INLINE_H
/* The real amiga <sys/inline.h> emits the m68k SR manipulation inline
 * (spl2 = `move.w #0x2200,%sr`, splx restores the SR word).  On the host the
 * driver's sdspl()/splx() bracket resolves to the counting mock in kstubs.c. */
extern int spl2();
extern int splx();
#endif
