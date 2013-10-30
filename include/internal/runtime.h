#ifndef TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_


#include "internal/common.h"
#include "internal/module_ce.h"
#include "internal/module_fb.h"
#include "internal/module_v4l2.h"
#include "internal/module_rc.h"
#include "internal/module_rover.h"
#include "internal/module_driver.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct RuntimeConfig
{
  bool               m_verbose;

  CodecEngineConfig  m_codecEngineConfig;
  V4L2Config         m_v4l2Config;
  FBConfig           m_fbConfig;
  RCConfig           m_rcConfig;
  RoverConfig        m_roverConfig;
  DriverConfig       m_driverConfig;
} RuntimeConfig;

typedef struct RuntimeModules
{
  CodecEngine  m_codecEngine;
  V4L2Input    m_v4l2Input;
  FBOutput     m_fbOutput;
  RCInput      m_rcInput;
  RoverOutput  m_roverOutput;
  DriverOutput m_driverOutput;
} RuntimeModules;

typedef struct RuntimeState
{
  volatile bool       m_terminate;

  TargetDetectParams  m_targetParams;
  TargetLocation      m_targetLocation;
  DriverManualControl m_driverManualControl;
} RuntimeState;

typedef struct Runtime
{
  RuntimeConfig  m_config;
  RuntimeModules m_modules;
  RuntimeState   m_state;

} Runtime;


int runtimeReset(Runtime* _runtime);
int runtimeParseArgs(Runtime* _runtime, int _argc, char* const _argv[]);

int runtimeInit(Runtime* _runtime);
int runtimeFini(Runtime* _runtime);


bool                     runtimeCfgVerbose(const Runtime* _runtime);
const CodecEngineConfig* runtimeCfgCodecEngine(const Runtime* _runtime);
const V4L2Config*        runtimeCfgV4L2Input(const Runtime* _runtime);
const FBConfig*          runtimeCfgFBOutput(const Runtime* _runtime);
const RCConfig*          runtimeCfgRCInput(const Runtime* _runtime);
const RoverConfig*       runtimeCfgRoverOutput(const Runtime* _runtime);
const DriverConfig*      runtimeCfgDriverOutput(const Runtime* _runtime);

CodecEngine*  runtimeModCodecEngine(Runtime* _runtime);
V4L2Input*    runtimeModV4L2Input(Runtime* _runtime);
FBOutput*     runtimeModFBOutput(Runtime* _runtime);
RCInput*      runtimeModRCInput(Runtime* _runtime);
RoverOutput*  runtimeModRoverOutput(Runtime* _runtime);
DriverOutput* runtimeModDriverOutput(Runtime* _runtime);


bool runtimeGetTerminate(const Runtime* _runtime);
int  runtimeSetTerminate(Runtime* _runtime, bool _terminate);
int  runtimeGetTargetDetectParams(const Runtime* _runtime, TargetDetectParams* _targetParams);
int  runtimeSetTargetDetectParams(Runtime* _runtime, const TargetDetectParams* _targetParams);
int  runtimeGetTargetLocation(const Runtime* _runtime, TargetLocation* _targetLocation);
int  runtimeSetTargetLocation(Runtime* _runtime, const TargetLocation* _targetLocation);
int  runtimeGetDriverManualControl(const Runtime* _runtime, DriverManualControl* _manualControl);
int  runtimeSetDriverManualControl(Runtime* _runtime, const DriverManualControl* _manualControl);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


#endif // !TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
