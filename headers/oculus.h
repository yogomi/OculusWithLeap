// Copyright 2015 Makoto Yano

#ifndef OCULUS_H_  // NOLINT
#define OCULUS_H_

#include <OVR_CAPI_0_5_0.h>

namespace oculus_vr {

bool Initialize();
void Shutdown();

class OculusHmd {
 public:
  OculusHmd();
  ~OculusHmd();

  ovrPoseStatef Track();
 private:
  void InitializeHmd_();

  ovrHmd hmd_;
  ovrTrackingState tracking_state_;
};

}  // namespace oculus_vr

#endif  // OCULUS_H_  // NOLINT
