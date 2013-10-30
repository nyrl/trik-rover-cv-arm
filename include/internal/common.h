#ifndef TRIK_V4L2_DSP_FB_INTERNAL_COMMON_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_COMMON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct TargetDetectParams
{
  float m_detectHueFrom;
  float m_detectHueTo;
  float m_detectSatFrom;
  float m_detectSatTo;
  float m_detectValFrom;
  float m_detectValTo;
} TargetDetectParams;

typedef struct TargetLocation
{
  int m_targetX;
  int m_targetY;
  int m_targetMass;
} TargetLocation;

typedef struct DriverManualControl
{
  bool m_manualMode;

  int  m_ctrlChasisLR;
  int  m_ctrlChasisFB;
  int  m_ctrlHand;
  int  m_ctrlArm;
} DriverManualControl;

typedef struct RoverControl
{
  int m_chasisLeftSpeed;
  int m_chasisRightSpeed;
  int m_handSpeed;
  int m_armSpeed;
} RoverControl;


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


#endif // !TRIK_V4L2_DSP_FB_INTERNAL_COMMON_H_
