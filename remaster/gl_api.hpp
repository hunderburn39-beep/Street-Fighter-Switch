#pragma once
// One include point for OpenGL ES 3 (Switch, and the EGL/GLES test build) and
// GLEW-backed desktop GL (SDL test build).
#if defined(__SWITCH__) || defined(SFTM_GLES)
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GLES3/gl3.h>
#ifdef __SWITCH__
#include <GLES3/gl3ext.h>
#endif
#else
#include <GL/glew.h>
#endif
