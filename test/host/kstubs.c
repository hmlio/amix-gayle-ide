/*
 * kstubs.c -- the kernel services ide.c uses, stubbed for the host harness:
 * the spl bracket (counted), timeout() (recorded, fired by the test) and a
 * printf that records what the driver would have put on the console.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

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
 * reports through fprintf(stdout/stderr)). */
int
printf( const char *fmt, ...)
{
	va_list	ap;
	size_t	used = strlen( mock_console);
	int	n;

	va_start( ap, fmt);
	n = vsnprintf( mock_console + used, sizeof mock_console - used, fmt, ap);
	va_end( ap);
	return n;
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
