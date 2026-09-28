#ifndef _HOSTTEST_SD_H
#define _HOSTTEST_SD_H
/* struct sdcom, layout-compatible with usr/sys/amiga/alien/sd.h so idequeue()
 * sees the exact kernel ABI (field names/types/order as in the Amix
 * header; the struct is an interface, not code).  rico.h must precede this. */
#define SDCARDS 2
#define SDUNITS 8
#define sdspl   spl2

struct sdcom {
	volatile struct sdcom	*next;
	bool		reading,
			okay;
	uchar		status,
			cdb[12];
	caddr_t		addr;
	uint		nbyte,
			card,
			unit;
	void		(*intr)( );
};
#endif
