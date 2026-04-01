#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>

#include "ZipFile.h"
#include "d2-converter.h"
#include "d1-converter.h"

// ========================================================================
// Utility function implementations (shared across converters)
// ========================================================================

bool dirExists(const char* path) {
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool mkdirRecursive(const std::string& path) {
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

bool writeFile(const std::string& path, const uint8_t* data, int size) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f)
		return false;
	fwrite(data, 1, size, f);
	fclose(f);
	return true;
}

bool writeString(const std::string& path, const std::string& content) {
	FILE* f = fopen(path.c_str(), "w");
	if (!f)
		return false;
	fwrite(content.data(), 1, content.size(), f);
	fclose(f);
	return true;
}

int16_t readShort(const uint8_t* data, int offset) {
	return (int16_t)(data[offset] | (data[offset + 1] << 8));
}

uint16_t readUShort(const uint8_t* data, int offset) {
	return (uint16_t)(data[offset] | (data[offset + 1] << 8));
}

int32_t readInt(const uint8_t* data, int offset) {
	return (int32_t)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

uint32_t readUInt(const uint8_t* data, int offset) {
	return (uint32_t)(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24));
}

std::string escapeString(const uint8_t* raw, int len) {
	std::string result;
	for (int i = 0; i < len; i++) {
		uint8_t ch = raw[i];
		if (ch == '"') {
			result += "\\\"";
		} else if (ch == '\\') {
			result += "\\\\";
		} else if (ch == 0x0A) {
			result += "\\n";
		} else if (ch == 0x0D) {
			result += "\\r";
		} else if (ch == 0x09) {
			result += "\\t";
		} else if (ch >= 0x80 && ch <= 0x9F) {
			char buf[8];
			snprintf(buf, sizeof(buf), "\\x%02X", ch);
			result += buf;
		} else if (ch >= 0xA0) {
			char buf[8];
			snprintf(buf, sizeof(buf), "\\x%02X", ch);
			result += buf;
		} else {
			result += (char)ch;
		}
	}
	return result;
}

// ========================================================================
// Main
// ========================================================================
static void printUsage(const char* progName) {
	printf("Usage: %s --ipa <path> [--game <id>] [--output <dir>]\n", progName);
	printf("\n");
	printf("  --ipa <path>     Path to game .ipa file\n");
	printf("  --game <id>      Game to convert: doom2rpg (default), doom1rpg\n");
	printf("  --output <dir>   Output directory (default: games/<game_id>)\n");
}

int main(int argc, char* argv[]) {
	const char* ipaPath = nullptr;
	const char* gameId = nullptr;
	const GameDef* game = &GAME_DOOM2RPG;
	std::string outputDir;
	bool force = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--ipa") == 0 && i + 1 < argc) {
			ipaPath = argv[++i];
		} else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
			gameId = argv[++i];
		} else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
			outputDir = argv[++i];
		} else if (strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "-f") == 0) {
			force = true;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printUsage(argv[0]);
			return 0;
		}
	}

	// Select game
	if (gameId) {
		if (strcmp(gameId, "doom2rpg") == 0 || strcmp(gameId, "d2rpg") == 0) {
			game = &GAME_DOOM2RPG;
		} else if (strcmp(gameId, "doom1rpg") == 0 || strcmp(gameId, "d1rpg") == 0) {
			game = &GAME_DOOM1RPG;
		} else {
			fprintf(stderr, "Error: unknown game '%s'. Use doom2rpg or doom1rpg.\n", gameId);
			return 1;
		}
	}

	if (outputDir.empty()) {
		outputDir = game->defaultOutput;
	}

	if (!ipaPath) {
		fprintf(stderr, "Error: --ipa <path> is required\n\n");
		printUsage(argv[0]);
		return 1;
	}

	// Check if output already exists
	if (dirExists(outputDir.c_str()) && !force) {
		fprintf(stderr, "Output directory '%s' already exists. Use --force to overwrite.\n", outputDir.c_str());
		return 1;
	}

	// Open IPA
	printf("Opening IPA: %s\n", ipaPath);
	printf("Game: %s\n", game->name);
	ZipFile zip;
	zip.openZipFile(ipaPath);

	// Create output directory structure
	printf("Creating output directory: %s\n", outputDir.c_str());
	if (!mkdirRecursive(outputDir)) {
		fprintf(stderr, "Error: failed to create directory '%s'\n", outputDir.c_str());
		return 1;
	}

	// Both games use the same D2RPG converter for now.
	// D1RPG-specific logic will be added later.
	bool ok = convertD2RPG(zip, *game, outputDir);

	zip.closeZipFile();

	if (ok) {
		printf("\nConversion complete: %s\n", outputDir.c_str());
		printf("Run the game with: ./DRPGEngine --game %s\n", game->id);
	} else {
		printf("\nConversion completed with errors.\n");
	}

	return ok ? 0 : 1;
}
