#ifndef TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_


#include "internal/module_ce.h"
#include "internal/module_fb.h"
#include "internal/module_v4l2.h"
#include "internal/module_rc.h"
#include "internal/module_rover.h"
#include "internal/module_driver.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct Runtime
{
} Runtime;


bool                     runtimeCfgVerbose(const Runtime* _runtime);
const CodecEngineConfig* runtimeCfgCodecEngine(const Runtime* _runtime);
const V4L2Config*        runtimeCfgV4L2Input(const Runtime* _runtime);
const FBConfig*          runtimeCfgFBOutput(const Runtime* _runtime);
const RCConfig*          runtimeCfgRCInput(const Runtime* _runtime);
const RoverConfig*       runtimeCfgRoverOutput(const Runtime* _runtime);
const DriverOutput*      runtimeCfgDriverOutput(const Runtime* _runtime);


bool runtimeGetTerminate(const Runtime* _runtime);
int  runtimeSetTerminate(const Runtime* _runtime, bool _terminate);
int  runtimeGetAutoTargetDetectParams(const Runtime* _runtime,
                                      float* _detectHueFrom, float* _detectHueTo,
                                      float* _detectSatFrom, float* _detectSatTo,
                                      float* _detectValFrom, float* _detectValTo);
int  runtimeSetAutoTargetDetectParams(Runtime* _runtime,
                                      float _detectHueFrom, float _detectHueTo,
                                      float _detectSatFrom, float _detectSatTo,
                                      float _detectValFrom, float _detectValTo);
int  runtimeGetAutoTargetDetectedLocation(const Runtime* _runtime,
                                          int* _targetX, int* _targetY, int* _targetMass);
int  runtimeSetAutoTargetDetectedLocation(Runtime* _runtime,
                                          int _targetX, int _targetY, int _targetMass);
int  runtimeGetManualControl(const Runtime* _runtime,
                             bool* _ctrlManualMode, int* _ctrlChasisLR, int* _ctrlChasisFB, int* _ctrlHand, int* _ctrlArm);
int  runtimeSetManualControl(Runtime* _runtime,
                             bool _ctrlManualMode, int _ctrlChasisLR, int _ctrlChasisFB, int _ctrlHand, int _ctrlArm);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


#endif // !TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
