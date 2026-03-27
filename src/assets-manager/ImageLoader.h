#pragma once

#include <SDL_opengl.h>
#include <string>
#include <unordered_map>

// Loads PNG/JPG/BMP images via stb_image and creates OpenGL textures.
// Caches loaded textures by file path to avoid redundant loads.
class ImageLoader {
public:
	~ImageLoader();

	// Load an image file and return its GL texture handle.
	// Returns 0 on failure. Results are cached by path.
	GLuint loadTexture(const std::string& path);

	// Load and also retrieve dimensions
	GLuint loadTexture(const std::string& path, int& outWidth, int& outHeight);

private:
	struct CachedTexture {
		GLuint id = 0;
		int width = 0;
		int height = 0;
	};

	std::unordered_map<std::string, CachedTexture> cache_;
};
