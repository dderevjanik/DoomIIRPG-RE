#ifndef __CRASHHANDLER_H__
#define __CRASHHANDLER_H__

// Install signal handlers for SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL.
// Call early in main() before any game initialization.
void CrashHandler_Init();

#endif
