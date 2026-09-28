#ifndef _HOSTTEST_RICO_H
#define _HOSTTEST_RICO_H
/* Subset of the Amix "alien" short-name macros ide.c and sd.h rely on
 * (identical definitions to usr/sys/amiga/alien/rico.h, trimmed). */
#define uchar   unsigned char
#define ushort  unsigned short
#define uint    unsigned
#define ulong   unsigned long
#define bool    char
#define TRUE    (0 == 0)
#define FALSE   (not TRUE)
#define not     !
#define and     &&
#define or      ||
#define loop    while (TRUE)
#define until(expr)   while (not (expr))
#define unless(expr)  if (not (expr))
#define nel( a)       (sizeof( a) / sizeof( (a)[0]))
#define endof( a)     ((a) + nel( a))
#endif
