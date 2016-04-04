#ifndef dbg_h
#define dbg_h

#include <stdio.h>
#include <stdarg.h>

#ifdef NDEBUG
#define DEBUG(...)
#else
#define DEBUG(...) DEBUG_FUNC(__FILE__, __LINE__, __VA_ARGS__)

static void DEBUG_FUNC(char const *file, int line, char const *fmt, ...)
{
	fprintf(stderr, "DEBUG %s:%d: ", file, line);
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
}
#endif

#endif
