#ifndef TRIK_V4L2_DSP_FB_INTERNAL_FB_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_FB_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct FBConfig // what user wants to set
{
  const char* m_path;
} FBConfig;

typedef struct FBOutput
{
  int m_fd;
} FBOutput;




int fbOutputInit(bool _verbose);
int fbOutputFini();

int fbOutputOpen(FBOutput* _fb, const FBConfig* _config);
int fbOutputClose(FBOutput* _fb);
int fbOutputStart(FBOutput* _fb);
int fbOutputStop(FBOutput* _fb);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_FB_H_
