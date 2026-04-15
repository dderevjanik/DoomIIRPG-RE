#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>

#include <SDL.h>
#include "Error.h"
#include "Log.h"
#include "CAppContainer.h"

static void showFatalMessageBox(const char* msg) {
	const SDL_MessageBoxButtonData buttons[] = {
	    {/* .flags, .buttonid, .text */ 0, 0, "Ok"},
	};
	const SDL_MessageBoxColorScheme colorScheme = {{/* .colors (.r, .g, .b) */
	                                                /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
	                                                {255, 0, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_TEXT] */
	                                                {0, 255, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
	                                                {255, 255, 0},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
	                                                {0, 0, 255},
	                                                /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
	                                                {255, 0, 255}}};
	const SDL_MessageBoxData messageboxdata = {
	    SDL_MESSAGEBOX_ERROR,                                               /* .flags */
	    nullptr,                                                            /* .window */
	    (CAppContainer::getInstance()->gameConfig.name + " Error").c_str(), /* .title */
	    msg,                                                                /* .message */
	    SDL_arraysize(buttons),                                             /* .numbuttons */
	    buttons,                                                            /* .buttons */
	    &colorScheme                                                        /* .colorScheme */
	};
	SDL_ShowMessageBox(&messageboxdata, nullptr);
}

static void dumpCrashLog(const char* msg) {
	FILE* crashFile = fopen("crash.log", "w");
	if (crashFile) {
		fprintf(crashFile, "=== FATAL ERROR ===\n%s\n", msg);
		logDumpRecentToFile(crashFile, 64);
		fclose(crashFile);
	}
}

void Error(const char* fmt, ...) {
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
#ifdef _WIN32
	vsprintf_s(errMsg, sizeof(errMsg), fmt, ap);
#else
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
#endif
	va_end(ap);

	LOG_ERROR("[FATAL] {}\n", errMsg);
	dumpCrashLog(errMsg);
	logShutdown();
	showFatalMessageBox(errMsg);
	exit(1);
}

[[noreturn]] void fatalErrorImpl(const std::string& msg, std::source_location loc) {
	std::string full = std::format("[FATAL {}:{} {}] {}",
		loc.file_name(), loc.line(), loc.function_name(), msg);
	LOG_ERROR("{}\n", full);
	dumpCrashLog(full.c_str());
	logShutdown();
	showFatalMessageBox(msg.c_str());
	exit(1);
}
