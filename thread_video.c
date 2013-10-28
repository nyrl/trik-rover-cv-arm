#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/select.h>

#include "internal/ce.h"
#include "internal/fb.h"
#include "internal/v4l2.h"
#include "internal/rc.h"
#include "internal/rover.h"
#include "internal/runtime.h"




static int video_loop(Runtime* _runtime,
                      CodecEngine* _ce,
                      V4L2Input* _v4l2Src,
                      FBOutput* _fbDst)
{
  int res;
  int maxFd = 0;
  fd_set fdsIn;
  static const struct timespec s_select_timeout = { .tv_sec=1, .tv_nsec=0 };

  assert(_runtime != NULL && _ce != NULL && _v4l2Src != NULL && _fbDst != NULL);

  FD_ZERO(&fdsIn);

  FD_SET(_v4l2Src->m_fd, &fdsIn);
  if (maxFd < _v4l2Src->m_fd)
    maxFd = _v4l2Src->m_fd;

  if ((res = pselect(maxFd+1, &fdsIn, NULL, NULL, &s_select_timeout, NULL)) < 0)
  {
    res = errno;
    fprintf(stderr, "pselect() failed: %d\n", res);
    return res;
  }

  if (!FD_ISSET(_v4l2Src->m_fd, &fdsIn))
  {
    fprintf(stderr, "pselect() did not select V4L2\n");
    return EBUSY;
  }

  const void* frameSrcPtr;
  size_t frameSrcSize;
  size_t frameSrcIndex;
  if ((res = v4l2InputGetFrame(_v4l2Src, &frameSrcPtr, &frameSrcSize, &frameSrcIndex)) != 0)
  {
    fprintf(stderr, "v4l2InputGetFrame() failed: %d\n", res);
    return res;
  }

  void* frameDstPtr;
  size_t frameDstSize;
  if ((res = fbOutputGetFrame(_fbDst, &frameDstPtr, &frameDstSize)) != 0)
  {
    fprintf(stderr, "fbOutputGetFrame() failed: %d\n", res);
    return res;
  }


  float detectHueFrom;
  float detectHueTo;
  float detectSatFrom;
  float detectSatTo;
  float detectValFrom;
  float detectValTo;
  if ((res = runtimeGetDetectedObjectParams(_runtime,
                                            &detectHueFrom, &detectHueTo,
                                            &detectSatFrom, &detectSatTo,
                                            &detectValFrom, &detectValTo)) != 0)
  {
    fprintf(stderr, "runtimeGetDetectParams() failed: %d\n", res);
    return res;
  }

  size_t frameDstUsed = frameDstSize;
  int targetX;
  int targetY;
  int targetMass;

  if ((res = codecEngineTranscodeFrame(_ce,
                                       frameSrcPtr, frameSrcSize,
                                       frameDstPtr, frameDstSize, &frameDstUsed,
                                       detectHueFrom, detectHueTo,
                                       detectSatFrom, detectSatTo,
                                       detectValFrom, detectValTo,
                                       &targetX, &targetY, &targetMass)) != 0)
  {
    fprintf(stderr, "codecEngineTranscodeFrame(%p[%zu] -> %p[%zu]) failed: %d\n",
            frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, res);
    return res;
  }


  if ((res = fbOutputPutFrame(_fbDst)) != 0)
  {
    fprintf(stderr, "fbOutputPutFrame() failed: %d\n", res);
    return res;
  }

  if ((res = v4l2InputPutFrame(_v4l2Src, frameSrcIndex)) != 0)
  {
    fprintf(stderr, "v4l2InputPutFrame() failed: %d\n", res);
    return res;
  }


  if ((res = runtimeSetDetectedObjectLocation(_runtime, targetX, targetY, targetMass)) != 0)
  {
    fprintf(stderr, "runtimeSetDetectedObjectLocation() failed: %d\n", res);
    return res;
  }

  return 0;
}




static int video_thread(void* _arg)
{
  int res = 0;
  int exit_code = EX_OK;
  Runtime* runtime = (Runtime*)_arg;
  struct timespec last_fps_report_time;

  if ((res = codecEngineInit(runtimeCfgVerbose(runtime))) != 0)
  {
    fprintf(stderr, "codecEngineInit() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit;
  }

  if ((res = v4l2InputInit(runtimeCfgVerbose(runtime))) != 0)
  {
    fprintf(stderr, "v4l2InputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_ce_fini;
  }

  if ((res = fbOutputInit(runtimeCfgVerbose(runtime))) != 0)
  {
    fprintf(stderr, "fbOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_v4l2_fini;
  }


  CodecEngine codecEngine;
  memset(&codecEngine, 0, sizeof(codecEngine));
  codecEngine.m_handle = 0;
  if ((res = codecEngineOpen(&codecEngine, runtimeCfgCodecEngine(runtime))) != 0)
  {
    fprintf(stderr, "codecEngineOpen() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_fini;
  }

  V4L2Input v4l2Src;
  memset(&v4l2Src, 0, sizeof(v4l2Src));
  v4l2Src.m_fd = -1;
  if ((res = v4l2InputOpen(&v4l2Src, runtimeCfgV4L2Input(runtime))) != 0)
  {
    fprintf(stderr, "v4l2InputOpen() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_ce_close;
  }

  FBOutput fbDst;
  memset(&fbDst, 0, sizeof(fbDst));
  fbDst.m_fd = -1;
  if ((res = fbOutputOpen(&fbDst, runtimeCfgFBOutput(runtime))) != 0)
  {
    fprintf(stderr, "fbOutputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_v4l2_close;
  }


  size_t srcWidth, srcHeight, srcLineLength, srcImageSize;
  size_t dstWidth, dstHeight, dstLineLength, dstImageSize;
  uint32_t srcFormat, dstFormat;
  if ((res = v4l2InputGetFormat(&v4l2Src, &srcWidth, &srcHeight, &srcLineLength, &srcImageSize, &srcFormat)) != 0)
  {
    fprintf(stderr, "v4l2InputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_close;
  }
  if ((res = fbOutputGetFormat(&fbDst, &dstWidth, &dstHeight, &dstLineLength, &dstImageSize, &dstFormat)) != 0)
  {
    fprintf(stderr, "fbOutputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_close;
  }
  if ((res = codecEngineStart(&codecEngine, runtimeCfgCodecEngine(runtime),
                              srcWidth, srcHeight, srcLineLength, srcImageSize, srcFormat,
                              dstWidth, dstHeight, dstLineLength, dstImageSize, dstFormat)) != 0)
  {
    fprintf(stderr, "codecEngineStart() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_close;
  }

  if ((res = v4l2InputStart(&v4l2Src)) != 0)
  {
    fprintf(stderr, "v4l2InputStart() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_ce_stop;
  }

  if ((res = fbOutputStart(&fbDst)) != 0)
  {
    fprintf(stderr, "fbOutputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_v4l2_stop;
  }


  if ((res = clock_gettime(CLOCK_MONOTONIC, &last_fps_report_time)) != 0)
  {
    fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);
    exit_code = EX_IOERR;
    goto exit_fb_stop;
  }


  printf("Entering video thread loop\n");
  while (!runtimeGetTerminate(runtime))
  {
    struct timespec now;
    long long last_fps_report_elapsed_ms;

    if ((res = clock_gettime(CLOCK_MONOTONIC, &now)) != 0)
    {
      fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);
      exit_code = EX_IOERR;
      goto exit_fb_stop;
    }

    last_fps_report_elapsed_ms = (now.tv_sec  - last_fps_report_time.tv_sec )*1000
                               + (now.tv_nsec - last_fps_report_time.tv_nsec)/1000000;
    if (last_fps_report_elapsed_ms >= 5*1000)
    {
      last_fps_report_time.tv_sec += 5;

      if ((res = codecEngineReportLoad(&codecEngine, last_fps_report_elapsed_ms)) != 0)
        fprintf(stderr, "codecEngineReportLoad() failed: %d\n", res);

      if ((res = v4l2InputReportFPS(&v4l2Src, last_fps_report_elapsed_ms)) != 0)
        fprintf(stderr, "v4l2InputReportFPS() failed: %d\n", res);
    }


    if ((res = video_loop(runtime, &codecEngine, &v4l2Src, &fbDst)) != 0)
    {
      fprintf(stderr, "video_loop() failed: %d\n", res);
      exit_code = EX_SOFTWARE;
      break;
    }
  }
  printf("Left video thread loop\n");


exit_fb_stop:
  if ((res = fbOutputStop(&fbDst)) != 0)
    fprintf(stderr, "fbOutputStop() failed: %d\n", res);

exit_v4l2_stop:
  if ((res = v4l2InputStop(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputStop() failed: %d\n", res);

exit_ce_stop:
  if ((res = codecEngineStop(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineStop() failed: %d\n", res);


exit_fb_close:
  if ((res = fbOutputClose(&fbDst)) != 0)
    fprintf(stderr, "fbOutputClose() failed: %d\n", res);

exit_v4l2_close:
  if ((res = v4l2InputClose(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputClose() failed: %d\n", res);

exit_ce_close:
  if ((res = codecEngineClose(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineClose() failed: %d\n", res);


exit_fb_fini:
  if ((res = fbOutputFini()) != 0)
    fprintf(stderr, "fbOutputFini() failed: %d\n", res);

exit_v4l2_fini:
  if ((res = v4l2InputFini()) != 0)
    fprintf(stderr, "v4l2InputFini() failed: %d\n", res);

exit_ce_fini:
  if ((res = codecEngineFini()) != 0)
    fprintf(stderr, "codecEngineFini() failed: %d\n", res);


exit:
  return exit_code;
}




