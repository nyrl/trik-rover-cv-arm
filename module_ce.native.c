#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <linux/videodev2.h>

#include "internal/module_ce.h"


int codecEngineInit(bool _verbose)
{
  return 0;
}

int codecEngineFini()
{
  return 0;
}


int codecEngineOpen(CodecEngine* _ce, const CodecEngineConfig* _config)
{
  if (_ce == NULL || _config == NULL)
    return EINVAL;

  return 0;
}

int codecEngineClose(CodecEngine* _ce)
{
  if (_ce == NULL)
    return EINVAL;

  return 0;
}


int codecEngineStart(CodecEngine* _ce, const CodecEngineConfig* _config,
                     const ImageDescription* _srcImageDesc,
                     const ImageDescription* _dstImageDesc)
{
  if (_ce == NULL || _config == NULL || _srcImageDesc == NULL || _dstImageDesc == NULL)
    return EINVAL;

  return 0;
}

int codecEngineStop(CodecEngine* _ce)
{
  if (_ce == NULL)
    return EINVAL;

  return 0;
}

int codecEngineTranscodeFrame(CodecEngine* _ce,
                              const void* _srcFramePtr, size_t _srcFrameSize,
                              void* _dstFramePtr, size_t _dstFrameSize, size_t* _dstFrameUsed,
                              const TargetDetectParams* _targetParams,
                              TargetLocation* _targetLocation)
{
  if (_ce == NULL || _targetParams == NULL || _targetLocation == NULL)
    return EINVAL;

  _targetLocation->m_targetX = 0;
  _targetLocation->m_targetY = 0;
  _targetLocation->m_targetMass = -1;

  return 0;
}

int codecEngineReportLoad(const CodecEngine* _ce, long long _ms)
{
  if (_ce == NULL)
    return EINVAL;

  return 0;
}


