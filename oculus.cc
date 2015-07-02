// Copyright 2015 Makoto Yano

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

#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#include <OpenGL/gl.h>
#elif __linux__
#include <GL/glu.h>
#include <GL/gl.h>
#endif
#include <OVR_CAPI_GL.h>

#include <stdio.h>
#include <unistd.h>

#include "headers/field_line.h"
#include "headers/hand_input_listener.h"
#include "headers/pen_line.h"
#include "headers/oculus.h"

namespace oculus_vr {

bool Initialize() {
  if (!ovr_Initialize(0))
    return false;
  return true;
}

void Shutdown() {
  printf("hmd shutdown\n");
  ovr_Shutdown();
}

OculusHmd::OculusHmd() {
  InitializeHmd_();
}

OculusHmd::~OculusHmd() {
  if (hmd_) {
    ovrHmd_Destroy(hmd_);
  }
}

ovrPoseStatef OculusHmd::Track() {
  ovrPoseStatef pose;
  if (hmd_) {
    tracking_state_ = ovrHmd_GetTrackingState(hmd_, ovr_GetTimeInSeconds());
    if (tracking_state_.StatusFlags & (ovrStatus_OrientationTracked |
                                        ovrStatus_PositionTracked)) {
      pose = tracking_state_.HeadPose;
    } else {
      printf("Failed to get HMD status\n");
    }
  } else {
    InitializeHmd_();
  }
  return pose;
}

GLFWmonitor *OculusHmd::Monitor() {
  int count;
  GLFWmonitor** monitors = glfwGetMonitors(&count);

  for (int i = 0; i < count; i++) {
#if defined(_WIN32)
    if (strcmp(glfwGetWin32Monitor(monitors[i])
              , hmd_->DisplayDeviceName) == 0) {
      return monitors[i];
    }
#elif defined(__APPLE__)
    if (glfwGetCocoaMonitor(monitors[i]) == hmd_->DisplayId) {
      return monitors[i];
    }
#elif defined(__linux__)
    int xpos, ypos;
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
    glfwGetMonitorPos(monitors[i], &xpos, &ypos);

    if (hmd_->WindowsPos.x == xpos &&
        hmd_->WindowsPos.y == ypys &&
        hmd_->Resolution.w == mode->width &&
        hmd_->Resolution.h == mode->height) {
      return monitors[i];
    }
#endif  // OS dependency
  }
  return nullptr;
}

void OculusHmd::FrameInit() {
  // フレームの開始
  ovrFrameTiming frameTiming = ovrHmd_BeginFrame(hmd_, 0);
  // FBOのバインド
  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);

  return;
}

void OculusHmd::FrameRender(field_line::FieldLine *bg_line
                , hand_listener::HandInputListener &listener_for_draw) {  // NOLINT
  ovrPosef eyeRenderPose[2];
  ovrVector3f eyeRenderOffset[2];
  eyeRenderOffset[ovrEye_Left] =
                    eye_render_desc_[ovrEye_Left].HmdToEyeViewOffset;
  eyeRenderOffset[ovrEye_Right] =
                    eye_render_desc_[ovrEye_Right].HmdToEyeViewOffset;
  ovrHmd_GetEyePoses(hmd_, 0, eyeRenderOffset, eyeRenderPose, NULL);
  for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
    ovrEyeType eye = hmd_->EyeRenderOrder[eyeIndex];

    // ビューポートの設定
    glViewport(eyeRenderViewport_[eye].Pos.x
            , eyeRenderViewport_[eye].Pos.y
            , eyeRenderViewport_[eye].Size.w
            , eyeRenderViewport_[eye].Size.h);
    bg_line->draw();

    glPushAttrib(GL_LIGHTING_BIT);
    GLfloat lmodel_ambient[] = { 1.0, 1.0, 1.0, 1.0 };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    for (pen_line::LineList::iterator line = listener_for_draw.penline_list.begin()
        ; line != listener_for_draw.penline_list.end(); line++) {
      if (line->size() > 3) {
        glPushMatrix();
        glLineWidth(3);
        pen_line::Line::iterator point = line->begin();
        glColor3f(point->x, point->y, point->z);
        ++point;
        glBegin(GL_LINE_STRIP);
        for (; point != line->end(); point++) {
          glVertex3f(point->x, point->y, point->z);
        }
        glEnd();
        glPopMatrix();
      }
    }
    glPopAttrib();

    glPushAttrib(GL_LIGHTING_BIT);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    for (std::map<int, pen_line::TracingLine>::iterator tracing_line_map
          = listener_for_draw.tracing_lines.begin()
        ; tracing_line_map != listener_for_draw.tracing_lines.end()
        ; tracing_line_map++) {
      if ((*tracing_line_map).second.line.size() > 3) {
        glPushMatrix();
        glLineWidth(3);
        pen_line::Line::iterator point = (*tracing_line_map).second.line.begin();
        glColor3f(point->x, point->y, point->z);
        ++point;
        glBegin(GL_LINE_STRIP);
        for (; point != (*tracing_line_map).second.line.end(); point++) {
          glVertex3f(point->x, point->y, point->z);
        }
        glEnd();
        glPopMatrix();
      }
    }

    glPopAttrib();
  }
  return;
}

void OculusHmd::FrameEnd() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return;
}

void OculusHmd::SetupOvrConfig() {
  union ovrGLConfig config;
  config.OGL.Header.API = ovrRenderAPI_OpenGL;
  config.OGL.Header.BackBufferSize = hmd_->Resolution;
  config.OGL.Header.Multisample = 0;
  // For Linux?
  // config.OGL.Disp = glfwGetX11Display();

  SetupOvrEye_();

  eye_render_desc_[ovrEye_Left] = ovrHmd_GetRenderDesc(hmd_
                                        , ovrEye_Left
                                        , hmd_->DefaultEyeFov[ovrEye_Left]);
  eye_render_desc_[ovrEye_Right] = ovrHmd_GetRenderDesc(hmd_
                                        , ovrEye_Right
                                        , hmd_->DefaultEyeFov[ovrEye_Right]);

  ovrBool result = ovrHmd_ConfigureRendering(hmd_, &config.Config,
                                    // ovrDistortionCap_Chromatic |
                                    ovrDistortionCap_Vignette |
                                    ovrDistortionCap_TimeWarp |
                                    ovrDistortionCap_Overdrive ,
                                    hmd_->DefaultEyeFov, eye_render_desc_);

  ovrHmd_SetEnabledCaps(hmd_, ovrHmdCap_LowPersistence |
                              ovrHmdCap_DynamicPrediction |
                              ovrHmdCap_ExtendDesktop);
  return;
}

void OculusHmd::InitializeHmd_() {
  hmd_ = ovrHmd_Create(0);
  if (hmd_) {
    // Get more details about the HMD.
    ovrSizei resolution = hmd_->Resolution;
    printf("Hmd is found.\n");

    ovrHmd_ConfigureTracking(hmd_
                            , ovrTrackingCap_Orientation |
                              ovrTrackingCap_MagYawCorrection |
                              ovrTrackingCap_Position
                            , 0);
    sleep(1);

  } else {
    printf("Cannot find hmd.\n");
  }
}

void OculusHmd::SetupOvrEye_() {
  frameBuffer_ = 0;
  vaoHandle_ = 0;
  renderBuffer_ = 0;

  ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd_
                                                  , ovrEye_Left
                                                  , hmd_->DefaultEyeFov[0]
                                                  , 1.0f);
  ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd_
                                                  , ovrEye_Right
                                                  , hmd_->DefaultEyeFov[1]
                                                  , 1.0f);
  ovrSizei renderTargetSize;
  renderTargetSize.w  = recommenedTex0Size.w + recommenedTex1Size.w;
  renderTargetSize.h = fmax(recommenedTex0Size.h, recommenedTex1Size.h);

  renderTargetSize = hmd_->Resolution;

  // FBOの準備
  glGenFramebuffers(1, &frameBuffer_);
  glGenTextures(1, &texture_);

  // 計算したテクスチャサイズのFBOを確保
  glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA
              , renderTargetSize.w, renderTargetSize.h, 0
              , GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_, 0);

  // Render Bufferの準備
  glGenRenderbuffers(1, &renderBuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24
                      , renderTargetSize.w, renderTargetSize.h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT
                          , GL_RENDERBUFFER, renderBuffer_);

  // HMDの準備
  ovrFovPort eyeFov[2] = { hmd_->DefaultEyeFov[0], hmd_->DefaultEyeFov[1] };

  // 1枚のテクスチャに書く場合。左は左上(0,0)、
  // サイズは(renderTargetSize.w/2, renderTargetSize.h)
  eyeRenderViewport_[0].Pos = {0, 0};
  eyeRenderViewport_[0].Size = \
                    {renderTargetSize.w / 2, renderTargetSize.h};
  // 右は真ん中上((renderTargetSize.w + 1) / 2, 0)、
  // サイズは同じ(eyeRenderViewport[0].Size)
  eyeRenderViewport_[1].Pos = {(renderTargetSize.w + 1) / 2, 0};
  eyeRenderViewport_[1].Size = eyeRenderViewport_[0].Size;

  // OpenGLテクスチャの設定（左目用）
  eyeTexture_[0].OGL.Header.API = ovrRenderAPI_OpenGL;
  eyeTexture_[0].OGL.Header.TextureSize = renderTargetSize;
  eyeTexture_[0].OGL.Header.RenderViewport = eyeRenderViewport_[0];
  eyeTexture_[0].OGL.TexId = texture_;

  // OpenGLテクスチャの設定（右目用）
  eyeTexture_[1] = eyeTexture_[0];
  eyeTexture_[1].OGL.Header.RenderViewport = eyeRenderViewport_[1];

  return;
}

}  // namespace oculus_vr
