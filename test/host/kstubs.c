/*
 * kstubs.c -- the kernel services ide.c uses, stubbed for the host harness:
 * the spl bracket (counted), timeout() (recorded, fired by the test) and a
 * printf that records what the driver would have put on the console.
 */
#include <stddef.h>
#include <string.h>

/* No <stdio.h>: it would prototype printf() as variadic (see below). */
extern int	snprintf( char *, size_t, const char *, ...);

int	mock_ipl;
long	mock_spl_calls, mock_splx_calls;
char	mock_console[8192];

int
spl2()
{
	int	old = mock_ipl;

	mock_spl_calls++;
	mock_ipl = 2;
	return old;
}

int
splx( s)
int	s;
{
	mock_splx_calls++;
	mock_ipl = s;
	return 0;
}

void
mock_spl_reset()
{
	mock_ipl = 0;
	mock_spl_calls = mock_splx_calls = 0;
}

void
mock_console_reset()
{
	mock_console[0] = '\0';
}

/* Overrides the C library's printf for the whole executable (the harness
 * reports through fprintf(stdout/stderr)).  Defined K&R-style with fixed
 * slots, not variadic: the driver calls it through `extern int printf();`,
 * i.e. with the non-variadic convention, which on arm64 differs from the
 * variadic one.  Every slot is register/stack-word sized, so the driver's
 * int and pointer arguments (%d/%x/%s, at most 7) arrive intact. */
#undef printf
int
printf( fmt, a1, a2, a3, a4, a5, a6, a7)
const char	*fmt;
long	a1, a2, a3, a4, a5, a6, a7;
{
	size_t	used = strlen( mock_console);

	return snprintf( mock_console + used, sizeof mock_console - used, fmt,
	    a1, a2, a3, a4, a5, a6, a7);
}

/*
 * timeout(func, arg, ticks): recorded, not scheduled.  mock_fire_timeouts()
 * runs every pending callout once (they may re-arm themselves).
 */
#define MOCK_NTO	8
static struct { void (*func)(); char *arg; long ticks; } mock_to[MOCK_NTO];
static int	mock_nto;
long		mock_timeout_calls;

int
timeout( func, arg, ticks)
void	(*func)();
char	*arg;
long	ticks;
{
	mock_timeout_calls++;
	if (mock_nto < MOCK_NTO) {
		mock_to[mock_nto].func = func;
		mock_to[mock_nto].arg = arg;
		mock_to[mock_nto].ticks = ticks;
		mock_nto++;
	}
	return mock_nto;
}

int
mock_timeouts_pending()
{
	return mock_nto;
}

void
mock_fire_timeouts()
{
	void	(*f[MOCK_NTO])();
	char	*a[MOCK_NTO];
	int	n, i;

	n = mock_nto;
	for (i = 0; i < n; ++i) { f[i] = mock_to[i].func; a[i] = mock_to[i].arg; }
	mock_nto = 0;
	for (i = 0; i < n; ++i)
		(*f[i])( a[i]);
}

void
mock_timeouts_reset()
{
	mock_nto = 0;
	mock_timeout_calls = 0;
}
