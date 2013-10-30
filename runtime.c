#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "internal/runtime.h"


int runtimeInit(Runtime* _runtime)
{
  memset(&_runtime, 0, sizeof(_runtime));

  _runtime->m_config.m_verbose = false;
  memset(&_runtime->m_config.m_codecEngineConfig, 0, sizeof(_runtime->m_config.m_codecEngineConfig));
  memset(&_runtime->m_config.m_v4l2Config,        0, sizeof(_runtime->m_config.m_v4l2Config));
  memset(&_runtime->m_config.m_fbConfig,          0, sizeof(_runtime->m_config.m_fbConfig));
  memset(&_runtime->m_config.m_rcConfig,          0, sizeof(_runtime->m_config.m_rcConfig));
  memset(&_runtime->m_config.m_roverConfig,       0, sizeof(_runtime->m_config.m_roverConfig));
  memset(&_runtime->m_config.m_driverConfig,      0, sizeof(_runtime->m_config.m_driverConfig));

  memset(&_runtime->m_modules.m_codecEngine,  0, sizeof(_runtime->m_modules.m_codecEngine));
  memset(&_runtime->m_modules.m_v4l2Input,    0, sizeof(_runtime->m_modules.m_v4l2Input));
  _runtime->m_modules.m_v4l2Input.m_fd = -1;
  memset(&_runtime->m_modules.m_fbOutput,     0, sizeof(_runtime->m_modules.m_fbOutput));
  _runtime->m_modules.m_fbOutput.m_fd = -1;
  memset(&_runtime->m_modules.m_rcInput,      0, sizeof(_runtime->m_modules.m_rcInput));
  _runtime->m_modules.m_rcInput.m_stdinFd = -1;
  _runtime->m_modules.m_rcInput.m_eventInputFd = -1;
  _runtime->m_modules.m_rcInput.m_serverFd = -1;
  _runtime->m_modules.m_rcInput.m_connectionFd = -1;
  memset(&_runtime->m_modules.m_roverOutput,  0, sizeof(_runtime->m_modules.m_roverOutput));
  _runtime->m_modules.m_roverOutput.m_motorChasisLeft1.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisLeft2.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisRight1.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisRight2.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorHand1.m_fd = -1;
  _runtime->m_modules.m_roverOutput.m_motorHand2.m_fd = -1;
  _runtime->m_modules.m_roverOutput.m_motorArm.m_fd = -1;
  memset(&_runtime->m_modules.m_driverOutput, 0, sizeof(_runtime->m_modules.m_driverOutput));

  _runtime->m_state.m_terminate = false;
  memset(&_runtime->m_state.m_targetParams,        0, sizeof(_runtime->m_state.m_targetParams));
  memset(&_runtime->m_state.m_targetLocation,      0, sizeof(_runtime->m_state.m_targetLocation));
  memset(&_runtime->m_state.m_driverManualControl, 0, sizeof(_runtime->m_state.m_driverManualControl));

  return 0;
}

int runtimeParseArgs(Runtime* _runtime, int _argc, char* const _argv[])
{
#warning TODO parse command line args
  return EINVAL;
}




bool runtimeCfgVerbose(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return false;

  return _runtime->m_config.m_verbose;
}

const CodecEngineConfig* runtimeCfgCodecEngine(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_codecEngineConfig;
}

const V4L2Config* runtimeCfgV4L2Input(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_v4l2Config;
}

const FBConfig* runtimeCfgFBOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_fbConfig;
}

const RCConfig* runtimeCfgRCInput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_rcConfig;
}

const RoverConfig* runtimeCfgRoverOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_roverConfig;
}

const DriverConfig* runtimeCfgDriverOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_driverConfig;
}




CodecEngine* runtimeModCodecEngine(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_codecEngine;
}

V4L2Input* runtimeModV4L2Input(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_v4l2Input;
}

FBOutput* runtimeModFBOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_fbOutput;
}

RCInput* runtimeModRCInput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_rcInput;
}

RoverOutput* runtimeModRoverOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_roverOutput;
}

DriverOutput* runtimeModDriverOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_driverOutput;
}




bool runtimeGetTerminate(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return true;

  return _runtime->m_state.m_terminate;
}

int runtimeSetTerminate(Runtime* _runtime, bool _terminate)
{
  if (_runtime == NULL)
    return EINVAL;

  _runtime->m_state.m_terminate = _terminate;
  return 0;
}

int runtimeGetTargetDetectParams(const Runtime* _runtime, TargetDetectParams* _targetParams)
{
  if (_runtime == NULL || _targetParams == NULL)
    return EINVAL;

#warning TODO add lock
  *_targetParams = _runtime->m_state.m_targetParams;
  return 0;
}

int runtimeSetTargetDetectParams(Runtime* _runtime, const TargetDetectParams* _targetParams)
{
  if (_runtime == NULL || _targetParams == NULL)
    return EINVAL;

#warning TODO add lock
  _runtime->m_state.m_targetParams = *_targetParams;
  return 0;
}

int runtimeGetTargetLocation(const Runtime* _runtime, TargetLocation* _targetLocation)
{
  if (_runtime == NULL || _targetLocation == NULL)
    return EINVAL;

#warning TODO add lock
  *_targetLocation = _runtime->m_state.m_targetLocation;
  return 0;
}

int runtimeSetTargetLocation(Runtime* _runtime, const TargetLocation* _targetLocation)
{
  if (_runtime == NULL || _targetLocation == NULL)
    return EINVAL;

#warning TODO add lock
  _runtime->m_state.m_targetLocation = *_targetLocation;
  return 0;
}

int runtimeGetDriverManualControl(const Runtime* _runtime, DriverManualControl* _manualControl)
{
  if (_runtime == NULL || _manualControl == NULL)
    return EINVAL;

#warning TODO add lock
  *_manualControl = _runtime->m_state.m_driverManualControl;
  return 0;
}

int runtimeSetDriverManualControl(Runtime* _runtime, const DriverManualControl* _manualControl)
{
  if (_runtime == NULL || _manualControl == NULL)
    return EINVAL;

#warning TODO add lock
  _runtime->m_state.m_driverManualControl = *_manualControl;
  return 0;
}


