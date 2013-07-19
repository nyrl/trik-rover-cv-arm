#ifndef TRIK_V4L2_DSP_FB_INTERNAL_ROVER_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_ROVER_H_

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct RoverConfigMotor
{
  const char* m_path;
  int m_powerBack;
  int m_powerNeutral;
  int m_powerForward;
} RoverConfigMotor;


typedef struct RoverConfig // what user wants to set
{
  RoverConfigMotor m_motor1;
  RoverConfigMotor m_motor2;
  RoverConfigMotor m_motor3;
  RoverConfigMotor m_motor4;

  int m_zeroX;
  int m_zeroY;
  int m_zeroMass;
} RoverConfig;


typedef struct RoverMotor
{
  int m_fd;
  int m_powerBack;
  int m_powerNeutral;
  int m_powerForward;
} RoverMotor;

typedef struct RoverOutput
{
  bool       m_opened;

  RoverMotor m_motor1;
  RoverMotor m_motor2;
  RoverMotor m_motor3;
  RoverMotor m_motor4;
} RoverOutput;


int roverOutputInit(bool _verbose);
int roverOutputFini();

int roverOutputOpen(RoverOutput* _rover, const RoverConfig* _config);
int roverOutputClose(RoverOutput* _rover);
int roverOutputStart(RoverOutput* _rover);
int roverOutputStop(RoverOutput* _rover);
int roverOutputControl(RoverOutput* _rover, int _targetX, int _targetY, int _targetMass);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_ROVER_H_
