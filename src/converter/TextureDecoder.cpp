#include "TextureDecoder.h"
#include "converter.h"

#include <cstdio>
#include <cstring>
#include <format>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::vector<uint8_t> readBinaryFile(const std::string& path) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return {};
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	std::vector<uint8_t> buf(sz);
	fread(buf.data(), 1, sz, f);
	fclose(f);
	return buf;
}

void TextureDecoder::rgb565ToRGBA(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
	uint8_t r5 = (c >> 11) & 0x1F;
	uint8_t g6 = (c >> 5) & 0x3F;
	uint8_t b5 = c & 0x1F;
	r = (r5 << 3) | (r5 >> 2);
	g = (g6 << 2) | (g6 >> 4);
	b = (b5 << 3) | (b5 >> 2);
	// Magenta = transparent
	if (r >= 250 && b >= 250 && g <= 4)
		a = 0;
	else
		a = 255;
}

// ---------------------------------------------------------------------------
// Load binary data from texture directory
// ---------------------------------------------------------------------------
bool TextureDecoder::load(const std::string& texDir, int numTexFiles) {
	// --- newMappings.bin ---
	auto mdata = readBinaryFile(texDir + "/newMappings.bin");
	if (mdata.empty()) {
		fprintf(stderr, "  TextureDecoder: failed to read newMappings.bin\n");
		return false;
	}

	int pos = 0;
	for (int i = 0; i < MAX_MAPPINGS; i++) {
		mediaMappings[i] = readShort(mdata.data(), pos + i * 2);
	}
	pos = MAX_MAPPINGS * 2;

	memcpy(mediaDimensions, mdata.data() + pos, MAX_MEDIA);
	pos += MAX_MEDIA;

	for (int i = 0; i < MAX_MEDIA * 4; i++) {
		mediaBounds[i] = readShort(mdata.data(), pos + i * 2);
	}
	pos += MAX_MEDIA * 4 * 2;

	for (int i = 0; i < MAX_MEDIA; i++) {
		mediaPalColors[i] = readInt(mdata.data(), pos + i * 4);
	}
	pos += MAX_MEDIA * 4;

	for (int i = 0; i < MAX_MEDIA; i++) {
		mediaTexelSizes[i] = readInt(mdata.data(), pos + i * 4);
	}

	// --- newPalettes.bin ---
	auto palData = readBinaryFile(texDir + "/newPalettes.bin");
	if (palData.empty()) {
		fprintf(stderr, "  TextureDecoder: failed to read newPalettes.bin\n");
		return false;
	}

	// Pass 1: read non-reference palettes
	int palPos = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		int numColors = mediaPalColors[i] & 0x3FFFFFFF;
		bool isRef = (mediaPalColors[i] & FLAG_REFERENCE) != 0;
		if (!isRef && numColors > 0 && palPos + numColors * 2 <= (int)palData.size()) {
			std::vector<uint16_t> pal(numColors);
			for (int c = 0; c < numColors; c++) {
				pal[c] = readUShort(palData.data(), palPos + c * 2);
			}
			palettes[i] = std::move(pal);
		}
		if (!isRef) {
			palPos += numColors * 2 + 4;
		}
	}

	// Pass 2: resolve palette references
	for (int i = 0; i < MAX_MEDIA; i++) {
		if ((mediaPalColors[i] & FLAG_REFERENCE) != 0) {
			int ref = mediaPalColors[i] & 0x3FF;
			if (auto it = palettes.find(ref); it != palettes.end()) {
				palettes[i] = it->second;
			}
		}
	}

	// --- newTexels*.bin ---
	std::map<int, std::vector<uint8_t>> texelFiles;
	for (int fi = 0; fi < numTexFiles; fi++) {
		auto path = texDir + "/" + std::format("newTexels{:03d}.bin", fi);
		auto data = readBinaryFile(path);
		if (!data.empty()) {
			texelFiles[fi] = std::move(data);
		}
	}

	// Pass 1: read non-reference texels
	int fileIdx = 0;
	int fileOffset = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		if ((mediaTexelSizes[i] & FLAG_REFERENCE) != 0) {
			continue;
		}
		int size = (mediaTexelSizes[i] & 0x3FFFFFFF) + 1;
		if (auto it = texelFiles.find(fileIdx); it != texelFiles.end()) {
			const auto& data = it->second;
			if (fileOffset + size <= (int)data.size()) {
				texels[i] = std::vector<uint8_t>(data.begin() + fileOffset,
				                                  data.begin() + fileOffset + size);
			}
		}
		fileOffset += size + 4;
		if (fileOffset > 0x40000) {
			fileIdx++;
			fileOffset = 0;
		}
	}

	// Pass 2: resolve texel references
	for (int i = 0; i < MAX_MEDIA; i++) {
		if ((mediaTexelSizes[i] & FLAG_REFERENCE) != 0) {
			int ref = mediaTexelSizes[i] & 0x3FF;
			if (auto it = texels.find(ref); it != texels.end()) {
				texels[i] = it->second;
			}
		}
	}

	printf("  TextureDecoder: loaded %d palettes, %d texels from %d files\n",
	       (int)palettes.size(), (int)texels.size(), (int)texelFiles.size());
	return true;
}

// ---------------------------------------------------------------------------
// Decode full texture (texelSize == w*h)
// ---------------------------------------------------------------------------
void TextureDecoder::decodeFull(int mid, int w, int h, const uint8_t* texelData,
                                const std::vector<uint16_t>& palette, std::vector<uint8_t>& rgba) {
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint8_t idx = texelData[y * w + x];
			int pixel = (y * w + x) * 4;
			if (idx < (int)palette.size()) {
				rgb565ToRGBA(palette[idx], rgba[pixel], rgba[pixel + 1],
				             rgba[pixel + 2], rgba[pixel + 3]);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Decode RLE sprite
// ---------------------------------------------------------------------------
void TextureDecoder::decodeRLE(int mid, int w, int h, const uint8_t* texelData, int texelSize,
                               const std::vector<uint16_t>& palette, std::vector<uint8_t>& rgba) {
	int shapeOffset = (texelData[texelSize - 1] << 8) | texelData[texelSize - 2];
	int pixelEnd = texelSize - shapeOffset - 2;
	int shapeMin = mediaBounds[4 * mid] & 0xFF;
	int shapeMax = mediaBounds[4 * mid + 1] & 0xFF;

	if (shapeMax > w || shapeMin > shapeMax) return;

	int shapeTableStart = pixelEnd + ((shapeMax - shapeMin + 1) >> 1);
	int pixelPos = 0;

	for (int colIdx = 0; colIdx < shapeMax - shapeMin; colIdx++) {
		int x = shapeMin + colIdx;
		int noff = pixelEnd + (colIdx >> 1);
		if (noff >= texelSize) break;

		int numRuns;
		if (colIdx & 1)
			numRuns = (texelData[noff] >> 4) & 0xF;
		else
			numRuns = texelData[noff] & 0xF;

		for (int r = 0; r < numRuns; r++) {
			if (shapeTableStart + 1 >= texelSize) break;
			int yStart = texelData[shapeTableStart];
			int runH = texelData[shapeTableStart + 1];
			shapeTableStart += 2;

			for (int dy = 0; dy < runH; dy++) {
				if (pixelPos >= pixelEnd) break;
				uint8_t idx = texelData[pixelPos];
				pixelPos++;
				int y = yStart + dy;
				if (x >= 0 && x < w && y >= 0 && y < h && idx < (int)palette.size()) {
					int pixel = (y * w + x) * 4;
					rgb565ToRGBA(palette[idx], rgba[pixel], rgba[pixel + 1],
					             rgba[pixel + 2], rgba[pixel + 3]);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Decode all media entries
// ---------------------------------------------------------------------------
bool TextureDecoder::decode() {
	decoded.clear();

	for (auto& [mid, texelData] : texels) {
		if (palettes.find(mid) == palettes.end()) continue;

		uint8_t dim = mediaDimensions[mid];
		int w = 1 << ((dim >> 4) & 0xF);
		int h = 1 << (dim & 0xF);
		if (w == 0 || h == 0) continue;

		int texelSize = (int)texelData.size();
		if (dim == 0 && texelSize <= 1) continue;

		const auto& palette = palettes[mid];
		std::vector<uint8_t> rgba(w * h * 4, 0); // initialized to transparent black

		if (texelSize == w * h) {
			decodeFull(mid, w, h, texelData.data(), palette, rgba);
		} else if (texelSize >= 2) {
			decodeRLE(mid, w, h, texelData.data(), texelSize, palette, rgba);
		} else {
			continue;
		}

		DecodedMedia dm;
		dm.mediaId = mid;
		dm.width = w;
		dm.height = h;
		dm.rgba = std::move(rgba);
		decoded[mid] = std::move(dm);
	}

	printf("  TextureDecoder: decoded %d media entries\n", (int)decoded.size());
	return true;
}

const DecodedMedia* TextureDecoder::getMedia(int mediaId) const {
	if (auto it = decoded.find(mediaId); it != decoded.end()) {
		return &it->second;
	}
	return nullptr;
}
