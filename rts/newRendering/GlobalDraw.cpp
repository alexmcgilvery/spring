#include "GlobalDraw.h"

#include "System/Misc/TracyDefs.h"

void GlobalDraw::Func::ClearScreen()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1, 1, 1);
}

void GlobalDraw::Resources::glDeleteTextures(GLsizei n, const GLuint* textures)
{
	glDeleteTextures(n, textures);
}

void GlobalDraw::Resources::glGenTextures(GLsizei n, const GLuint* textures)
{
	glGenTextures(n, textures);
}

void GlobalDraw::Resources::glBindTexture(GLenum target, GLuint texture)
{
	glBindTexture(target, texture);
}

void GlobalDraw::Resources::glTexParameteri(GLenum target, GLenum pname, GLint param)
{
	glTexParameteri(target, pname, param);
}

void GlobalDraw::Resources::glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
	glTexParameteriv(target, pname, params);
}

void GlobalDraw::Resources::glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
	glTexParameterfv(target, pname, params);
}

void GlobalDraw::Resources::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels)
{
	glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}