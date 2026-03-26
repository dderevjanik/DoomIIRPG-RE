#pragma once

#include <SDL_opengl.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Loads media textures from the binary format (newMappings.bin, newPalettes.bin,
// newTexelsNNN.bin) and creates OpenGL textures suitable for ImGui::Image().
class SpriteLoader {
public:
	~SpriteLoader();

	// Load all binary data from the game directory
	bool load(const std::string& gameDir);

	// Get or create an OpenGL texture for a given media ID.
	// Returns 0 if the media ID is invalid or has no data.
	GLuint getTexture(int mediaID);

	// Get texture for a tile number + frame offset.
	// Frame 0 = base sprite, frame 1+ = animation frames.
	GLuint getTextureForTile(int tileNum, int frame = 0);

	// How many frames does a tile have?
	int getFrameCount(int tileNum) const;

	// Get sprite dimensions for a media ID
	void getDimensions(int mediaID, int& w, int& h) const;

	// Get total number of media entries loaded
	int mediaCount() const { return loadedMediaCount_; }

	// Get the media ID for a tile+frame (exposes the mapping)
	int getMediaID(int tileNum, int frame = 0) const;

	// Composite monster sprite: overlays legs (frame 0) + torso (frame 2) +
	// head (frame 3) into a single texture. Body part Z-offsets shift parts
	// vertically. Returns cached GL texture.
	struct BodyPartOffsets {
		int torsoZ;
		int headZ;
		int headX;
		BodyPartOffsets() : torsoZ(0), headZ(0), headX(0) {}
		BodyPartOffsets(int tz, int hz, int hx) : torsoZ(tz), headZ(hz), headX(hx) {}
	};
	GLuint getCompositeMonster(int tileNum, const BodyPartOffsets& offsets,
	                           int legsFrame = 0, int torsoFrame = 2, int headFrame = 3);

	// Composite for a specific animation pose
	// anim: 0=idle, 1=walk, 2=attack1, 3=attack2, 4=idle_back, 5=walk_back
	GLuint getCompositePose(int tileNum, int pose, const BodyPartOffsets& offsets,
	                        int walkFrame = 0);

private:
	static constexpr int MAX_MAPPINGS = 512;
	static constexpr int MAX_MEDIA = 1024;
	static constexpr uint32_t FLAG_REFERENCE = 0x80000000;

	// Mappings data
	int16_t mediaMappings_[MAX_MAPPINGS] = {};
	uint8_t mediaDimensions_[MAX_MEDIA] = {};
	int16_t mediaBounds_[MAX_MEDIA * 4] = {};
	int32_t mediaPalColors_[MAX_MEDIA] = {};
	int32_t mediaTexelSizes_[MAX_MEDIA] = {};

	// Loaded palette and texel data
	std::vector<uint16_t> palettes_[MAX_MEDIA]; // RGB565 palettes per media ID
	std::vector<uint8_t> texels_[MAX_MEDIA];    // raw texel data per media ID

	// Cached GL textures (individual frames)
	GLuint textures_[MAX_MEDIA] = {};
	int loadedMediaCount_ = 0;

	// Cached composite textures (keyed by composite ID string)
	std::unordered_map<uint64_t, GLuint> compositeCache_;

	// Decode a media entry into RGBA pixels and upload as GL texture
	GLuint createTexture(int mediaID);

	// Decode full texture (grid)
	void decodeFull(int mediaID, const uint8_t* texels, const uint32_t* rgba_pal,
	                int w, int h, uint32_t* pixels);

	// Decode sprite texture (RLE columns)
	void decodeSprite(int mediaID, const uint8_t* texels, int texelSize,
	                  const uint32_t* rgba_pal, bool hasTrans,
	                  int w, int h, uint32_t* pixels);

	// Convert RGB565 palette to RGBA8888
	void buildRGBAPalette(const std::vector<uint16_t>& rgb565,
	                      uint32_t* rgba, bool& hasTrans);

	// Decode a media ID into an RGBA pixel buffer (does NOT create GL texture)
	bool decodeToBuffer(int mediaID, std::vector<uint32_t>& pixels, int& w, int& h);

	// Overlay src onto dst at pixel offset (dx, dy), skipping transparent pixels
	static void blitOver(const uint32_t* src, int sw, int sh,
	                     uint32_t* dst, int dw, int dh, int dx, int dy);
};
