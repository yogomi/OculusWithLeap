// Copyright 2015 Makoto Yano

#ifndef HEADERS_HAND_INPUT_LISTENER_H_
#define HEADERS_HAND_INPUT_LISTENER_H_

#include <Leap.h>

#include <map>

#include "./Quaternion.h"
#include "./pen_line.h"
#include "./virtual_hand.h"

#define DEFAULT_CAMERA_X 0
#define DEFAULT_CAMERA_Y 300
#define DEFAULT_CAMERA_Z 600
#define MAX_TRACABLE_POINT_COUNT 10

namespace hand_listener {

class HandInputListener : public Leap::Listener {
 public:
  virtual void onInit(const Leap::Controller& controller);
  virtual void onFrame(const Leap::Controller& controller);
  void initialize_world_position();

  void lock();
  void unlock();
  bool rotating;
  Quaternion world_x_quaternion;
  Quaternion world_y_quaternion;
  pen_line::LineList penline_list;
  std::map<int, pen_line::TracingLine> tracing_lines;

  float camera_x_position;
  float camera_y_position;
  float camera_z_position;

 private:
  Vector convert_to_world_position_(const Leap::Vector &input_vector);
  bool _lock;
  int open_hand_id_(const Leap::Frame& frame);
  void trace_finger_(const Leap::Hand& hand);
  void rotate_camera_(const Leap::Hand& hand);
  void clean_line_map_(const Leap::Frame& frame);
};

}  // namespace hand_listener

#endif  // HEADERS_HAND_INPUT_LISTENER_H_
