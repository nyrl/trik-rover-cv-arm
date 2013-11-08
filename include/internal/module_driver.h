#ifndef TRIK_V4L2_DSP_FB_INTERNAL_MODULE_DRIVER_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_MODULE_DRIVER_H_

#include <stdbool.h>
#include <time.h>

#include "internal/common.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct DriverConfig // what user wants to set
{
  int m_zeroX;
  int m_zeroY;
  int m_zeroSize;
} DriverConfig;


typedef struct DriverCtrlChasis
{
  int         m_motorSpeedLeft;
  int         m_motorSpeedRight;
} DriverCtrlChasis;

typedef struct DriverCtrlHand
{
  int         m_motorSpeed;
} DriverCtrlHand;

typedef struct DriverCtrlArm
{
  int         m_motorSpeed;
} DriverCtrlArm;

typedef struct DriverOutput
{
  int              m_zeroX;
  int              m_zeroY;
  int              m_zeroSize;

  DriverCtrlChasis m_ctrlChasis;
  DriverCtrlHand   m_ctrlHand;
  DriverCtrlArm    m_ctrlArm;

  enum State
  {
    StateManual,
    StatePreparing,
    StateSearching,
    StateTracking,
    StateSqueezing,
    StateReleasing
  } m_state;
  struct timespec m_stateEntryTime;

} DriverOutput;


int driverOutputInit(bool _verbose);
int driverOutputFini();

int driverOutputOpen(DriverOutput* _driver, const DriverConfig* _config);
int driverOutputClose(DriverOutput* _driver);
int driverOutputStart(DriverOutput* _driver);
int driverOutputStop(DriverOutput* _driver);

int driverOutputControl(DriverOutput* _driver,
                        const DriverManualControl* _manualControl,
                        const TargetLocation* _targetLocation);

int driverOutputGetRoverControl(const DriverOutput* _driver, RoverControl* _roverControl);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_MODULE_DRIVER_H_
