#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#include "internal/rc.h"



int rcInputInit(bool _verbose)
{
  (void)_verbose;
  return 0;
}

int rcInputFini()
{
  return 0;
}

int rcInputOpen(RCInput* _rc, const RCConfig* _config)
{
  int res = 0;

  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd != -1)
    return EALREADY;

#warning TODO open

  _rc->m_manualMode = _config->m_manualMode;
  _rc->m_autoTargetDetectHue          = _config->m_autoTargetDetectHue;
  _rc->m_autoTargetDetectHueTolerance = _config->m_autoTargetDetectHueTolerance;
  _rc->m_autoTargetDetectSat          = _config->m_autoTargetDetectSat;
  _rc->m_autoTargetDetectSatTolerance = _config->m_autoTargetDetectSatTolerance;
  _rc->m_autoTargetDetectVal          = _config->m_autoTargetDetectVal;
  _rc->m_autoTargetDetectValTolerance = _config->m_autoTargetDetectValTolerance;

  return 0;
}

int rcInputClose(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return EALREADY;

#warning TODO

  return 0;
}

int rcInputStart(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

#warning TODO

  _rc->m_manualCtrlChasisLR = 0;
  _rc->m_manualCtrlChasisFB = 0;
  _rc->m_manualCtrlHand = 0;
  _rc->m_manualCtrlArm = 0;

  return 0;
}

int rcInputStop(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

#warning TODO

  return 0;
}

bool rcInputIsManualMode(RCInput* _rc)
{
  if (_rc == NULL)
    return false;

  return _rc->m_manualMode;
}

int rcInputGetManualCommand(RCInput* _rc, int* _ctrlChasisLR, int* _ctrlChasisFB, int* _ctrlHand, int* _ctrlArm)
{
  if (_rc == NULL)
    return EINVAL;

  if (!_rc->m_manualMode)
    return EADDRINUSE;

  if (_ctrlChasisLR)
    *_ctrlChasisLR = _rc->m_manualCtrlChasisLR;
  if (_ctrlChasisFB)
    *_ctrlChasisFB = _rc->m_manualCtrlChasisFB;
  if (_ctrlHand)
    *_ctrlHand = _rc->m_manualCtrlHand;
  if (_ctrlArm)
    *_ctrlArm = _rc->m_manualCtrlArm;

  return 0;
}

int rcInputGetAutoTargetDetect(RCInput* _rc,
                               float* _detectHueFrom, float* _detectHueTo,
                               float* _detectSatFrom, float* _detectSatTo,
                               float* _detectValFrom, float* _detectValTo)
{
  if (_rc == NULL)
    return EINVAL;

  // valid both in auto and manual mode!
  *_detectHueFrom = _rc->m_autoTargetDetectHue-_rc->m_autoTargetDetectHueTolerance;
  *_detectHueTo   = _rc->m_autoTargetDetectHue+_rc->m_autoTargetDetectHueTolerance;
  while (*_detectHueFrom < 0.0f)
    *_detectHueFrom += 360.0f;
  while (*_detectHueFrom >= 360.0f)
    *_detectHueFrom -= 360.0f;
  while (*_detectHueTo < 0.0f)
    *_detectHueTo += 360.0f;
  while (*_detectHueTo >= 360.0f)
    *_detectHueTo -= 360.0f;

  *_detectSatFrom = _rc->m_autoTargetDetectSat-_rc->m_autoTargetDetectSatTolerance;
  *_detectSatTo   = _rc->m_autoTargetDetectSat+_rc->m_autoTargetDetectSatTolerance;
  *_detectValFrom = _rc->m_autoTargetDetectVal-_rc->m_autoTargetDetectValTolerance;
  *_detectValTo   = _rc->m_autoTargetDetectVal+_rc->m_autoTargetDetectValTolerance;

  return 0;
}

