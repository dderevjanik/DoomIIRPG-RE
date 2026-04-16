#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <execinfo.h>
#include <unistd.h>
#endif

#include "CrashHandler.h"
#include "Log.h"

static const char* signalName(int sig) {
	switch (sig) {
		case SIGSEGV: return "SIGSEGV (Segmentation fault)";
		case SIGABRT: return "SIGABRT (Aborted)";
#ifdef SIGBUS
		case SIGBUS:  return "SIGBUS (Bus error)";
#endif
		case SIGFPE:  return "SIGFPE (Floating point exception)";
		case SIGILL:  return "SIGILL (Illegal instruction)";
#ifdef SIGTRAP
		case SIGTRAP: return "SIGTRAP (Trace trap)";
#endif
		default:      return "Unknown signal";
	}
}

static void writeCrashLog(const char* text) {
	FILE* f = fopen("crash.log", "w");
	if (f) {
		fputs(text, f);
		fclose(f);
	}
}

static void crashHandler(int sig) {
	// Prevent recursive signals
	signal(sig, SIG_DFL);

	char buf[4096];
	int pos = 0;

	pos += snprintf(buf + pos, sizeof(buf) - pos,
		"\n=== CRASH ===\n"
		"Signal: %d — %s\n\n"
		"Stack trace:\n",
		sig, signalName(sig));

#ifdef _WIN32
	void* stack[64];
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, nullptr, TRUE);
	WORD frames = CaptureStackBackTrace(1, 64, stack, nullptr);

	SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	for (WORD i = 0; i < frames; i++) {
		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
		pos += snprintf(buf + pos, sizeof(buf) - pos,
			"  #%d  0x%0llx  %s\n", i, (unsigned long long)symbol->Address, symbol->Name);
		if (pos >= (int)sizeof(buf) - 1) break;
	}

	free(symbol);
	SymCleanup(process);
#else
	void* stack[64];
	int count = backtrace(stack, 64);
	char** symbols = backtrace_symbols(stack, count);

	if (symbols) {
		for (int i = 1; i < count; i++) {
			pos += snprintf(buf + pos, sizeof(buf) - pos, "  #%d  %s\n", i - 1, symbols[i]);
			if (pos >= (int)sizeof(buf) - 1) break;
		}
		free(symbols);
	} else {
		pos += snprintf(buf + pos, sizeof(buf) - pos, "  (failed to get symbols)\n");
	}
#endif

	snprintf(buf + pos, sizeof(buf) - pos,
		"\nFor better traces, build with: cmake -DCMAKE_BUILD_TYPE=Debug\n");

	fprintf(stderr, "%s", buf);

	// Write crash.log with stack trace + recent log entries for context
	FILE* crashFile = fopen("crash.log", "w");
	if (crashFile) {
		fputs(buf, crashFile);
		logDumpRecentToFile(crashFile, 64);
		fputs("\nThis trace was saved to crash.log\n", crashFile);
		fclose(crashFile);
	}

	// Re-raise so the OS can generate a core dump if configured
	raise(sig);
}

void CrashHandler_Init() {
	signal(SIGSEGV, crashHandler);
	signal(SIGABRT, crashHandler);
#ifdef SIGBUS
	signal(SIGBUS, crashHandler);
#endif
	signal(SIGFPE, crashHandler);
	signal(SIGILL, crashHandler);
#ifdef SIGTRAP
	signal(SIGTRAP, crashHandler);
#endif
}
