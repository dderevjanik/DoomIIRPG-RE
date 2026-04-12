#include "SpriteLoader.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>

// ─── Helpers ────────────────────────────────────────────────────────────────

static bool readFile(const std::string& path, std::vector<uint8_t>& out) {
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	if (!f) return false;
	auto size = f.tellg();
	f.seekg(0);
	out.resize(static_cast<size_t>(size));
	f.read(reinterpret_cast<char*>(out.data()), size);
	return f.good();
}

static int16_t readLE16(const uint8_t* p) {
	return static_cast<int16_t>(p[0] | (p[1] << 8));
}

static int32_t readLE32(const uint8_t* p) {
	return static_cast<int32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static inline bool isTransparentColor(uint8_t r, uint8_t g, uint8_t b) {
	return (r >= 250 && b >= 250 && g <= 4);
}

// ─── SpriteLoader ───────────────────────────────────────────────────────────

SpriteLoader::~SpriteLoader() {
	for (int i = 0; i < MAX_MEDIA; i++) {
		if (textures_[i]) {
			glDeleteTextures(1, &textures_[i]);
		}
	}
}

bool SpriteLoader::load(const std::string& gameDir) {
	std::string texDir = gameDir + "/levels/textures";

	// ── Load newMappings.bin ──
	std::vector<uint8_t> mappingsData;
	if (!readFile(texDir + "/newMappings.bin", mappingsData)) {
		std::fprintf(stderr, "SpriteLoader: cannot read newMappings.bin\n");
		return false;
	}

	// Parse: int16[512] mediaMappings
	size_t pos = 0;
	for (int i = 0; i < MAX_MAPPINGS && pos + 2 <= mappingsData.size(); i++, pos += 2)
		mediaMappings_[i] = readLE16(&mappingsData[pos]);

	// uint8[1024] mediaDimensions
	for (int i = 0; i < MAX_MEDIA && pos < mappingsData.size(); i++, pos++)
		mediaDimensions_[i] = mappingsData[pos];

	// int16[4096] mediaBounds (4 shorts per media = 4*1024)
	for (int i = 0; i < MAX_MEDIA * 4 && pos + 2 <= mappingsData.size(); i++, pos += 2)
		mediaBounds_[i] = readLE16(&mappingsData[pos]);

	// int32[1024] mediaPalColors
	for (int i = 0; i < MAX_MEDIA && pos + 4 <= mappingsData.size(); i++, pos += 4)
		mediaPalColors_[i] = readLE32(&mappingsData[pos]);

	// int32[1024] mediaTexelSizes
	for (int i = 0; i < MAX_MEDIA && pos + 4 <= mappingsData.size(); i++, pos += 4)
		mediaTexelSizes_[i] = readLE32(&mappingsData[pos]);

	// ── Load palettes ──
	std::vector<uint8_t> palData;
	if (!readFile(texDir + "/newPalettes.bin", palData)) {
		std::fprintf(stderr, "SpriteLoader: cannot read newPalettes.bin\n");
		return false;
	}

	size_t palPos = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		int numColors = mediaPalColors_[i] & 0x3FFFFFFF;
		bool isRef = (mediaPalColors_[i] & FLAG_REFERENCE) != 0;

		if (!isRef && numColors > 0 && palPos + numColors * 2 <= palData.size()) {
			palettes_[i].resize(numColors);
			for (int c = 0; c < numColors; c++) {
				palettes_[i][c] = static_cast<uint16_t>(
					palData[palPos + c * 2] | (palData[palPos + c * 2 + 1] << 8));
			}
		}

		if (!isRef) {
			palPos += numColors * 2 + 4; // +4 for marker
		}
	}

	// Resolve palette references
	for (int i = 0; i < MAX_MEDIA; i++) {
		if ((mediaPalColors_[i] & FLAG_REFERENCE) != 0) {
			int refID = mediaPalColors_[i] & 0x3FF;
			if (refID < MAX_MEDIA && !palettes_[refID].empty()) {
				palettes_[i] = palettes_[refID];
			}
		}
	}

	// ── Load texels ──
	// Load all texel files
	std::vector<std::vector<uint8_t>> texelFiles;
	for (int f = 0; f < 100; f++) {
		auto fname = std::format("{}/newTexels{:03d}.bin", texDir, f);
		std::vector<uint8_t> td;
		if (!readFile(fname, td)) break;
		texelFiles.push_back(std::move(td));
	}

	int fileIdx = 0;
	size_t fileOffset = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		bool isRef = (mediaTexelSizes_[i] & FLAG_REFERENCE) != 0;
		if (isRef) continue;

		int size = (mediaTexelSizes_[i] & 0x3FFFFFFF) + 1;

		if (fileIdx < (int)texelFiles.size() &&
			fileOffset + size <= texelFiles[fileIdx].size()) {
			texels_[i].assign(
				texelFiles[fileIdx].begin() + fileOffset,
				texelFiles[fileIdx].begin() + fileOffset + size);
		}

		fileOffset += size + 4; // +4 for marker

		if (fileOffset > 0x40000) {
			fileIdx++;
			fileOffset = 0;
		}
	}

	// Resolve texel references
	for (int i = 0; i < MAX_MEDIA; i++) {
		if ((mediaTexelSizes_[i] & FLAG_REFERENCE) != 0) {
			int refID = mediaTexelSizes_[i] & 0x3FF;
			if (refID < MAX_MEDIA && !texels_[refID].empty()) {
				texels_[i] = texels_[refID];
			}
		}
	}

	// Count loaded media
	loadedMediaCount_ = 0;
	for (int i = 0; i < MAX_MEDIA; i++) {
		if (!texels_[i].empty() && !palettes_[i].empty())
			loadedMediaCount_++;
	}

	std::fprintf(stderr, "SpriteLoader: loaded %d media entries from %zu texel files\n",
	             loadedMediaCount_, texelFiles.size());
	return true;
}

GLuint SpriteLoader::getTexture(int mediaID) {
	if (mediaID < 0 || mediaID >= MAX_MEDIA) return 0;
	if (textures_[mediaID]) return textures_[mediaID];
	if (texels_[mediaID].empty() || palettes_[mediaID].empty()) return 0;
	textures_[mediaID] = createTexture(mediaID);
	return textures_[mediaID];
}

GLuint SpriteLoader::getTextureForTile(int tileNum, int frame) {
	if (tileNum < 0 || tileNum >= MAX_MAPPINGS) return 0;
	int mediaID = mediaMappings_[tileNum] + frame;
	if (mediaID < 0 || mediaID >= MAX_MEDIA) return 0;
	return getTexture(mediaID);
}

int SpriteLoader::getFrameCount(int tileNum) const {
	if (tileNum < 0 || tileNum >= MAX_MAPPINGS - 1) return 0;
	int start = mediaMappings_[tileNum];
	int end = mediaMappings_[tileNum + 1];
	if (start < 0 || end <= start) return 1;
	return end - start;
}

void SpriteLoader::getDimensions(int mediaID, int& w, int& h) const {
	if (mediaID < 0 || mediaID >= MAX_MEDIA) { w = h = 0; return; }
	uint8_t dim = mediaDimensions_[mediaID];
	w = 1 << ((dim >> 4) & 0xF);
	h = 1 << (dim & 0xF);
}

// ─── Texture Creation ───────────────────────────────────────────────────────

void SpriteLoader::buildRGBAPalette(const std::vector<uint16_t>& rgb565,
                                     uint32_t* rgba, bool& hasTrans) {
	hasTrans = false;
	for (int i = 0; i < (int)rgb565.size() && i < 256; i++) {
		uint16_t c = rgb565[i];
		uint8_t r5 = (c >> 11) & 0x1F;
		uint8_t g6 = (c >> 5) & 0x3F;
		uint8_t b5 = c & 0x1F;
		uint8_t r8 = (r5 << 3) | (r5 >> 2);
		uint8_t g8 = (g6 << 2) | (g6 >> 4);
		uint8_t b8 = (b5 << 3) | (b5 >> 2);

		if (isTransparentColor(r8, g8, b8)) {
			rgba[i] = 0; // fully transparent
			hasTrans = true;
		} else {
			rgba[i] = (0xFF << 24) | (b8 << 16) | (g8 << 8) | r8; // ABGR for GL_RGBA
		}
	}
}

void SpriteLoader::decodeFull(int mediaID, const uint8_t* texels,
                               const uint32_t* rgba_pal,
                               int w, int h, uint32_t* pixels) {
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			uint8_t idx = texels[y * w + x];
			pixels[y * w + x] = rgba_pal[idx];
		}
	}
}

void SpriteLoader::decodeSprite(int mediaID, const uint8_t* texels, int texelSize,
                                 const uint32_t* rgba_pal, bool hasTrans,
                                 int w, int h, uint32_t* pixels) {
	// Clear to transparent
	std::memset(pixels, 0, w * h * sizeof(uint32_t));

	if (texelSize < 2) return;

	int shapeOffset = (texels[texelSize - 1] << 8) | texels[texelSize - 2];
	int pixelEnd = texelSize - shapeOffset - 2;

	int shapeMin = mediaBounds_[4 * mediaID + 0] & 0xFF;
	int shapeMax = mediaBounds_[4 * mediaID + 1] & 0xFF;

	if (shapeMax > w || shapeMin > shapeMax) return;

	int shapeTableStart = pixelEnd + ((shapeMax - shapeMin + 1) >> 1);
	int pixelPos = 0;

	for (int colIdx = 0; colIdx < shapeMax - shapeMin; colIdx++) {
		int x = shapeMin + colIdx;

		// Read number of runs from shape table (4-bit nibbles)
		int nibbleOffset = pixelEnd + (colIdx >> 1);
		if (nibbleOffset >= texelSize) break;

		int numRuns;
		if (colIdx & 1)
			numRuns = (texels[nibbleOffset] >> 4) & 0xF;
		else
			numRuns = texels[nibbleOffset] & 0xF;

		for (int r = 0; r < numRuns; r++) {
			if (shapeTableStart + 1 >= texelSize) break;
			int yStart = texels[shapeTableStart];
			int runHeight = texels[shapeTableStart + 1];
			shapeTableStart += 2;

			for (int dy = 0; dy < runHeight; dy++) {
				if (pixelPos >= pixelEnd) break;
				uint8_t idx = texels[pixelPos++];
				int y = yStart + dy;
				if (x >= 0 && x < w && y >= 0 && y < h)
					pixels[y * w + x] = rgba_pal[idx];
			}
		}
	}
}

GLuint SpriteLoader::createTexture(int mediaID) {
	int w, h;
	getDimensions(mediaID, w, h);
	if (w == 0 || h == 0) return 0;

	const auto& texelData = texels_[mediaID];
	const auto& palette = palettes_[mediaID];
	if (texelData.empty() || palette.empty()) return 0;

	int texelSize = (int)texelData.size();

	// Skip trivial entries
	uint8_t dim = mediaDimensions_[mediaID];
	if (dim == 0 && texelSize <= 1) return 0;

	// Build RGBA palette
	uint32_t rgbaPal[256] = {};
	bool hasTrans = false;
	buildRGBAPalette(palette, rgbaPal, hasTrans);

	// Decode pixels
	std::vector<uint32_t> pixels(w * h, 0);
	bool isFullTexture = (texelSize == w * h);

	if (isFullTexture) {
		decodeFull(mediaID, texelData.data(), rgbaPal, w, h, pixels.data());
	} else {
		decodeSprite(mediaID, texelData.data(), texelSize, rgbaPal, hasTrans, w, h, pixels.data());
	}

	// Upload to OpenGL
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	return tex;
}

// ─── Public helpers ─────────────────────────────────────────────────────────

int SpriteLoader::getMediaID(int tileNum, int frame) const {
	if (tileNum < 0 || tileNum >= MAX_MAPPINGS) return -1;
	int mid = mediaMappings_[tileNum] + frame;
	if (mid < 0 || mid >= MAX_MEDIA) return -1;
	return mid;
}

// ─── Decode to buffer (no GL) ───────────────────────────────────────────────

bool SpriteLoader::decodeToBuffer(int mediaID, std::vector<uint32_t>& pixels, int& w, int& h) {
	if (mediaID < 0 || mediaID >= MAX_MEDIA) return false;

	getDimensions(mediaID, w, h);
	if (w == 0 || h == 0) return false;

	const auto& texelData = texels_[mediaID];
	const auto& palette = palettes_[mediaID];
	if (texelData.empty() || palette.empty()) return false;

	int texelSize = (int)texelData.size();
	uint8_t dim = mediaDimensions_[mediaID];
	if (dim == 0 && texelSize <= 1) return false;

	uint32_t rgbaPal[256] = {};
	bool hasTrans = false;
	buildRGBAPalette(palette, rgbaPal, hasTrans);

	pixels.assign(w * h, 0);
	bool isFullTexture = (texelSize == w * h);

	if (isFullTexture)
		decodeFull(mediaID, texelData.data(), rgbaPal, w, h, pixels.data());
	else
		decodeSprite(mediaID, texelData.data(), texelSize, rgbaPal, hasTrans, w, h, pixels.data());

	return true;
}

// ─── Blit overlay ───────────────────────────────────────────────────────────

void SpriteLoader::blitOver(const uint32_t* src, int sw, int sh,
                             uint32_t* dst, int dw, int dh, int dx, int dy) {
	for (int sy = 0; sy < sh; sy++) {
		int dstY = dy + sy;
		if (dstY < 0 || dstY >= dh) continue;
		for (int sx = 0; sx < sw; sx++) {
			int dstX = dx + sx;
			if (dstX < 0 || dstX >= dw) continue;
			uint32_t pixel = src[sy * sw + sx];
			uint8_t alpha = (pixel >> 24) & 0xFF;
			if (alpha > 0)
				dst[dstY * dw + dstX] = pixel;
		}
	}
}

// ─── Composite monster ──────────────────────────────────────────────────────

GLuint SpriteLoader::getCompositeMonster(int tileNum, const BodyPartOffsets& offsets,
                                          int legsFrame, int torsoFrame, int headFrame) {
	// Build cache key from tile + frames + offsets
	uint64_t key = ((uint64_t)tileNum << 40) |
	               ((uint64_t)(legsFrame & 0xF) << 36) |
	               ((uint64_t)(torsoFrame & 0xF) << 32) |
	               ((uint64_t)(headFrame & 0xF) << 28) |
	               ((uint64_t)((offsets.torsoZ + 512) & 0x3FF) << 18) |
	               ((uint64_t)((offsets.headZ + 512) & 0x3FF) << 8) |
	               ((uint64_t)((offsets.headX + 128) & 0xFF));

	if (auto it = compositeCache_.find(key); it != compositeCache_.end()) return it->second;

	// Decode each body part
	int legsMediaID = getMediaID(tileNum, legsFrame);
	int torsoMediaID = getMediaID(tileNum, torsoFrame);
	int headMediaID = getMediaID(tileNum, headFrame);

	std::vector<uint32_t> legsPx, torsoPx, headPx;
	int lw, lh, tw, th, hw, hh;

	bool hasLegs = decodeToBuffer(legsMediaID, legsPx, lw, lh);
	bool hasTorso = decodeToBuffer(torsoMediaID, torsoPx, tw, th);
	bool hasHead = decodeToBuffer(headMediaID, headPx, hw, hh);

	if (!hasLegs && !hasTorso && !hasHead) {
		compositeCache_[key] = 0;
		return 0;
	}

	// The engine uses a coordinate system where Z goes up (positive = higher).
	// In pixel coordinates, Y goes down. The Z offsets are in engine units
	// where ~16 units = 1 pixel (values are <<4 shifted).
	// Convert engine Z offsets to pixel offsets:
	//   pixel_offset_up = engineZ / 16
	// In our image, "up" means lower Y, so dy = -pixel_offset_up

	int torsoPixelUp = offsets.torsoZ / 16;
	int headPixelUp = offsets.headZ / 16;
	int headPixelRight = offsets.headX; // already in pixel-ish units

	// Determine composite canvas size.
	// Use the widest part as base width.
	// For height, we need to accommodate all parts with their offsets.
	int baseW = lw;
	if (tw > baseW) baseW = tw;
	if (hw > baseW) baseW = hw;

	// Vertical layout: legs at bottom, torso above, head above torso.
	// We'll center everything horizontally and position vertically.
	//
	// Without offsets, all parts render at the same base position (feet level).
	// The Z offset moves parts UP (visually higher = lower Y in image).
	//
	// We need to figure out the total vertical extent.
	// Parts are centered on the same base x/y in the engine, with Z controlling height.
	// Each sprite's visual content fills its full frame dimensions.

	// Calculate vertical extents (Y in image space, 0 = top)
	// Legs anchored at bottom of canvas
	int legsTop = 0;
	int torsoTop = -torsoPixelUp;    // negative offset = higher up = lower Y
	int headTop = -headPixelUp;

	// Find the minimum Y (highest point) and maximum extent
	int minTop = 0;
	if (hasLegs) minTop = std::min(minTop, legsTop);
	if (hasTorso) minTop = std::min(minTop, torsoTop);
	if (hasHead) minTop = std::min(minTop, headTop);

	// Shift everything so the highest part starts at Y=0
	int shiftY = -minTop;
	legsTop += shiftY;
	torsoTop += shiftY;
	headTop += shiftY;

	// Canvas height = max bottom edge
	int canvasH = 0;
	if (hasLegs) canvasH = std::max(canvasH, legsTop + lh);
	if (hasTorso) canvasH = std::max(canvasH, torsoTop + th);
	if (hasHead) canvasH = std::max(canvasH, headTop + hh);

	if (canvasH <= 0) canvasH = lh;

	int canvasW = baseW;
	// Account for head lateral offset
	if (hasHead) {
		int headLeft = (canvasW - hw) / 2 + headPixelRight;
		int headRight = headLeft + hw;
		if (headLeft < 0) {
			canvasW += -headLeft;
			// Re-center everything
		}
		if (headRight > canvasW) canvasW = headRight;
	}

	// Composite buffer
	std::vector<uint32_t> composite(canvasW * canvasH, 0);

	// Center each part horizontally, blit in order: legs, torso, head
	if (hasLegs) {
		int dx = (canvasW - lw) / 2;
		blitOver(legsPx.data(), lw, lh, composite.data(), canvasW, canvasH, dx, legsTop);
	}
	if (hasTorso) {
		int dx = (canvasW - tw) / 2;
		blitOver(torsoPx.data(), tw, th, composite.data(), canvasW, canvasH, dx, torsoTop);
	}
	if (hasHead) {
		int dx = (canvasW - hw) / 2 + headPixelRight;
		blitOver(headPx.data(), hw, hh, composite.data(), canvasW, canvasH, dx, headTop);
	}

	// Upload to GL
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvasW, canvasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, composite.data());

	compositeCache_[key] = tex;
	return tex;
}

// ─── Composite pose ─────────────────────────────────────────────────────────

GLuint SpriteLoader::getCompositePose(int tileNum, int pose,
                                       const BodyPartOffsets& offsets, int walkFrame) {
	// Pose frame mapping based on engine's renderSpriteAnim:
	// Standard layout per tile:
	//   Frame 0: legs idle front
	//   Frame 1: legs walk step front
	//   Frame 2: torso front
	//   Frame 3: head front
	//   Frame 4: legs idle back
	//   Frame 5: legs walk step back
	//   Frame 6: torso back
	//   Frame 7: head back
	//   Frame 8: attack1 torso frame 0
	//   Frame 9: attack1 torso frame 1
	//   Frame 10: attack2 torso frame 0
	//   Frame 11: attack2 torso frame 1
	//   Frame 12: pain (full sprite)
	//   Frame 13: death (full sprite)

	int legsF, torsoF, headF;

	switch (pose) {
		default:
		case 0: // idle front
			legsF = 0; torsoF = 2; headF = 3;
			break;
		case 1: // walk front
			legsF = walkFrame & 1; torsoF = 2; headF = 3;
			break;
		case 2: // attack1
			legsF = 0; torsoF = 8 + (walkFrame & 1); headF = 3;
			break;
		case 3: // attack2
			legsF = 0; torsoF = 10 + (walkFrame & 1); headF = 3;
			break;
		case 4: // idle back
			legsF = 4; torsoF = 6; headF = 7;
			break;
		case 5: // walk back
			legsF = 4 + (walkFrame & 1); torsoF = 6; headF = 7;
			break;
	}

	return getCompositeMonster(tileNum, offsets, legsF, torsoF, headF);
}
