#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "internal/ce.h"
#include "internal/fb.h"
#include "internal/v4l2.h"
#include "internal/rover.h"



static sig_atomic_t s_terminate = false;
static void sigterm_action(int _signal, siginfo_t* _siginfo, void* _context)
{
  (void)_signal;
  (void)_siginfo;
  (void)_context;
  s_terminate = true;
}
static void sigactions_setup()
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
static V4L2Config s_cfgV4L2Input = { "/dev/video0", 640, 480, V4L2_PIX_FMT_YUYV };
static FBConfig s_cfgFBOutput = { "/dev/fb0" };
static RoverConfig s_cfgRoverOutput = { { "/sys/class/pwm/ecap.1/duty_ns",     700000, 1500000, 2300000 }, //left
                                        { "/sys/class/pwm/ecap.2/duty_ns",     700000, 1500000, 2300000 }, //right
                                        { "/sys/class/pwm/ehrpwm.1:0/duty_ns", 700000, 1500000, 2300000 }, //up-down
                                        { "/sys/class/pwm/ecap.0/duty_ns",     700000, 1500000, 2300000 }, //squeeze
                                        0, 0, 2000 };

static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RoverOutput* _rover);


static bool parse_args(int _argc, char* const _argv[])
{
  int opt;
  int longopt;

  static const char* optstring = "vh";
  static const struct option longopts[] =
  {
    { "ce-server",		1,	NULL,	0   }, // 0
    { "ce-codec",		1,	NULL,	0   },
    { "v4l2-path",		1,	NULL,	0   }, // 2
    { "v4l2-width",		1,	NULL,	0   },
    { "v4l2-height",		1,	NULL,	0   },
    { "v4l2-format",		1,	NULL,	0   },
    { "fb-path",		1,	NULL,	0   }, // 6
    { "rover-m1-path",		1,	NULL,	0   }, // 7
    { "rover-m1-back",		1,	NULL,	0   },
    { "rover-m1-neutral",	1,	NULL,	0   },
    { "rover-m1-forward",	1,	NULL,	0   },
    { "rover-m2-path",		1,	NULL,	0   }, // 11
    { "rover-m2-back",		1,	NULL,	0   },
    { "rover-m2-neutral",	1,	NULL,	0   },
    { "rover-m2-forward",	1,	NULL,	0   },
    { "rover-m3-path",		1,	NULL,	0   }, // 15
    { "rover-m3-back",		1,	NULL,	0   },
    { "rover-m3-neutral",	1,	NULL,	0   },
    { "rover-m3-forward",	1,	NULL,	0   },
    { "rover-m4-path",		1,	NULL,	0   }, // 19
    { "rover-m4-back",		1,	NULL,	0   },
    { "rover-m4-neutral",	1,	NULL,	0   },
    { "rover-m4-forward",	1,	NULL,	0   },
    { "rover-zero-x",		1,	NULL,	0   }, // 23
    { "rover-zero-y",		1,	NULL,	0   },
    { "rover-zero-mass",	1,	NULL,	0   },
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

          case 7:  s_cfgRoverOutput.m_motor1.m_path = optarg;			break;
          case 8:  s_cfgRoverOutput.m_motor1.m_powerBack = atoi(optarg);	break;
          case 9:  s_cfgRoverOutput.m_motor1.m_powerNeutral = atoi(optarg);	break;
          case 10: s_cfgRoverOutput.m_motor1.m_powerForward = atoi(optarg);	break;

          case 11: s_cfgRoverOutput.m_motor2.m_path = optarg;			break;
          case 12: s_cfgRoverOutput.m_motor2.m_powerBack = atoi(optarg);	break;
          case 13: s_cfgRoverOutput.m_motor2.m_powerNeutral = atoi(optarg);	break;
          case 14: s_cfgRoverOutput.m_motor2.m_powerForward = atoi(optarg);	break;

          case 15: s_cfgRoverOutput.m_motor3.m_path = optarg;			break;
          case 16: s_cfgRoverOutput.m_motor3.m_powerBack = atoi(optarg);	break;
          case 17: s_cfgRoverOutput.m_motor3.m_powerNeutral = atoi(optarg);	break;
          case 18: s_cfgRoverOutput.m_motor3.m_powerForward = atoi(optarg);	break;

          case 19: s_cfgRoverOutput.m_motor3.m_path = optarg;			break;
          case 20: s_cfgRoverOutput.m_motor3.m_powerBack = atoi(optarg);	break;
          case 21: s_cfgRoverOutput.m_motor3.m_powerNeutral = atoi(optarg);	break;
          case 22: s_cfgRoverOutput.m_motor3.m_powerForward = atoi(optarg);	break;

          case 23: s_cfgRoverOutput.m_zeroX    = atoi(optarg);	break;
          case 24: s_cfgRoverOutput.m_zeroY    = atoi(optarg);	break;
          case 25: s_cfgRoverOutput.m_zeroMass = atoi(optarg);	break;

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

  sigactions_setup();

  if (!parse_args(_argc, _argv))
  {
    fprintf(stderr, "Usage:\n"
                    "    %s <opts>\n"
                    " where opts are:\n"
                    "   --ce-server    <dsp-server-name>\n"
                    "   --ce-codec     <dsp-codec-name>\n"
                    "   --v4l2-path    <input-device-path>\n"
                    "   --v4l2-width   <input-width>\n"
                    "   --v4l2-height  <input-height>\n"
                    "   --v4l2-format  <input-pixel-format>\n"
                    "   --fb-path      <output-device-path>\n"
                    "   --rover-mN-path        <rover-motorN-path>\n"
                    "   --rover-mN-back        <rover-motorN-full-back-power>\n"
                    "   --rover-mN-neutral     <rover-motorN-neutral-power>\n"
                    "   --rover-mN-forward     <rover-motorN-full-forward-power>\n"
                    "   --rover-zero-x         <rover-center-X>\n"
                    "   --rover-zero-y         <rover-center-Y>\n"
                    "   --rover-zero-mass      <rover-center-mass>\n"
                    "   --verbose\n"
                    "   --help\n",
            _argv[0]);
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

  if ((res = roverOutputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "roverOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_fb_fini;
  }


  CodecEngine codecEngine;
  memset(&codecEngine, 0, sizeof(codecEngine));
  if ((res = codecEngineOpen(&codecEngine, &s_cfgCodecEngine)) != 0)
  {
    fprintf(stderr, "codecEngineOpen() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_fini;
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

  RoverOutput rover;
  memset(&rover, 0, sizeof(rover));
  rover.m_motor1.m_fd = -1;
  rover.m_motor2.m_fd = -1;
  rover.m_motor3.m_fd = -1;
  rover.m_motor4.m_fd = -1;
  if ((res = roverOutputOpen(&rover, &s_cfgRoverOutput)) != 0)
  {
    fprintf(stderr, "roverOutputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_close;
  }


  size_t srcWidth, srcHeight, srcLineLength, srcImageSize;
  size_t dstWidth, dstHeight, dstLineLength, dstImageSize;
  uint32_t srcFormat, dstFormat;
  if ((res = v4l2InputGetFormat(&v4l2Src, &srcWidth, &srcHeight, &srcLineLength, &srcImageSize, &srcFormat)) != 0)
  {
    fprintf(stderr, "v4l2InputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
  }
  if ((res = fbOutputGetFormat(&fbDst, &dstWidth, &dstHeight, &dstLineLength, &dstImageSize, &dstFormat)) != 0)
  {
    fprintf(stderr, "fbOutputGetFormat() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
  }
  if ((res = codecEngineStart(&codecEngine, &s_cfgCodecEngine,
                              srcWidth, srcHeight, srcLineLength, srcImageSize, srcFormat,
                              dstWidth, dstHeight, dstLineLength, dstImageSize, dstFormat)) != 0)
  {
    fprintf(stderr, "codecEngineStart() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_rover_close;
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

  if ((res = roverOutputStart(&rover)) != 0)
  {
    fprintf(stderr, "roverOutputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_stop;
  }


  printf("Entering main loop\n");
  struct timespec lastReportTime;
  if (clock_gettime(CLOCK_MONOTONIC, &lastReportTime) != 0)
    fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);

  while (!s_terminate)
  {
    struct timespec currentTime;
    if (clock_gettime(CLOCK_MONOTONIC, &currentTime) != 0)
      fprintf(stderr, "clock_gettime(CLOCK_MONOTONIC) failed: %d\n", errno);
    else if (currentTime.tv_sec > lastReportTime.tv_sec+5) // approx check that ~5sec elapsed
    {
      unsigned long long elapsedMs = currentTime.tv_sec - lastReportTime.tv_sec;
      elapsedMs *= 1000;
      elapsedMs += currentTime.tv_nsec/1000000;
      elapsedMs -= lastReportTime.tv_nsec/1000000;

      lastReportTime = currentTime;
      if ((res = codecEngineReportLoad(&codecEngine)) != 0)
        fprintf(stderr, "codecEngineReportLoad() failed: %d\n", res);

      if ((res = v4l2InputReportFPS(&v4l2Src, elapsedMs)) != 0)
        fprintf(stderr, "v4l2InputReportFPS() failed: %d\n", res);
    }


    if ((res = mainLoop(&codecEngine, &v4l2Src, &fbDst, &rover)) != 0)
    {
      fprintf(stderr, "mainLoop() failed: %d\n", res);
      exit_code = EX_SOFTWARE;
      break;
    }
  }
  printf("Left main loop\n");


//exit_rover_stop:
  if ((res = roverOutputStop(&rover)) != 0)
    fprintf(stderr, "roverOutputStop() failed: %d\n", res);

exit_fb_stop:
  if ((res = fbOutputStop(&fbDst)) != 0)
    fprintf(stderr, "fbOutputStop() failed: %d\n", res);

exit_v4l2_stop:
  if ((res = v4l2InputStop(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputStop() failed: %d\n", res);

exit_ce_stop:
  if ((res = codecEngineStop(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineStop() failed: %d\n", res);


exit_rover_close:
  if ((res = roverOutputClose(&rover)) != 0)
    fprintf(stderr, "roverOutputClose() failed: %d\n", res);

exit_fb_close:
  if ((res = fbOutputClose(&fbDst)) != 0)
    fprintf(stderr, "fbOutputClose() failed: %d\n", res);

exit_v4l2_close:
  if ((res = v4l2InputClose(&v4l2Src)) != 0)
    fprintf(stderr, "v4l2InputClose() failed: %d\n", res);

exit_ce_close:
  if ((res = codecEngineClose(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineClose() failed: %d\n", res);


exit_rover_fini:
  if ((res = roverOutputFini()) != 0)
    fprintf(stderr, "roverOutputFini() failed: %d\n", res);

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




static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RoverOutput* _rover)
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
    int targetX;
    int targetY;
    int targetMass;
    if ((res = codecEngineTranscodeFrame(_ce,
                                         frameSrcPtr, frameSrcSize,
                                         frameDstPtr, frameDstSize, &frameDstUsed,
                                         &targetX, &targetY, &targetMass)) != 0)
    {
      fprintf(stderr, "codecEngineTranscodeFrame(%p[%zu] -> %p[%zu]) failed: %d\n",
              frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, res);
      return res;
    }

    if (s_cfgVerbose)
    {
      fprintf(stderr, "Transcoded frame %p[%zu] -> %p[%zu/%zu]\n",
              frameSrcPtr, frameSrcSize, frameDstPtr, frameDstSize, frameDstUsed);
      if (targetMass > 0)
        fprintf(stderr, "Target detected at %d x %d @ %d\n", targetX, targetY, targetMass);
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

    if ((res = roverOutputControl(_rover, targetX, targetY, targetMass)) != 0)
    {
      fprintf(stderr, "roverOutputControl() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}


