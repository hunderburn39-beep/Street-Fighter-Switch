#pragma once
// One include point for OpenGL ES 3 on Switch and GLEW-backed desktop GL for CI.
#ifdef __SWITCH__
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#else
#include <GL/glew.h>
#endif
