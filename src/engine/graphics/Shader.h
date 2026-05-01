#pragma once
// Expose GL 2.0+ extension prototypes (glCreateShader, glCreateProgram, …)
// when including the system GL header. macOS's <OpenGL/gl.h> ships up to
// GL 1.4 by default; the rest are gated behind this macro.
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <string>
#include <unordered_map>

// Thin wrapper around an OpenGL program object: compile a vertex+fragment
// pair, link, cache uniform/attribute locations. Used by the GLES backend to
// migrate off fixed-function GL toward programmable shaders, which is the
// only pipeline that exists on GLES 2.0+ / WebGL.
//
// GLSL source should be 1.10 / GLES 2.0 compatible (no `#version`, use
// `attribute`/`varying`/`texture2D`/`gl_FragColor`) so the same shader works
// on macOS compat profile, desktop GL2, and mobile GLES2.
class Shader {
public:
	Shader() = default;
	~Shader();

	// Compile and link. Returns true on success. On failure, logs the GLSL
	// info-log via LOG_ERROR and leaves the program unbound (`isValid()`
	// stays false). Call `use()` to bind for subsequent draws.
	bool compile(const char* vertSource, const char* fragSource);

	// Bind this program for subsequent draws. Idempotent — bind count is
	// not tracked, but redundant binds are cheap.
	void use() const;

	// Unbind any program (revert to fixed-function pipeline on contexts
	// that still allow it). Useful during the migration where some draws
	// still use fixed-function while others are on the shader path.
	static void useNone();

	// Look up uniform/attribute locations. Cached per-Shader. Returns -1
	// if the name doesn't exist or the variable was optimized out.
	GLint uniform(const char* name);
	GLint attribute(const char* name);

	bool isValid() const { return program != 0; }
	GLuint id() const { return program; }

private:
	GLuint program = 0;
	std::unordered_map<std::string, GLint> uniformCache;
	std::unordered_map<std::string, GLint> attribCache;
};
