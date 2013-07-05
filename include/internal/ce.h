#ifndef TRIK_V4L2_DSP_FB_INTERNAL_CE_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_CE_H_

#include <stdbool.h>
#include <xdc/std.h>
#include <ti/xdais/xdas.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/vidtranscode/vidtranscode.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct CodecEngineConfig // what user wants to set
{
  const char* m_serverPath;
  const char* m_codecName;
} CodecEngineConfig;

typedef struct CodecEngine
{
  Engine_Handle m_handle;

  Memory_AllocParams m_allocParams;
  size_t     m_srcBufferSize;
  void*      m_srcBuffer;

  size_t     m_dstBufferSize;
  void*      m_dstBuffer;

  VIDTRANSCODE_Handle m_vidtranscodeHandle;

} CodecEngine;




int codecEngineInit(bool _verbose);
int codecEngineFini();

int codecEngineOpen(CodecEngine* _ce, const CodecEngineConfig* _config);
int codecEngineClose(CodecEngine* _ce);
int codecEngineStart(CodecEngine* _ce, const CodecEngineConfig* _config, size_t _srcBufferSize, size_t _dstBufferSize);
int codecEngineStop(CodecEngine* _ce);

int codecEngineTranscodeFrame(CodecEngine* _ce,
                              const void* _frameSrcPtr, size_t _frameSrcSize,
                              void* _frameDstPtr, size_t _frameDstSize, size_t* _frameDstUsed);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_CE_H_
