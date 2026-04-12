#include "ImageLoader.h"
#include <stb_image.h>
#include <cstdio>

ImageLoader::~ImageLoader() {
	for (auto& [path, cached] : cache_) {
		if (cached.id) {
			glDeleteTextures(1, &cached.id);
		}
	}
	cache_.clear();
}

GLuint ImageLoader::loadTexture(const std::string& path) {
	int w, h;
	return loadTexture(path, w, h);
}

GLuint ImageLoader::loadTexture(const std::string& path, int& outWidth, int& outHeight) {
	if (auto it = cache_.find(path); it != cache_.end()) {
		outWidth = it->second.width;
		outHeight = it->second.height;
		return it->second.id;
	}

	int channels;
	unsigned char* data = stbi_load(path.c_str(), &outWidth, &outHeight, &channels, 4);
	if (!data) {
		std::fprintf(stderr, "[ImageLoader] Failed to load '%s': %s\n",
			path.c_str(), stbi_failure_reason());
		outWidth = outHeight = 0;
		return 0;
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outWidth, outHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);

	cache_[path] = { tex, outWidth, outHeight };
	return tex;
}
