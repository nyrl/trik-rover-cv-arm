#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include "internal/ce.h"
#include "internal/fb.h"
#include "internal/v4l2.h"



static sig_atomic_t s_terminate = false;
static void sigterm_action(int _signal, siginfo_t* _siginfo, void* _context)
{
  (void)_signal;
  (void)_siginfo;
  (void)_context;
  s_terminate = true;
}
static void sigterm_action_setup()
{
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_sigaction = &sigterm_action;
  action.sa_flags = SA_SIGINFO|SA_RESTART;

  if (sigaction(SIGTERM, &action, NULL) != 0)
    fprintf(stderr, "sigaction(SIGTERM) failed: %d\n", errno);
  if (sigaction(SIGINT,  &action, NULL) != 0)
    fprintf(stderr, "sigaction(SIGINT) failed: %d\n", errno);
}


static bool s_cfgVerbose = false;
static CodecEngineConfig s_cfgCodecEngine = { "dsp_server.xe674", "vidtranscode_resample" };
static V4L2Config s_cfgV4L2Input = { "/dev/video0", 640, 480, V4L2_PIX_FMT_RGB24 };
static FBConfig s_cfgFBOutput = { "/dev/fb0" };

static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst);


static bool parse_args(int _argc, char* const _argv[])
{
  int opt;
  int longopt;

  static const char* optstring = "vh";
  static const struct option longopts[] =
  {
    { "ce-server",		1,	NULL,	0   },
    { "ce-codec",		1,	NULL,	0   },
    { "v4l2-path",		1,	NULL,	0   },
    { "v4l2-width",		1,	NULL,	0   },
    { "v4l2-height",		1,	NULL,	0   },
    { "v4l2-format",		1,	NULL,	0   },
    { "fb-path",		1,	NULL,	0   },
    { "verbose",		0,	NULL,	'v' },
    { "help",			0,	NULL,	'h' },
  };

  while ((opt = getopt_long(_argc, _argv, optstring, longopts, &longopt)) != -1)
  {
    switch (opt)
    {
      case 'v':	s_cfgVerbose = true;		break;

      case 0:
        switch (longopt)
        {
          case 0: s_cfgCodecEngine.m_serverPath = optarg;	break;
          case 1: s_cfgCodecEngine.m_codecName = optarg;	break;
          case 2: s_cfgV4L2Input.m_path = optarg;		break;
          case 3: s_cfgV4L2Input.m_width = atoi(optarg);	break;
          case 4: s_cfgV4L2Input.m_height = atoi(optarg);	break;
          case 5:
            if	    (!strcasecmp(optarg, "rgb888"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB24;
            else if (!strcasecmp(optarg, "rgb565"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB565;
            else if (!strcasecmp(optarg, "rgb565x"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB565X;
            else if (!strcasecmp(optarg, "yuv444"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_YUV32;
            else if (!strcasecmp(optarg, "yuv422"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_YUYV;
            else
            {
              fprintf(stderr, "Unknown v4l2 format '%s'\n"
                              "Known formats: rgb888, rgb565, rgb565x, yuv444, yuv422\n",
                      optarg);
              return false;
            }
            break;
          case 6: s_cfgFBOutput.m_path = optarg;		break;

          default:
            return false;
        }
        break;

      case 'h':
      default:
        return false;
    }
  }

  return true;
}




int main(int _argc, char* const _argv[])
{
  int res = 0;
  int exit_code = EX_OK;

  sigterm_action_setup();

  if (!parse_args(_argc, _argv))
  {
    fprintf(stderr, "Usage:\n"
                    "    %s <opts>\n",
            _argv[0]);
#warning TODO usage
    exit_code = EX_USAGE;
    goto exit;
  }


  if ((res = codecEngineInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "codecEngineInit() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit;
  }

  if ((res = v4l2InputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "v4l2InputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_ce_fini;
  }

  if ((res = fbOutputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "fbOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_v4l2_fini;
  }


  CodecEngine codecEngine;
  memset(&codecEngine, 0, sizeof(codecEngine));
  if ((res = codecEngineOpen(&codecEngine, &s_cfgCodecEngine)) != 0)
  {
    fprintf(stderr, "codecEngineOpen() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_fini;
  }

  V4L2Input v4l2Src;
  memset(&v4l2Src, 0, sizeof(v4l2Src));
  v4l2Src.m_fd = -1;
  if ((res = v4l2InputOpen(&v4l2Src, &s_cfgV4L2Input)) != 0)
  {
    fprintf(stderr, "v4l2InputOpen() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_ce_close;
  }

  FBOutput fbDst;
  memset(&fbDst, 0, sizeof(fbDst));
  fbDst.m_fd = -1;
  if ((res = fbOutputOpen(&fbDst, &s_cfgFBOutput)) != 0)
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
  if ((res = codecEngineStart(&codecEngine, &s_cfgCodecEngine,
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


  printf("Entering main loop\n");
  while (!s_terminate)
  {
    if ((res = mainLoop(&codecEngine, &v4l2Src, &fbDst)) != 0)
    {
      fprintf(stderr, "mainLoop() failed: %d\n", res);
      exit_code = EX_SOFTWARE;
      break;
    }
  }
  printf("Left main loop\n");


//exit_fb_stop:
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




static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst)
{
  int res;

  fd_set fdsIn;
  FD_ZERO(&fdsIn);
  FD_SET(_v4l2Src->m_fd, &fdsIn);
  int maxFd = _v4l2Src->m_fd+1;

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if ((res = select(maxFd, &fdsIn, NULL, NULL, &timeout)) < 0)
  {
    res = errno;
    fprintf(stderr, "select() failed: %d\n", res);
    return res;
  }

  if (FD_ISSET(_v4l2Src->m_fd, &fdsIn))
  {
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

    size_t frameDstUsed = frameDstSize;
    if ((res = codecEngineTranscodeFrame(_ce,
                                         frameSrcPtr, frameSrcSize,
                                         frameDstPtr, frameDstSize, &frameDstUsed)) != 0)
    {
      fprintf(stderr, "codecEngineTranscodeFrame(%p[%zu] -> %p[%zu]) failed: %d\n",
              frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, res);
      return res;
    }

    if (s_cfgVerbose)
      fprintf(stderr, "Transcoded frame %p[%zu] -> %p[%zu/%zu]\n",
              frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, frameDstUsed);

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
  }

  return 0;
}





