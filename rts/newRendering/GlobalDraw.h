
#include <GL/glxew.h>
namespace GlobalDraw::Func
{
	void ClearScreen();
}

namespace GlobalDraw::State
{

}

namespace GlobalDraw::Resources
{
	void glDeleteTextures(GLsizei n, const GLuint* textures);
	void glGenTextures(GLsizei n, const GLuint* textures);
	void glBindTexture(GLenum target, GLuint texture);
	void glTexParameteri(GLenum target, GLenum pname, GLint param);
	void glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
	void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
	void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
}