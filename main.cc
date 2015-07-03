// Copyright 2014 Makoto Yano

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


#include <math.h>
#include <unistd.h>
#include <stdio.h>

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
#include <LeapMath.h>
#include <OVR_CAPI_GL.h>
#include <boost/optional.hpp>
#include <memory>

#include "headers/pen_line.h"
#include "headers/field_line.h"
#include "headers/Quaternion.h"
#include "headers/hand_input_listener.h"
#include "headers/oculus.h"

field_line::FieldLine *background_line;
oculus_vr::OculusHmd *hmd;

/////////////////////////////////
// for Leap

hand_listener::HandInputListener listener;

void reshape_func(int width, int height) {
  glViewport(0, 0, width, height);
}

void apply_world_quaternion(const Quaternion &q) {
  GLfloat m[16];
  float x2 = q[1] * q[1] * 2.0f;
  float y2 = q[2] * q[2] * 2.0f;
  float z2 = q[3] * q[3] * 2.0f;
  float xy = q[1] * q[2] * 2.0f;
  float yz = q[2] * q[3] * 2.0f;
  float zx = q[3] * q[1] * 2.0f;
  float xw = q[1] * q[0] * 2.0f;
  float yw = q[2] * q[0] * 2.0f;
  float zw = q[3] * q[0] * 2.0f;

  m[0] = 1.0f - y2 - z2;
  m[1] = xy + zw;
  m[2] = zx - yw;
  m[3] = 0.0f;

  m[4] = xy - zw;
  m[5] = 1.0 - z2 - x2;
  m[6] = yz + xw;
  m[7] = 0.0f;

  m[8] = zx + yw;
  m[9] = yz - xw;
  m[10] = 1.0f - x2 - y2;
  m[11] = 0.0f;

  m[12] = 0.0f;
  m[13] = 0.0f;
  m[14] = 0.0f;
  m[15] = 1.0f;
  glMultMatrixf(m);
}

void display_func(GLFWwindow *window) {
  float ratio;
  int width, height;

  glfwGetFramebufferSize(window, &width, &height);
  ratio = width / static_cast<float>(height);

  listener.lock();

  hmd->FrameInit();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(
    60.0f,
    static_cast<float>(width)
    / static_cast<float>(height), 2.0f, 200000.0f);
  gluLookAt(0, 0, 0
      , 0, 0,  -300, 0, 1, 0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  boost::optional<ovrPoseStatef> pose = hmd->Track();

  if (pose) {
    Quaternion hmd_quart((*pose).ThePose.Orientation.w
                      , - (*pose).ThePose.Orientation.x
                      , - (*pose).ThePose.Orientation.y
                      , - (*pose).ThePose.Orientation.z);
    apply_world_quaternion(hmd_quart);
  } else {
    Quaternion hmd_quart(1, 0, 0, 0);
    apply_world_quaternion(hmd_quart);
  }
  glTranslated(listener.camera_x_position
      , -listener.camera_y_position
      , -listener.camera_z_position);
  apply_world_quaternion(listener.world_x_quaternion);
  apply_world_quaternion(listener.world_y_quaternion);

  hmd->FrameRender(background_line, listener);

  hmd->FrameEnd();
  listener.unlock();
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void key_func(unsigned char key, int x, int y) {
  switch (toupper(key)) {
  case 'Q':
    exit(0);
    break;
  case 'I':
    listener.initialize_world_position();
    break;
  }
}

void error_callback(int error, const char *description) {
  fputs(description, stderr);
}

static void key_callback(GLFWwindow* window
                        , int key
                        , int scancode
                        , int action
                        , int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

void init_opengl() {
  glEnable(GL_DEPTH_TEST);

  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat mat_shininess[] = { 100.0 };
  GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
  GLfloat white_light[] = { 1.0, 1.0, 1.0, 0.0 };
  GLfloat lmodel_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
  glClearColor(0, 0, 0, 1.0f);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
  glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glShadeModel(GL_SMOOTH);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  background_line = new field_line::FieldLine();
  listener.initialize_world_position();
  float hard = Leap::PI / 32;
  float s = sin(hard);
  Quaternion rotate_quaternion(cos(hard), 1*s, 0*s, 0*s);
}

int main(int argc, char** argv) {
  if (!oculus_vr::Initialize()) {
    printf("Oculus initialize failed.\n");
    return -1;
  }

  hmd = new oculus_vr::OculusHmd();

  glfwSetErrorCallback(error_callback);

  if (!glfwInit())
    exit(EXIT_FAILURE);

  GLFWmonitor* monitor = hmd->Monitor();

  GLFWwindow *window = nullptr;

  if (monitor) {
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    window = glfwCreateWindow(mode->width
                                      , mode->height
                                      , "My Title"
                                      , monitor
                                      , NULL);

    hmd->SetupOvrConfig();
  } else {
    printf("Cannot get monitor.\n");
    window = glfwCreateWindow(1440
                                      , 900
                                      , "My Title"
                                      , glfwGetPrimaryMonitor()
                                      , NULL);
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glfwSetKeyCallback(window, key_callback);

  init_opengl();
  Leap::Controller controller;
  controller.setPolicyFlags(
        static_cast<Leap::Controller::PolicyFlag>(
          Leap::Controller::PolicyFlag::POLICY_IMAGES |
          Leap::Controller::PolicyFlag::POLICY_OPTIMIZE_HMD));
  controller.addListener(listener);

  while (!glfwWindowShouldClose(window)) {
    display_func(window);
  }

  printf("finish\n");

  glfwDestroyWindow(window);
  glfwTerminate();

  delete hmd;
  oculus_vr::Shutdown();

  return 0;
}
