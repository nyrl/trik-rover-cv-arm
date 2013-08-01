#ifndef TRIK_V4L2_DSP_FB_INTERNAL_ROVER_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_ROVER_H_

#include <stdbool.h>
#include <time.h>


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct RoverConfigMotorMsp
{
} RoverConfigMotorMsp;

typedef struct RoverConfigMotor
{
  const char* m_path;
  int m_powerBack;
  int m_powerNeutral;
  int m_powerForward;
} RoverConfigMotor;


typedef struct RoverConfig // what user wants to set
{
  RoverConfigMotorMsp m_motorMsp1;
  RoverConfigMotorMsp m_motorMsp2;
  RoverConfigMotor m_motor1;
  RoverConfigMotor m_motor2;
  RoverConfigMotor m_motor3;

  int m_zeroX;
  int m_zeroY;
  int m_zeroMass;
} RoverConfig;




typedef struct RoverMotorMsp
{
} RoverMotorMsp;

typedef struct RoverMotor
{
  int m_fd;
  int m_powerBack;
  int m_powerNeutral;
  int m_powerForward;
} RoverMotor;

typedef struct RoverControlChasis
{
  RoverMotorMsp* m_motorLeft;
  RoverMotorMsp* m_motorRight;

  int         m_lastSpeed; // -100..100
  int         m_lastYaw;   // -100..100
  int         m_zeroX;
  int         m_zeroY;
  int         m_zeroMass;
} RoverControlChasis;

typedef struct RoverControlHand
{
  RoverMotor* m_motor1;
  RoverMotor* m_motor2;

  int         m_lastSpeed; // -100..100
  int         m_zeroY;
} RoverControlHand;

typedef struct RoverControlArm
{
  RoverMotor* m_motor;
  int         m_zeroX;
  int         m_zeroY;
  int         m_zeroMass;
} RoverControlArm;

typedef struct RoverOutput
{
  bool       m_opened;

  RoverMotorMsp m_motorMsp1;
  RoverMotorMsp m_motorMsp2;
  RoverMotor m_motor1;
  RoverMotor m_motor2;
  RoverMotor m_motor3;

  RoverControlChasis m_ctrlChasis;
  RoverControlHand   m_ctrlHand;
  RoverControlArm    m_ctrlArm;

  enum State
  {
    StatePreparing,
    StateSearching,
    StateTracking,
    StateSqueezing,
    StateReleasing
  } m_state;
  struct timespec m_stateEntryTime;

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
