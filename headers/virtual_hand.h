// Copyright 2015 Makoto Yano

#ifndef HEADERS_VIRTUAL_HAND_H_
#define HEADERS_VIRTUAL_HAND_H_

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

namespace virtual_hand {

struct SkeletonHand {
  // Hand Id from Leap API
  int id;

  float confidence;

  // Pinch strength, from API
  float grabStrength;

  // Palm's position
  Eigen::Vector3f center;

  // Palm's rotation/basis -- it's reversed for the left hand
  Eigen::Matrix3f rotationButNotReally;

  // Eigen::stdvectorV3f tips[5];
  Eigen::Vector3f joints[23];
  Eigen::Vector3f jointConnections[23];
  Eigen::Vector3f avgExtended;

  Eigen::Vector3f getManipulationPoint() const {
                                      return 0.5f * (joints[0] + joints[3]); }

  Eigen::Matrix3f arbitraryRelatedRotation() const;


  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

}  // namespace virtual_hand

#endif  // HEADERS_VIRTUAL_HAND_H_
