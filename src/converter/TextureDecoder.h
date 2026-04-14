#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

static constexpr int MAX_MAPPINGS = 512;
static constexpr int MAX_MEDIA = 1024;
static constexpr uint32_t FLAG_REFERENCE = 0x80000000;

struct DecodedMedia {
	int mediaId = 0;
	int width = 0;
	int height = 0;
	std::vector<uint8_t> rgba; // w*h*4 RGBA bytes
};

class TextureDecoder {
  public:
	// Load binary texture data from the textures directory
	// (expects newMappings.bin, newPalettes.bin, newTexels*.bin)
	bool load(const std::string& texDir, int numTexFiles);

	// Decode all media entries into RGBA images
	bool decode();

	// Get a decoded media entry by ID, or nullptr
	const DecodedMedia* getMedia(int mediaId) const;

	// Get the media mappings array (tile -> media ID)
	const int16_t* getMappings() const { return mediaMappings; }

	// Access raw metadata arrays
	const int32_t* getPalColors() const { return mediaPalColors; }
	const int32_t* getTexelSizes() const { return mediaTexelSizes; }

	// Access parsed palette/texel data (after load())
	const std::map<int, std::vector<uint16_t>>& getPalettes() const { return palettes; }
	const std::map<int, std::vector<uint8_t>>& getTexels() const { return texels; }

	// Convert RGB565 to RGBA (public for use by extraction code)
	static void rgb565ToRGBA(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);

  private:
	// Raw binary data
	int16_t mediaMappings[MAX_MAPPINGS] = {};
	uint8_t mediaDimensions[MAX_MEDIA] = {};
	int16_t mediaBounds[MAX_MEDIA * 4] = {};
	int32_t mediaPalColors[MAX_MEDIA] = {};
	int32_t mediaTexelSizes[MAX_MEDIA] = {};

	// Parsed palette/texel data
	std::map<int, std::vector<uint16_t>> palettes;    // mediaId -> RGB565 colors
	std::map<int, std::vector<uint8_t>> texels;        // mediaId -> raw texel bytes

	// Decoded RGBA images
	std::map<int, DecodedMedia> decoded;

	// Helpers
	void decodeFull(int mid, int w, int h, const uint8_t* texelData,
	                const std::vector<uint16_t>& palette, std::vector<uint8_t>& rgba);
	void decodeRLE(int mid, int w, int h, const uint8_t* texelData, int texelSize,
	               const std::vector<uint16_t>& palette, std::vector<uint8_t>& rgba);
};
