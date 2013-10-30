#ifndef TRIK_V4L2_DSP_FB_INTERNAL_MODULE_ROVER_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_MODULE_ROVER_H_

#include <stdbool.h>

#include "internal/common.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct RoverConfigMotorMsp
{
  int m_mspI2CBusId;
  int m_mspI2CDeviceId;
  int m_mspI2CMotorCmd;
  int m_powerBackFull;
  int m_powerBackZero;
  int m_powerNeutral;
  int m_powerForwardZero;
  int m_powerForwardFull;
} RoverConfigMotorMsp;

typedef struct RoverConfigMotor
{
  const char* m_path;
  int m_powerBackFull;
  int m_powerBackZero;
  int m_powerNeutral;
  int m_powerForwardZero;
  int m_powerForwardFull;
} RoverConfigMotor;


typedef struct RoverConfig // what user wants to set
{
  RoverConfigMotorMsp m_motorChasisLeft1;
  RoverConfigMotorMsp m_motorChasisLeft2;
  RoverConfigMotorMsp m_motorChasisRight1;
  RoverConfigMotorMsp m_motorChasisRight2;
  RoverConfigMotor m_motorHand1;
  RoverConfigMotor m_motorHand2;
  RoverConfigMotor m_motorArm;
} RoverConfig;




typedef struct RoverMotorMsp
{
  int m_i2cBusFd;
  int m_mspI2CDeviceId;
  int m_mspI2CMotorCmd;
  int m_powerBackFull;
  int m_powerBackZero;
  int m_powerNeutral;
  int m_powerForwardZero;
  int m_powerForwardFull;
} RoverMotorMsp;

typedef struct RoverMotor
{
  int m_fd;
  int m_powerBackFull;
  int m_powerBackZero;
  int m_powerNeutral;
  int m_powerForwardZero;
  int m_powerForwardFull;
} RoverMotor;

typedef struct RoverOutput
{
  bool       m_opened;

  RoverMotorMsp m_motorChasisLeft1;
  RoverMotorMsp m_motorChasisLeft2;
  RoverMotorMsp m_motorChasisRight1;
  RoverMotorMsp m_motorChasisRight2;
  RoverMotor m_motorHand1;
  RoverMotor m_motorHand2;
  RoverMotor m_motorArm;
} RoverOutput;


int roverOutputInit(bool _verbose);
int roverOutputFini();

int roverOutputOpen(RoverOutput* _rover, const RoverConfig* _config);
int roverOutputClose(RoverOutput* _rover);
int roverOutputStart(RoverOutput* _rover);
int roverOutputStop(RoverOutput* _rover);

int roverOutputControl(RoverOutput* _rover, const RoverControl* _roverControl);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_MODULE_ROVER_H_
