#ifndef __ERROR_H__
#define __ERROR_H__

// Fatal error handler — each target (engine, converter) provides its own implementation.
void Error(const char* fmt, ...);

#endif
