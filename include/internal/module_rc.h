#ifndef TRIK_V4L2_DSP_FB_INTERNAL_MODULE_RC_H_
#define TRIK_V4L2_DSP_FB_INTERNAL_MODULE_RC_H_

#include <stdbool.h>

#include "internal/common.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef struct RCConfig // what user wants to set
{
  int m_port;
  bool m_stdin;
  const char* m_eventInput;

  int  m_targetDetectHue;
  int  m_targetDetectHueTolerance;
  int  m_targetDetectSat;
  int  m_targetDetectSatTolerance;
  int  m_targetDetectVal;
  int  m_targetDetectValTolerance;
} RCConfig;

typedef struct RCInput
{
  int                      m_stdinFd;
  int                      m_eventInputFd;
  int                      m_serverFd;
  int                      m_connectionFd;
  char*                    m_readBuffer;
  size_t                   m_readBufferSize;
  size_t                   m_readBufferUsed;

  bool                     m_targetDetectParamsUpdated;
  int                      m_targetDetectHue;
  int                      m_targetDetectHueTolerance;
  int                      m_targetDetectSat;
  int                      m_targetDetectSatTolerance;
  int                      m_targetDetectVal;
  int                      m_targetDetectValTolerance;
} RCInput;




int rcInputInit(bool _verbose);
int rcInputFini();

int rcInputOpen(RCInput* _rc, const RCConfig* _config);
int rcInputClose(RCInput* _rc);
int rcInputStart(RCInput* _rc);
int rcInputStop(RCInput* _rc);

int rcInputReadStdin(RCInput* _rc);
int rcInputReadEventInput(RCInput* _rc);
int rcInputAcceptConnection(RCInput* _rc);
int rcInputReadConnection(RCInput* _rc);

int rcInputGetTargetDetectParams(RCInput* _rc, TargetDetectParams* _targetParams);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_V4L2_DSP_FB_INTERNAL_MODULE_RC_H_
