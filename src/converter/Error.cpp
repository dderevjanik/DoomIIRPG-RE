#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "Error.h"

void Error(const char* fmt, ...) {
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
	va_end(ap);

	fprintf(stderr, "Error: %s\n", errMsg);
	exit(1);
}
