#pragma once
#ifdef __SWITCH__
#include <switch.h>
#include <EGL/egl.h>
#include <stdexcept>
class SwitchGraphics {
 EGLDisplay display=EGL_NO_DISPLAY;
 EGLSurface surface=EGL_NO_SURFACE;
 EGLContext context=EGL_NO_CONTEXT;
public:
 SwitchGraphics(){
  try {
   display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if(display==EGL_NO_DISPLAY||!eglInitialize(display,nullptr,nullptr))
    throw std::runtime_error("EGL initialization failed");
   if(!eglBindAPI(EGL_OPENGL_API))throw std::runtime_error("OpenGL API unavailable");
   const EGLint attributes[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,
    EGL_BLUE_SIZE,8,EGL_NONE};
   EGLConfig config;EGLint count=0;
   if(!eglChooseConfig(display,attributes,&config,1,&count)||count<1)
    throw std::runtime_error("No EGL configuration");
   surface=eglCreateWindowSurface(display,config,nwindowGetDefault(),nullptr);
   if(surface==EGL_NO_SURFACE)throw std::runtime_error("EGL surface failed");
   const EGLint ca[]={EGL_CONTEXT_MAJOR_VERSION,3,EGL_CONTEXT_MINOR_VERSION,3,EGL_NONE};
   context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);
   if(context==EGL_NO_CONTEXT||!eglMakeCurrent(display,surface,surface,context))
    throw std::runtime_error("OpenGL context failed");
   eglSwapInterval(display,0);
  }catch(...){release();throw;}
 }
 SwitchGraphics(const SwitchGraphics&)=delete;
 SwitchGraphics& operator=(const SwitchGraphics&)=delete;
 ~SwitchGraphics(){release();}
 void present(){if(!eglSwapBuffers(display,surface))throw std::runtime_error("EGL swap failed");}
private:
 void release(){
  if(display==EGL_NO_DISPLAY)return;
  eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
  if(context!=EGL_NO_CONTEXT)eglDestroyContext(display,context);
  if(surface!=EGL_NO_SURFACE)eglDestroySurface(display,surface);
  eglTerminate(display);display=EGL_NO_DISPLAY;
 }
};
#endif
