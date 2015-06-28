// Copyright 2015 Makoto Yano

#ifndef OCULUS_H_  // NOLINT
#define OCULUS_H_

#if defined(_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
  #define GLFW_EXPOSE_NATIVE_WGL
  #define OVR_OS_WIN32
#elif defined(__APPLE__)
  #define GLFW_EXPOSE_NATIVE_COCOA
  #define GLFW_EXPOSE_NATIVE_NSGL
  #define OVR_OS_MAC
#elif defined(__linux__)
  #define GLFW_EXPOSE_NATIVE_X11
  #define GLFW_EXPOSE_NATIVE_GLX
  #define OVR_OS_LINUX
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <OVR_CAPI_GL.h>

namespace oculus_vr {

bool Initialize();
void Shutdown();

class OculusHmd {
 public:
  OculusHmd();
  ~OculusHmd();

  ovrPoseStatef Track();
  void SetupOvrConfig();
  GLFWmonitor *Monitor();

 private:
  void InitializeHmd_();
  void SetupOvrEye_();

  ovrHmd hmd_;
  ovrTrackingState tracking_state_;
  ovrEyeRenderDesc eye_render_desc_[2];
  ovrRecti eyeRenderViewport_[2];
  ovrGLTexture eyeTexture_[2];

  // Vertex Array Object用
  GLuint frameBuffer_;
  GLuint vaoHandle_;
  GLuint vboHandles_[2];
  GLuint texture_;

  // For Shader
  GLuint renderBuffer_;
};

}  // namespace oculus_vr

#endif  // OCULUS_H_  // NOLINT
