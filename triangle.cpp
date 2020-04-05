#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <initializer_list>
#include <vector>
#include <stdio.h>

#include <glew/include/GL/glew.h>
#include "glfw/include/GLFW/glfw3.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

std::string readFile(const std::string& file) {
	std::string dir = __FILE__;
	dir.resize(dir.find_last_of('/'));
	std::ifstream f((dir+'/'+file).c_str(), std::ifstream::in);
	if (!f) {
		printf("I could not find \"%s\" in \"%s\".\n",
			file.c_str(), dir.c_str());
		exit(EXIT_FAILURE);
	}
	std::string l;
	std::stringstream content;
	while (f.good()) {
		std::getline(f, l);
		content << l << std::endl;
	}
	f.close();
	return content.str();
}

struct OpenGL {
	static void on_keypress(GLFWwindow* window, int key, int scancode, int action, int mods) {
		if(key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
	}
	static void on_mouse_move(GLFWwindow* window, double xpos, double ypos) {
		printf("x=%f\ny=%f\n", xpos, ypos);
	}
	static void on_mouse_press(GLFWwindow* window, int button, int action, int mods) {
		printf("button=%d\naction=%d\nmods=%d\n", button, action, mods);
	}
	static void on_mouse_scroll(GLFWwindow* window, double xoffset, double yoffset) {
		printf("xoff=%f\nyoff=%f\n", xoffset, yoffset);
	}
	GLFWwindow* window;
	glm::vec2 viewport;
	OpenGL(const GLchar* name, GLsizei w, GLsizei h) {
		viewport = glm::vec2(w,h);
		if(!glfwInit()) exit(EXIT_FAILURE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		window = glfwCreateWindow((int)viewport.x, (int)viewport.y, name, NULL, NULL);
		glfwMakeContextCurrent(window);
		glfwSetKeyCallback(window, on_keypress);
		glfwSetCursorPosCallback(window, on_mouse_move);
		glfwSetMouseButtonCallback(window, on_mouse_press);
		glfwSetScrollCallback(window, on_mouse_scroll);
		glewExperimental = GL_TRUE;
		glewInit();
		glClearColor(1,1,1, 1.0f);
		glClearDepth(1.0);
	}
	~OpenGL() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	bool isRunning() {
		return !glfwWindowShouldClose(window);
	}
	void swapBuffers() {
		glfwSwapBuffers(window);
	}
};

struct Uniform {
	const int TYPE_INT = 0;
	const int TYPE_FLOAT = 1;
	const int TYPE_VEC3 = 2;
	const int TYPE_MAT4 = 3;
	int type;
	GLint int_value;
	GLfloat float_value;
	glm::vec3 vec3_value;
	glm::mat4 mat4_value;
	Uniform() {}
	Uniform* operator=(GLint value) {
		type = TYPE_INT;
		int_value = value;
		return this;
	}
	Uniform* operator=(GLfloat value) {
		type = TYPE_FLOAT;
		float_value = value;
		return this;
	}
	Uniform* operator=(glm::vec3 vector) {
		type = TYPE_VEC3;
		vec3_value = vector;
		return this;
	}
	Uniform* operator=(glm::mat4 matrix) {
		type = TYPE_MAT4;
		mat4_value = matrix;
		return this;
	}
	Uniform* operator=(Uniform &rhs) {
		type = rhs.type;
		if(type == TYPE_INT) int_value = rhs.int_value;
		else if(type == TYPE_FLOAT) float_value = rhs.float_value;
		else if(type == TYPE_VEC3) vec3_value = rhs.vec3_value;
		else if(type == TYPE_MAT4) mat4_value = rhs.mat4_value;
		return this;
	}
	// Update called by shader just before its execution
	virtual void update(GLint loc) {
		if(type == TYPE_INT) glUniform1i(loc, int_value);
		else if(type == TYPE_FLOAT) glUniform1f(loc, float_value);
		else if(type == TYPE_VEC3) glUniform3f(loc, vec3_value.x, vec3_value.y, vec3_value.z);
		else if(type == TYPE_MAT4) glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat*)(&mat4_value));
	}
};

struct VertexArray {
	GLuint id;
	GLsizei size;
	GLint num_attribs;
	VertexArray(std::vector<std::vector<std::vector<GLfloat>>> vertices) {
		size = vertices.size();
		num_attribs = vertices[0].size();
		glGenVertexArrays(1, &id);
		glBindVertexArray(id);
		GLuint buffer;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		GLsizeiptr bytes = 0;
		for(unsigned int i = 0; i < vertices.size(); i++) {
			for(unsigned int j = 0; j < vertices[i].size(); j++) {
				bytes += vertices[i][j].size() * sizeof(GLfloat);
			}
		}
		glBufferData(GL_ARRAY_BUFFER, bytes, 0, GL_STATIC_DRAW);
		GLintptr offset = 0;
		for(unsigned int i = 0; i < vertices.size(); i++) {
			for(unsigned int j = 0; j < vertices[i].size(); j++) {
				int n = vertices[i][j].size() * sizeof(GLfloat);
				glBufferSubData(GL_ARRAY_BUFFER, offset, n, vertices[i][j].data());
				offset += n;
			}
		}
		// Using explicit shader locations.
		// The vertex attributes must have indices 0+.
		// Uniform indices are assumed to start after the last attribute index.
		offset = 0;
		for(GLuint i = 0; i < vertices[0].size(); i++) {
			glVertexAttribPointer(i, vertices[0][i].size(), GL_FLOAT, GL_FALSE, bytes/size, (GLvoid*) offset);
			glEnableVertexAttribArray(i);
			offset += vertices[0][i].size() * sizeof(GLfloat);
		}
	}
	~VertexArray() {
		glDeleteVertexArrays(1, &id);
	}
};

struct Shader {
	GLuint frag;
	GLuint vert;
	GLuint program;
	Shader(std::string name) {
		frag = glCreateShader(GL_FRAGMENT_SHADER);
		vert = glCreateShader(GL_VERTEX_SHADER);
		std::string frag_source = readFile(name+".frag");
		std::string vert_source = readFile(name+".vert");
		const char* frag_source_c_str = frag_source.c_str();
		const char* vert_source_c_str = vert_source.c_str();
		int frag_source_length = frag_source.length();
		int vert_source_length = vert_source.length();
		glShaderSource(frag, 1, &frag_source_c_str, &frag_source_length);
		glShaderSource(vert, 1, &vert_source_c_str, &vert_source_length);
		glCompileShader(frag);
		glCompileShader(vert);
		GLint is_frag_compiled, is_vert_compiled;
		glGetShaderiv(frag, GL_COMPILE_STATUS, &is_frag_compiled);
		glGetShaderiv(vert, GL_COMPILE_STATUS, &is_vert_compiled);
		if(is_frag_compiled != GL_TRUE || is_vert_compiled != GL_TRUE) {
			GLchar infolog[1024];
			glGetShaderInfoLog(frag, 1024, 0, infolog);
			printf("In %s.frag: %s\n", name.c_str(), infolog);
			glGetShaderInfoLog(vert, 1024, 0, infolog);
			printf("In %s.vert: %s\n", name.c_str(), infolog);
			exit(EXIT_FAILURE);
		}
		program = glCreateProgram();
		glAttachShader(program, frag);
		glAttachShader(program, vert);
		glLinkProgram(program);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
	}
	~Shader() {
		glDeleteShader(frag);
		glDeleteShader(vert);
		glDeleteProgram(program);
	}
	struct TextureReadMode {
		GLint minfilter = GL_LINEAR;
		GLint magfilter = GL_LINEAR;
		GLint wrapx = GL_REPEAT;
		GLint wrapy = GL_REPEAT;
	} tex_read_mode;
	struct Execution {
		GLsizei nvertices = 0;
		GLenum primitive = GL_TRIANGLES;
	} execution;
	// Shader execution becomes like a function call. Arguments: VAO, list of uniforms, list of textures (list orders matter)
	Execution& operator()(VertexArray& vao, std::initializer_list<Uniform*> uniforms, std::initializer_list<GLuint> textures) {
		glUseProgram(program);
		glBindVertexArray(vao.id);
		GLuint uloc = 0;
		for(uloc = 0; uloc < uniforms.size(); uloc++)
			uniforms.begin()[uloc]->update(vao.num_attribs + uloc);
		for(GLuint tunit = 0; tunit < textures.size(); tunit++, uloc++) {
			glActiveTexture(GL_TEXTURE0 + tunit);
			glBindTexture(GL_TEXTURE_2D, textures.begin()[tunit]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex_read_mode.minfilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex_read_mode.magfilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex_read_mode.wrapx);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex_read_mode.wrapy);
			glUniform1i(uloc, tunit);
		}
		execution.nvertices = vao.size;
		return execution;
	}
};

struct Framebuffer {
	GLsizei w;
	GLsizei h;
	GLuint framebuffer;
	std::vector<GLuint> cbuffers;
	GLuint zbuffer;
	GLuint sbuffer;
	Framebuffer() : w(0), h(0), framebuffer(0) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK); // assuming double buffered context
		glReadBuffer(GL_BACK);
	}
	Framebuffer(GLsizei width, GLsizei height) : w(width), h(height) {
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}
	void attachColorBufferTexture(GLenum sized_format) {
		int maxColorAttachments;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
		assert(cbuffers.size() < maxColorAttachments);
		cbuffers.push_back(0);
		glGenTextures(1, &cbuffers.back());
		glBindTexture(GL_TEXTURE_2D, cbuffers.back());
		glTexStorage2D(GL_TEXTURE_2D, 1, sized_format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + cbuffers.size(), GL_TEXTURE_2D, cbuffers.back(), 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + cbuffers.size());
		glReadBuffer(GL_COLOR_ATTACHMENT0 + cbuffers.size());
	}
	void attachZBufferTexture(GLenum sized_format) {
		glGenTextures(1, &zbuffer);
		glBindTexture(GL_TEXTURE_2D, zbuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, sized_format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, zbuffer, 0);
	}
	void attachStencilBufferTexture(GLenum sized_format) {
		glGenTextures(1, &sbuffer);
		glBindTexture(GL_TEXTURE_2D, sbuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, sized_format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sbuffer, 0);
	}
	void attachColorRenderbuffer(GLenum format) {
		cbuffers.push_back(0);
		glGenRenderbuffers(1, &cbuffers.back());
		glBindRenderbuffer(GL_RENDERBUFFER, cbuffers.back());
		glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + cbuffers.size(), GL_RENDERBUFFER, cbuffers.back());
		glDrawBuffer(GL_BACK); // assuming double buffered context
		glReadBuffer(GL_BACK);
	}
	void attachZRenderbuffer(GLenum format) {
		GLuint b = 0;
		glGenRenderbuffers(1, &b);
		glBindRenderbuffer(GL_RENDERBUFFER, b);
		glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, b);
	}
	void attachStencilRenderbuffer(GLenum format) {
		GLuint b = 0;
		glGenRenderbuffers(1, &b);
		glBindRenderbuffer(GL_RENDERBUFFER, b);
		glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, b);
	}
	void attachDepthAndStencilRenderbuffers(GLenum format) {
		GLuint b = 0;
		glGenRenderbuffers(1, &b);
		glBindRenderbuffer(GL_RENDERBUFFER, b);
		glRenderbufferStorage(GL_RENDERBUFFER, format, w, h);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, b);
	}
	void operator=(Shader::Execution& shader_exec) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			exit(1);
		if(framebuffer!=0) glViewport(0, 0, w, h);
		glDrawArrays(shader_exec.primitive, 0, shader_exec.nvertices);
	}
	void clear(GLbitfield buffers) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			exit(1);
		glClear(buffers);
	}
};

int main() {
	OpenGL opengl("Hello World", 800, 600);

	glm::vec3 pos(0.0f, 0.0f, 4.0f);
	glm::vec3 target(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 1.0f, 0.0f);
	const float fov = glm::radians(90.0f);
	const float near = 0.1f;
	const float far = 100.0f;

	Uniform cam;
	cam = glm::perspective(fov, opengl.viewport.x / opengl.viewport.y, near, far) * glm::lookAt(pos, target, up);

	Uniform model;
	model = glm::mat4(1);

	VertexArray triangle({
		// position   // color
		{{-1,-1,0,1}, {1,0,0,1}}, // red
		{{-1,1,0,1}, {0,1,0,1}}, // green
		{{1,-1,0,1}, {0,0,1,1}} // blue
	});

	VertexArray quad({
		// (x, y)      // (u, v)
		{{-1.0f,  1.0f},  {0.0f, 1.0f}}, // top left
		{{1.0f, -1.0f},  {1.0f, 0.0f}}, // bottom right
		{{-1.0f, -1.0f},  {0.0f, 0.0f}}, // bottom left
		{{-1.0f,  1.0f},  {0.0f, 1.0f}}, // top left
		{{1.0f,  1.0f},  {1.0f, 1.0f}}, // top right
		{{1.0f, -1.0f},  {1.0f, 0.0f}} // bottom right
	});

	Shader writeDepth("writeDepth");
	Shader applyTexture("applyTexture");

	Framebuffer defaultFramebuffer;

	Framebuffer depthMap(1024, 1024);
	depthMap.attachZBufferTexture(GL_DEPTH_COMPONENT32F);

	while(opengl.isRunning()) {
		depthMap.clear(GL_DEPTH_BUFFER_BIT);
		depthMap = writeDepth(triangle, {cam=cam, model=glm::mat4(1)}, {});
		defaultFramebuffer.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		defaultFramebuffer = applyTexture(quad, {}, {depthMap.zbuffer});
		opengl.swapBuffers();
		glfwPollEvents();
	}
	return 0;
}