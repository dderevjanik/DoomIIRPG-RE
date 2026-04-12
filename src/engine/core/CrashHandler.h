#pragma once
// Install signal handlers for SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL.
// Call early in main() before any game initialization.
void CrashHandler_Init();
