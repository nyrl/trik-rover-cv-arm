#ifndef TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_


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


bool runtimeGetTerminate(const Runtime* _runtime);
int  runtimeGetDetectedObjectParams(const Runtime* _runtime,
                                    float* _detectHueFrom, float* _detectHueTo,
                                    float* _detectSatFrom, float* _detectSatTo,
                                    float* _detectValFrom, float* _detectValTo);
int  runtimeSetDetectedObjectLocation(Runtime* _runtime,
                                      int _targetX, int _targetY, int _targetMass);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


#endif // !TRIK_V4L2_DSP_FB_INTERNAL_RUNTIME_H_
