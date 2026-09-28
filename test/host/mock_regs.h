/* Force-included into the ide.c host compile: routes the driver's register
 * seam (absolute addresses) into the mock Gayle/ATA model (mock_gayle.c). */
#ifndef MOCK_REGS_H
#define MOCK_REGS_H
extern unsigned char  mock_rb();
extern void           mock_wb();
extern unsigned short mock_rw();
extern void           mock_ww();
#define RB( a)		mock_rb( (unsigned long)(a))
#define WB( a, v)	mock_wb( (unsigned long)(a), (unsigned char)(v))
#define RW( a)		mock_rw( (unsigned long)(a))
#define WW( a, v)	mock_ww( (unsigned long)(a), (unsigned short)(v))
#endif
