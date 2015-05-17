// Copyright 2015 Makoto Yano

#include "headers/oculus.h"

#include <OVR_CAPI_0_5_0.h>

#include <stdio.h>
#include <unistd.h>

namespace oculus_vr {

bool Initialize() {
  if (!ovr_Initialize(0))
    return false;
  return true;
}

void Shutdown() {
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
      float v = pose.ThePose.Position.x * pose.ThePose.Position.x +
                pose.ThePose.Position.y * pose.ThePose.Position.y +
                pose.ThePose.Position.z * pose.ThePose.Position.z;
      printf("Success head_pos (x, y, z) = (%f, %f, %f) v = %f\n"
              , pose.ThePose.Position.x
              , pose.ThePose.Position.y
              , pose.ThePose.Position.z
              , v);
    } else {
      printf("Failed to get HMD status\n");
    }
  } else {
    InitializeHmd_();
  }
  return pose;
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

void application() {
  if (ovr_Initialize(0)) {
    ovrHmd hmd = ovrHmd_Create(0);

    if (hmd) {
      // Get more details about the HMD.
      ovrSizei resolution = hmd->Resolution;
      printf("Hmd is found.\n");

      ovrHmd_Destroy(hmd);
    } else {
      printf("Cannot find hmd.\n");
    }

    ovr_Shutdown();
  }

  return;
}

}  // namespace oculus_vr
