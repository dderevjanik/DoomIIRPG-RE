#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>

#include "ZipFile.h"

static void printUsage(const char* progName) {
	printf("Usage: %s --ipa <path> [--output <dir>]\n", progName);
	printf("\n");
	printf("  --ipa <path>     Path to Doom 2 RPG.ipa file\n");
	printf("  --output <dir>   Output directory (default: games/doom2rpg)\n");
}

static bool dirExists(const char* path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static bool mkdirRecursive(const std::string& path) {
	size_t pos = 0;
	while ((pos = path.find('/', pos + 1)) != std::string::npos) {
		std::string sub = path.substr(0, pos);
		if (!dirExists(sub.c_str())) {
			if (mkdir(sub.c_str(), 0755) != 0) {
				return false;
			}
		}
	}
	if (!dirExists(path.c_str())) {
		return mkdir(path.c_str(), 0755) == 0;
	}
	return true;
}

int main(int argc, char* argv[]) {
	const char* ipaPath = nullptr;
	std::string outputDir = "games/doom2rpg";

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--ipa") == 0 && i + 1 < argc) {
			ipaPath = argv[++i];
		} else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
			outputDir = argv[++i];
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printUsage(argv[0]);
			return 0;
		}
	}

	if (!ipaPath) {
		fprintf(stderr, "Error: --ipa <path> is required\n\n");
		printUsage(argv[0]);
		return 1;
	}

	// Check if output already exists
	if (dirExists(outputDir.c_str())) {
		fprintf(stderr, "Output directory '%s' already exists. Remove it first to re-convert.\n", outputDir.c_str());
		return 1;
	}

	// Open IPA
	printf("Opening IPA: %s\n", ipaPath);
	ZipFile zip;
	zip.openZipFile(ipaPath);

	// Create output directory structure
	printf("Creating output directory: %s\n", outputDir.c_str());
	if (!mkdirRecursive(outputDir)) {
		fprintf(stderr, "Error: failed to create directory '%s'\n", outputDir.c_str());
		return 1;
	}

	std::string soundsDir = outputDir + "/sounds";
	if (!mkdirRecursive(soundsDir)) {
		fprintf(stderr, "Error: failed to create directory '%s'\n", soundsDir.c_str());
		return 1;
	}

	// TODO Phase 1: Convert INI-ready assets
	//   - entities.bin -> entities.ini
	//   - tables.bin -> tables.ini
	//   - menus.bin -> menus.ini
	//   - strings.idx + strings00-02.bin -> strings.ini
	//   - sound files -> sounds/ + sounds.ini
	//   - generate game.ini
	printf("[Phase 1] Convert INI assets        ... TODO\n");

	// TODO Phase 2: Convert texture atlas
	//   - newMappings.bin + newPalettes.bin + newTexels000-038.bin -> media.dratlas
	printf("[Phase 2] Convert texture atlas      ... TODO\n");

	// TODO Phase 3: Convert maps and models
	//   - map00-09.bin -> map00-09.drmap
	//   - model_0000-0009.bin -> model_0000-0009.drmap
	printf("[Phase 3] Convert maps and models    ... TODO\n");

	zip.closeZipFile();

	printf("\nConversion complete: %s\n", outputDir.c_str());
	return 0;
}
