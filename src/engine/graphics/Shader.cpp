#include "Shader.h"

#include "Log.h"

#include <vector>

namespace {

// Compile a single shader stage, returning the GL handle on success or 0
// on failure. The info log is dumped to LOG_ERROR on failure.
GLuint compileStage(GLenum stage, const char* source, const char* stageName) {
	GLuint sh = glCreateShader(stage);
	if (sh == 0) {
		LOG_ERROR("[shader] glCreateShader({}) returned 0\n", stageName);
		return 0;
	}
	glShaderSource(sh, 1, &source, nullptr);
	glCompileShader(sh);

	GLint status = GL_FALSE;
	glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLint logLen = 0;
		glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &logLen);
		std::vector<char> log((size_t)std::max(logLen, 1));
		glGetShaderInfoLog(sh, logLen, nullptr, log.data());
		LOG_ERROR("[shader] {} compile failed:\n{}\n", stageName, log.data());
		glDeleteShader(sh);
		return 0;
	}
	return sh;
}

} // namespace

Shader::~Shader() {
	if (program != 0) {
		glDeleteProgram(program);
		program = 0;
	}
}

bool Shader::compile(const char* vertSource, const char* fragSource) {
	GLuint vs = compileStage(GL_VERTEX_SHADER, vertSource, "vertex");
	if (vs == 0) return false;
	GLuint fs = compileStage(GL_FRAGMENT_SHADER, fragSource, "fragment");
	if (fs == 0) {
		glDeleteShader(vs);
		return false;
	}

	GLuint prog = glCreateProgram();
	if (prog == 0) {
		LOG_ERROR("[shader] glCreateProgram returned 0\n");
		glDeleteShader(vs);
		glDeleteShader(fs);
		return false;
	}
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);

	// Shaders can be detached + deleted after link; the program owns its copy.
	glDetachShader(prog, vs);
	glDetachShader(prog, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	GLint status = GL_FALSE;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) {
		GLint logLen = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
		std::vector<char> log((size_t)std::max(logLen, 1));
		glGetProgramInfoLog(prog, logLen, nullptr, log.data());
		LOG_ERROR("[shader] link failed:\n{}\n", log.data());
		glDeleteProgram(prog);
		return false;
	}

	if (this->program != 0) {
		glDeleteProgram(this->program);
	}
	this->program = prog;
	this->uniformCache.clear();
	this->attribCache.clear();
	LOG_INFO("[shader] linked program id={}\n", prog);
	return true;
}

void Shader::use() const {
	if (program != 0) {
		glUseProgram(program);
	}
}

void Shader::useNone() {
	glUseProgram(0);
}

GLint Shader::uniform(const char* name) {
	if (program == 0) return -1;
	auto it = this->uniformCache.find(name);
	if (it != this->uniformCache.end()) return it->second;
	GLint loc = glGetUniformLocation(program, name);
	this->uniformCache.emplace(name, loc);
	return loc;
}

GLint Shader::attribute(const char* name) {
	if (program == 0) return -1;
	auto it = this->attribCache.find(name);
	if (it != this->attribCache.end()) return it->second;
	GLint loc = glGetAttribLocation(program, name);
	this->attribCache.emplace(name, loc);
	return loc;
}
