#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <map>

#include <time.h>
#include <sys/time.h>
#include <Leap.h>

#include "headers/Quaternion.h"
#include "headers/pen_line.h"
#include "headers/hand_input_listener.h"

using namespace Leap;

namespace hand_listener{

void HandInputListener::onInit(const Controller& controller)
{
  rotating = false;
  _lock = false;
  world_x_quaternion = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
  world_y_quaternion = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
  camera_x_position = DEFAULT_CAMERA_X;
  camera_y_position = DEFAULT_CAMERA_Y;
  camera_z_position = DEFAULT_CAMERA_Z;
}

void HandInputListener::onFrame(const Controller& controller) {
  lock();
  const Frame frame = controller.frame();
  skeleton_hands.clear();
  int open_hand_id = open_hand_id_(frame);
  if (frame.hands().isEmpty()) {
    for (std::map<int,pen_line::TracingLine>::iterator tracing_line_map = tracing_lines.begin()
        ; tracing_line_map != tracing_lines.end()
        ; tracing_line_map++) {
      penline_list.push_back((*tracing_line_map).second.line);
      tracing_lines.erase((*tracing_line_map).first);
    }
  } else if (open_hand_id < 0) {
    for (int i=0; i<frame.hands().count(); i++) {
      trace_finger_(frame.hands()[i]);
    }
  } else {
    rotate_camera_(frame.hand(open_hand_id));
  }

  // set skeleton_hands
  for (int i=0; i<frame.hands().count(); i++) {
      Hand hand = frame.hands()[i];
      virtual_hand::SkeletonHand out_hand;
      out_hand.id = hand.id();
      out_hand.confidence = hand.confidence();
      out_hand.grabStrength = hand.grabStrength();

      const Eigen::Vector3f palm = hand.palmPosition().toVector3<Eigen::Vector3f>();
      const Eigen::Vector3f palmDir = (hand.direction().toVector3<Eigen::Vector3f>()).normalized();
      const Eigen::Vector3f palmNormal = (hand.palmNormal().toVector3<Eigen::Vector3f>()).normalized();
      const Eigen::Vector3f palmSide = palmDir.cross(palmNormal).normalized();

      const Eigen::Matrix3f palmRotation = (Eigen::Matrix3f(hand.basis().toArray3x3()));
      Eigen::Matrix3f palmBasis = Eigen::Matrix3f(hand.basis().toArray3x3());

      // Remove scale from palmBasis
      const float basisScale = (palmBasis * Eigen::Vector3f::UnitX()).norm();
      palmBasis *= 1.0f / basisScale;

      out_hand.center = palm;
      out_hand.rotationButNotReally = palmBasis;

      for (int j = 0; j < 5; j++) {
        const Leap::Finger& finger = hand.fingers()[j];

        for (int k = 0; k < 3; k++) {
          Leap::Bone bone = finger.bone(static_cast<Leap::Bone::Type>(k + 1));
          out_hand.joints[j*3 + k] = bone.nextJoint().toVector3<Eigen::Vector3f>();
          out_hand.jointConnections[j*3 + k] = bone.prevJoint().toVector3<Eigen::Vector3f>();
        }
      }

      const float thumbDist = (out_hand.jointConnections[0] - palm).norm();
      const Eigen::Vector3f wrist = palm - thumbDist*(palmDir*0.8f + static_cast<float>(hand.isLeft() ? -1 : 1)*palmSide*0.5f);

      for (int j = 0; j < 4; j++) {
        out_hand.joints[15 + j] = out_hand.jointConnections[3 * j];
        out_hand.jointConnections[15 + j] = out_hand.jointConnections[3 * (j + 1)];
      }
      out_hand.joints[19] = out_hand.jointConnections[12];
      out_hand.jointConnections[19] = wrist;
      out_hand.joints[20] = wrist;
      out_hand.jointConnections[20] = out_hand.jointConnections[0];

      // Arm
      const Eigen::Vector3f elbow = hand.arm().elbowPosition().toVector3<Eigen::Vector3f>();
      out_hand.joints[21] = elbow - thumbDist*(hand.isLeft() ? -1 : 1)*palmSide*0.5;
      out_hand.jointConnections[21] = wrist;
      out_hand.joints[22] = elbow + thumbDist*(hand.isLeft() ? -1 : 1)*palmSide*0.5;
      out_hand.jointConnections[22] = out_hand.jointConnections[0];

      skeleton_hands.push_back(out_hand);
  }
  unlock();
}

void HandInputListener::initialize_world_position() {
  camera_x_position = DEFAULT_CAMERA_X;
  camera_y_position = DEFAULT_CAMERA_Y;
  camera_z_position = DEFAULT_CAMERA_Z;
  world_x_quaternion = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
  world_y_quaternion = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
}

Vector HandInputListener::convert_to_world_position_(const Vector &input_vector) {
  Quaternion q(0.0f
      , input_vector.x - camera_x_position + DEFAULT_CAMERA_X
      , input_vector.y + camera_y_position - DEFAULT_CAMERA_Y
      , input_vector.z + camera_z_position - DEFAULT_CAMERA_Z);
  q = conj(world_x_quaternion) * q * world_x_quaternion;
  q = conj(world_y_quaternion) * q * world_y_quaternion;
  Vector v(q[1], q[2], q[3]);
  return v;
}

int HandInputListener::open_hand_id_(const Frame& frame) {
  for (int i=0; i<frame.hands().count(); i++) {
    if (frame.hands()[i].fingers().extended().count() > 3) {
      return frame.hands()[i].id();
    }
  }
  return -1;
}

void HandInputListener::lock() {
  while(_lock) {
  }
  _lock = true;
}

void HandInputListener::unlock() {
  _lock = false;
}

void HandInputListener::trace_finger_(const Hand& hand) {
  if (hand.fingers().extended().count() == 0) {
    return;
  }
  int id = hand.fingers()[1].id();
  struct timeval now;
  gettimeofday(&now, NULL);
  const Pointable tracing_object = hand.pointable(id);
  if (hand.pointables().count() == 0) {
    if (tracing_lines[id].line.size() > 3) {
      penline_list.push_back(tracing_lines[id].line);
    }
    tracing_lines.erase(id);
    return;
  }
  rotating = false;
  if (tracing_object.isValid()) {
    Vector tip_position = tracing_object.tipPosition();
    tip_position = convert_to_world_position_(tip_position);
    if (tracing_lines[id].counter > 10){
      if (tip_position.distanceTo(tracing_lines[id].previous_position) > 1) {
        tracing_lines[id].previous_position = tip_position;
        tracing_lines[id].line.push_back(tip_position);
      }
    } else {
      if (timercmp(&now, &(tracing_lines[id].time_buffer), >)) {
        if (tip_position.distanceTo(tracing_lines[id].previous_position) < 10) {
          tracing_lines[id].previous_position = tip_position;
          ++tracing_lines[id].counter;
          if(tracing_lines[id].counter == 11){
            pen_line::Line start_point;
            start_point.push_back(Vector((random() % 11) / 10.0f
                  , (random() % 11) / 10.0f
                  , (random() % 11) / 10.0f));
            start_point.push_back(tip_position);
            tracing_lines[id].line = start_point;
          }
        } else {
          tracing_lines[id].previous_position = tip_position;
          tracing_lines[id].counter = 0;
        }
        gettimeofday(&(tracing_lines[id].time_buffer), NULL);
        tracing_lines[id].time_buffer.tv_usec += 30 * 1000;
      }
    }
  } else {
    tracing_lines[id].counter = 0;
    gettimeofday(&(tracing_lines[id].time_buffer), NULL);
    tracing_lines[id].time_buffer.tv_usec += 100 * 1000;
    if(tracing_object.isValid()){
      tracing_lines[id].previous_position = convert_to_world_position_(tracing_object.tipPosition());
    }
  }
  gettimeofday(&(tracing_lines[id].time_buffer), NULL);
}

void HandInputListener::rotate_camera_(const Hand& hand) {
  int id = hand.id();
  const Vector parm_position = hand.palmPosition();
  if (parm_position == Vector(0,0,0)) {
    return;
  }
  if (!rotating) {
    rotating = true;
    for (std::map<int,pen_line::TracingLine>::iterator tracing_line_map = tracing_lines.begin()
        ; tracing_line_map != tracing_lines.end()
        ; tracing_line_map++) {
      if (tracing_lines[(*tracing_line_map).first].line.size() > 3) {
        penline_list.push_back(tracing_lines[(*tracing_line_map).first].line);
      }
      tracing_lines.erase((*tracing_line_map).first);
    }
    tracing_lines[id].previous_position = parm_position;
  } else if (parm_position.distanceTo(tracing_lines[id].previous_position) > 0.3) {
    Vector move_vector(parm_position - tracing_lines[id].previous_position);
    float hard = move_vector.x / 200;
    float s = sin(hard);
    Quaternion rotate_quaternion(cos(hard)
        , 0*s
        , 1*s
        , 0*s);
    world_y_quaternion = world_y_quaternion * rotate_quaternion;
    hard = move_vector.y / 200;
    s = sin(hard);
    rotate_quaternion = Quaternion(cos(hard)
        , 1*s
        , 0*s
        , 0*s);
    world_x_quaternion = world_x_quaternion * rotate_quaternion;
    camera_z_position += move_vector.z * 6;
    tracing_lines[id].previous_position = parm_position;
  }
}

void HandInputListener::clean_line_map_(const Frame& frame) {
  return;
  for (std::map<int,pen_line::TracingLine>::iterator tracing_line_map = tracing_lines.begin()
      ; tracing_line_map != tracing_lines.end()
      ; tracing_line_map++) {
    int id = (*tracing_line_map).first;
    if (!frame.pointable(id).isValid()) {
      if (tracing_lines[id].line.size() > 3) {
        penline_list.push_back(tracing_lines[id].line);
      }
      tracing_lines.erase(id);
    }
  }
}

} // namespace hand_listener
