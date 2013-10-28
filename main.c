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
#include "internal/rc.h"
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
static V4L2Config s_cfgV4L2Input = { "/dev/video0", 352, 288, V4L2_PIX_FMT_YUYV };
static FBConfig s_cfgFBOutput = { "/dev/fb0" };
static RoverConfig s_cfgRoverOutput = { { 2, 0x48, 0x14, 0x00, 0x64 }, //msp left1
                                        { 2, 0x48, 0x15, 0x00, 0x64 }, //msp left2
                                        { 2, 0x48, 0x16, 0x00, 0x64 }, //msp right1
                                        { 2, 0x48, 0x17, 0x00, 0x64 }, //msp right2
                                        { "/sys/class/pwm/ecap.0/duty_ns",     2300000, 1600000, 0, 1400000, 700000  }, //up-down m1
                                        { "/sys/class/pwm/ecap.1/duty_ns",     700000,  1400000, 0, 1600000, 2300000 }, //up-down m2
                                        { "/sys/class/pwm/ehrpwm.1:1/duty_ns", 700000,  1400000, 0, 1600000, 2300000 }, //squeeze
                                        0, 50, 600 };
static RCConfig s_cfgRCInput = { 4444, false, "/dev/input/by-path/platform-gpio-keys-event", false, 7.0f, 13.0f, 0.85f, 0.15f, 0.8f, 0.2f };


static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover);


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
    { "rover-m1-back-full",	1,	NULL,	0   },
    { "rover-m1-back-zero",	1,	NULL,	0   },
    { "rover-m1-neutral",	1,	NULL,	0   },
    { "rover-m1-forward-zero",	1,	NULL,	0   },
    { "rover-m1-forward-full",	1,	NULL,	0   },
    { "rover-m2-path",		1,	NULL,	0   }, // 13
    { "rover-m2-back-full",	1,	NULL,	0   },
    { "rover-m2-back-zero",	1,	NULL,	0   },
    { "rover-m2-neutral",	1,	NULL,	0   },
    { "rover-m2-forward-zero",	1,	NULL,	0   },
    { "rover-m2-forward-full",	1,	NULL,	0   },
    { "rover-m3-path",		1,	NULL,	0   }, // 19
    { "rover-m3-back-full",	1,	NULL,	0   },
    { "rover-m3-back-zero",	1,	NULL,	0   },
    { "rover-m3-neutral",	1,	NULL,	0   },
    { "rover-m3-forward-zero",	1,	NULL,	0   },
    { "rover-m3-forward-full",	1,	NULL,	0   },
    { "rover-p1-i2c-bus",	1,	NULL,	0   }, // 25
    { "rover-p1-i2c-dev",	1,	NULL,	0   },
    { "rover-p1-i2c-cmd",	1,	NULL,	0   },
    { "rover-p1-back-full",	1,	NULL,	0   },
    { "rover-p1-back-zero",	1,	NULL,	0   },
    { "rover-p1-neutral",	1,	NULL,	0   },
    { "rover-p1-forward-zero",	1,	NULL,	0   },
    { "rover-p1-forward-full",	1,	NULL,	0   },
    { "rover-p2-i2c-bus",	1,	NULL,	0   }, // 33
    { "rover-p2-i2c-dev",	1,	NULL,	0   },
    { "rover-p2-i2c-cmd",	1,	NULL,	0   },
    { "rover-p2-back-full",	1,	NULL,	0   },
    { "rover-p2-back-zero",	1,	NULL,	0   },
    { "rover-p2-neutral",	1,	NULL,	0   },
    { "rover-p2-forward-zero",	1,	NULL,	0   },
    { "rover-p2-forward-full",	1,	NULL,	0   },
    { "rover-p3-i2c-bus",	1,	NULL,	0   }, // 41
    { "rover-p3-i2c-dev",	1,	NULL,	0   },
    { "rover-p3-i2c-cmd",	1,	NULL,	0   },
    { "rover-p3-back-full",	1,	NULL,	0   },
    { "rover-p3-back-zero",	1,	NULL,	0   },
    { "rover-p3-neutral",	1,	NULL,	0   },
    { "rover-p3-forward-zero",	1,	NULL,	0   },
    { "rover-p3-forward-full",	1,	NULL,	0   },
    { "rover-p4-i2c-bus",	1,	NULL,	0   }, // 49
    { "rover-p4-i2c-dev",	1,	NULL,	0   },
    { "rover-p4-i2c-cmd",	1,	NULL,	0   },
    { "rover-p4-back-full",	1,	NULL,	0   },
    { "rover-p4-back-zero",	1,	NULL,	0   },
    { "rover-p4-neutral",	1,	NULL,	0   },
    { "rover-p4-forward-zero",	1,	NULL,	0   },
    { "rover-p4-forward-full",	1,	NULL,	0   },
    { "rover-zero-x",		1,	NULL,	0   }, // 57
    { "rover-zero-y",		1,	NULL,	0   },
    { "rover-zero-mass",	1,	NULL,	0   },
    { "rc-port",		1,	NULL,	0   }, // 60
    { "rc-stdin",		1,	NULL,	0   },
    { "rc-event-input",		1,	NULL,	0   },
    { "rc-manual",		1,	NULL,	0   },
    { "target-hue",		1,	NULL,	0   }, // 64
    { "target-hue-tolerance",	1,	NULL,	0   },
    { "target-sat",		1,	NULL,	0   }, // 66
    { "target-sat-tolerance",	1,	NULL,	0   },
    { "target-val",		1,	NULL,	0   }, // 68
    { "target-val-tolerance",	1,	NULL,	0   },
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
            if      (!strcasecmp(optarg, "rgb888"))	s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB24;
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

          case 6: s_cfgFBOutput.m_path = optarg;					break;

          case 7   : s_cfgRoverOutput.m_motor1.m_path = optarg;				break;
          case 7 +1: s_cfgRoverOutput.m_motor1.m_powerBackFull = atoi(optarg);		break;
          case 7 +2: s_cfgRoverOutput.m_motor1.m_powerBackZero = atoi(optarg);		break;
          case 7 +3: s_cfgRoverOutput.m_motor1.m_powerNeutral = atoi(optarg);		break;
          case 7 +4: s_cfgRoverOutput.m_motor1.m_powerForwardZero = atoi(optarg);	break;
          case 7 +5: s_cfgRoverOutput.m_motor1.m_powerForwardFull = atoi(optarg);	break;

          case 13  : s_cfgRoverOutput.m_motor2.m_path = optarg;				break;
          case 13+1: s_cfgRoverOutput.m_motor2.m_powerBackFull = atoi(optarg);		break;
          case 13+2: s_cfgRoverOutput.m_motor2.m_powerBackZero = atoi(optarg);		break;
          case 13+3: s_cfgRoverOutput.m_motor2.m_powerNeutral = atoi(optarg);		break;
          case 13+4: s_cfgRoverOutput.m_motor2.m_powerForwardZero = atoi(optarg);	break;
          case 13+5: s_cfgRoverOutput.m_motor2.m_powerForwardFull = atoi(optarg);	break;

          case 19  : s_cfgRoverOutput.m_motor3.m_path = optarg;				break;
          case 19+1: s_cfgRoverOutput.m_motor3.m_powerBackFull = atoi(optarg);		break;
          case 19+2: s_cfgRoverOutput.m_motor3.m_powerBackZero = atoi(optarg);		break;
          case 19+3: s_cfgRoverOutput.m_motor3.m_powerNeutral = atoi(optarg);		break;
          case 19+4: s_cfgRoverOutput.m_motor3.m_powerForwardZero = atoi(optarg);	break;
          case 19+5: s_cfgRoverOutput.m_motor3.m_powerForwardFull = atoi(optarg);	break;

          case 25  : s_cfgRoverOutput.m_motorMsp1.m_mspI2CBusId    = atoi(optarg);	break;
          case 25+1: s_cfgRoverOutput.m_motorMsp1.m_mspI2CDeviceId = atoi(optarg);	break;
          case 25+2: s_cfgRoverOutput.m_motorMsp1.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 25+3: s_cfgRoverOutput.m_motorMsp1.m_powerBackFull  = atoi(optarg);	break;
          case 25+4: s_cfgRoverOutput.m_motorMsp1.m_powerBackZero  = atoi(optarg);	break;
          case 25+5: s_cfgRoverOutput.m_motorMsp1.m_powerNeutral   = atoi(optarg);	break;
          case 25+6: s_cfgRoverOutput.m_motorMsp1.m_powerForwardZero = atoi(optarg);	break;
          case 25+7: s_cfgRoverOutput.m_motorMsp1.m_powerForwardFull = atoi(optarg);	break;

          case 33  : s_cfgRoverOutput.m_motorMsp2.m_mspI2CBusId    = atoi(optarg);	break;
          case 33+1: s_cfgRoverOutput.m_motorMsp2.m_mspI2CDeviceId = atoi(optarg);	break;
          case 33+2: s_cfgRoverOutput.m_motorMsp2.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 33+3: s_cfgRoverOutput.m_motorMsp2.m_powerBackFull  = atoi(optarg);	break;
          case 33+4: s_cfgRoverOutput.m_motorMsp2.m_powerBackZero  = atoi(optarg);	break;
          case 33+5: s_cfgRoverOutput.m_motorMsp2.m_powerNeutral   = atoi(optarg);	break;
          case 33+6: s_cfgRoverOutput.m_motorMsp2.m_powerForwardZero = atoi(optarg);	break;
          case 33+7: s_cfgRoverOutput.m_motorMsp2.m_powerForwardFull = atoi(optarg);	break;

          case 41  : s_cfgRoverOutput.m_motorMsp3.m_mspI2CBusId    = atoi(optarg);	break;
          case 41+1: s_cfgRoverOutput.m_motorMsp3.m_mspI2CDeviceId = atoi(optarg);	break;
          case 41+2: s_cfgRoverOutput.m_motorMsp3.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 41+3: s_cfgRoverOutput.m_motorMsp3.m_powerBackFull  = atoi(optarg);	break;
          case 41+4: s_cfgRoverOutput.m_motorMsp3.m_powerBackZero  = atoi(optarg);	break;
          case 41+5: s_cfgRoverOutput.m_motorMsp3.m_powerNeutral   = atoi(optarg);	break;
          case 41+6: s_cfgRoverOutput.m_motorMsp3.m_powerForwardZero = atoi(optarg);	break;
          case 41+7: s_cfgRoverOutput.m_motorMsp3.m_powerForwardFull = atoi(optarg);	break;

          case 49  : s_cfgRoverOutput.m_motorMsp4.m_mspI2CBusId    = atoi(optarg);	break;
          case 49+1: s_cfgRoverOutput.m_motorMsp4.m_mspI2CDeviceId = atoi(optarg);	break;
          case 49+2: s_cfgRoverOutput.m_motorMsp4.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 49+3: s_cfgRoverOutput.m_motorMsp4.m_powerBackFull  = atoi(optarg);	break;
          case 49+4: s_cfgRoverOutput.m_motorMsp4.m_powerBackZero  = atoi(optarg);	break;
          case 49+5: s_cfgRoverOutput.m_motorMsp4.m_powerNeutral   = atoi(optarg);	break;
          case 49+6: s_cfgRoverOutput.m_motorMsp4.m_powerForwardZero = atoi(optarg);	break;
          case 49+7: s_cfgRoverOutput.m_motorMsp4.m_powerForwardFull = atoi(optarg);	break;

          case 57  : s_cfgRoverOutput.m_zeroX    = atoi(optarg);			break;
          case 57+1: s_cfgRoverOutput.m_zeroY    = atoi(optarg);			break;
          case 57+2: s_cfgRoverOutput.m_zeroMass = atoi(optarg);			break;

          case 60  : s_cfgRCInput.m_port = atoi(optarg);				break;
          case 60+1: s_cfgRCInput.m_stdin = atoi(optarg);				break;
          case 60+2: s_cfgRCInput.m_eventInput = optarg;				break;
          case 60+3: s_cfgRCInput.m_manualMode = atoi(optarg);				break;

          case 64  : s_cfgRCInput.m_autoTargetDetectHue = atof(optarg);			break;
          case 64+1: s_cfgRCInput.m_autoTargetDetectHueTolerance = atof(optarg);	break;
          case 66  : s_cfgRCInput.m_autoTargetDetectSat = atof(optarg);			break;
          case 66+1: s_cfgRCInput.m_autoTargetDetectSatTolerance = atof(optarg);	break;
          case 68  : s_cfgRCInput.m_autoTargetDetectVal = atof(optarg);			break;
          case 68+1: s_cfgRCInput.m_autoTargetDetectValTolerance = atof(optarg);	break;

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
                    "   --rover-mN-path         <rover-motorN-path>\n"
                    "   --rover-mN-back-full    <rover-motorN-full-back-power>\n"
                    "   --rover-mN-back-zero    <rover-motorN-slow-back-power>\n"
                    "   --rover-mN-neutral      <rover-motorN-neutral-power>\n"
                    "   --rover-mN-forward-zero <rover-motorN-slow-forward-power>\n"
                    "   --rover-mN-forward-full <rover-motorN-full-forward-power>\n"
                    "   --rover-pN-i2c-bus      <rover-powermotorN-i2c-bus-id>\n"
                    "   --rover-pN-i2c-dev      <rover-powermotorN-i2c-device-id>\n"
                    "   --rover-pN-i2c-cmd      <rover-powermotorN-i2c-command>\n"
                    "   --rover-pN-back-full    <rover-powermotorN-full-back-power>\n"
                    "   --rover-pN-back-zero    <rover-powermotorN-slow-back-power>\n"
                    "   --rover-pN-neutral      <rover-powermotorN-neutral-power>\n"
                    "   --rover-pN-forward-zero <rover-powermotorN-slow-forward-power>\n"
                    "   --rover-pN-forward-full <rover-powermotorN-full-forward-power>\n"
                    "   --rover-zero-x          <rover-center-X>\n"
                    "   --rover-zero-y          <rover-center-Y>\n"
                    "   --rover-zero-mass       <rover-center-mass>\n"
                    "   --rc-port               <remote-control-port>\n"
                    "   --rc-stdin              <remote-control-via-stdin 0/1>\n"
                    "   --rc-event-input        <remote-control-via-event-input-path>\n"
                    "   --rc-manual             <remote-control-manual-mode 0/1>\n"
                    "   --target-hue            <target-hue>\n"
                    "   --target-hue-tolerance  <target-hue-tolerance>\n"
                    "   --target-sat            <target-saturation>\n"
                    "   --target-sat-tolerance  <target-saturation-tolerance>\n"
                    "   --target-val            <target-value>\n"
                    "   --target-val-tolerance  <target-value-tolerance>\n"
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

  if ((res = rcInputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "rcInputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_fb_fini;
  }

  if ((res = roverOutputInit(s_cfgVerbose)) != 0)
  {
    fprintf(stderr, "roverOutputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit_rc_fini;
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

  RCInput rc;
  memset(&rc, 0, sizeof(rc));
  rc.m_stdinFd = -1;
  rc.m_eventInputFd = -1;
  rc.m_serverFd = -1;
  rc.m_connectionFd = -1;
  if ((res = rcInputOpen(&rc, &s_cfgRCInput)) != 0)
  {
    fprintf(stderr, "rcInputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_close;
  }

  RoverOutput rover;
  memset(&rover, 0, sizeof(rover));
  rover.m_motorMsp1.m_i2cBusFd = -1;
  rover.m_motorMsp2.m_i2cBusFd = -1;
  rover.m_motorMsp3.m_i2cBusFd = -1;
  rover.m_motorMsp4.m_i2cBusFd = -1;
  rover.m_motor1.m_fd = -1;
  rover.m_motor2.m_fd = -1;
  rover.m_motor3.m_fd = -1;
  if ((res = roverOutputOpen(&rover, &s_cfgRoverOutput)) != 0)
  {
    fprintf(stderr, "roverOutputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_close;
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

  if ((res = rcInputStart(&rc)) != 0)
  {
    fprintf(stderr, "rcInputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_fb_stop;
  }

  if ((res = roverOutputStart(&rover)) != 0)
  {
    fprintf(stderr, "roverOutputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_stop;
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


    if ((res = mainLoop(&codecEngine, &v4l2Src, &fbDst, &rc, &rover)) != 0)
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

exit_rc_stop:
  if ((res = rcInputStop(&rc)) != 0)
    fprintf(stderr, "rcInputStop() failed: %d\n", res);

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

exit_rc_close:
  if ((res = rcInputClose(&rc)) != 0)
    fprintf(stderr, "rcInputClose() failed: %d\n", res);

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

exit_rc_fini:
  if ((res = rcInputFini()) != 0)
    fprintf(stderr, "rcInputFini() failed: %d\n", res);

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




static int mainLoopV4L2Frame(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

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
  if ((res = rcInputGetAutoTargetDetect(_rc, &detectHueFrom, &detectHueTo, &detectSatFrom, &detectSatTo, &detectValFrom, &detectValTo)) != 0)
  {
    fprintf(stderr, "rcInputGetAutoTargetDetect() failed: %d\n", res);
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


  if (!rcInputIsManualMode(_rc))
  {
    if ((res = roverOutputControlAuto(_rover, targetX, targetY, targetMass)) != 0)
    {
      fprintf(stderr, "roverOutputControlAuto() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}

static int mainLoopRCManualModeUpdate(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if (rcInputIsManualMode(_rc))
  {
    int ctrlChasisLR;
    int ctrlChasisFB;
    int ctrlHand;
    int ctrlArm;

    if ((res = rcInputGetManualCommand(_rc, &ctrlChasisLR, &ctrlChasisFB, &ctrlHand, &ctrlArm)) != 0)
    {
      fprintf(stderr, "rcInputGetManualCommand() failed: %d\n", res);
      return res;
    }
    if ((res = roverOutputControlManual(_rover, ctrlChasisLR, ctrlChasisFB, ctrlHand, ctrlArm)) != 0)
    {
      fprintf(stderr, "roverOutputControlManual() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}

static int mainLoopRCStdin(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputReadStdin(_rc)) != 0)
  {
    fprintf(stderr, "rcInputReadStdin() failed: %d\n", res);
    return res;
  }

  return mainLoopRCManualModeUpdate(_ce, _v4l2Src, _fbDst, _rc, _rover);
}

static int mainLoopRCEventInput(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputReadEventInput(_rc)) != 0)
  {
    fprintf(stderr, "rcInputReadEventInput() failed: %d\n", res);
    return res;
  }

  return mainLoopRCManualModeUpdate(_ce, _v4l2Src, _fbDst, _rc, _rover);
}

static int mainLoopRCServer(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputAcceptConnection(_rc)) != 0)
  {
    fprintf(stderr, "rcInputAcceptConnection() failed: %d\n", res);
    return res;
  }

  return 0;
}

static int mainLoopRCConnection(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  if ((res = rcInputReadConnection(_rc)) != 0)
  {
    fprintf(stderr, "rcInputReadConnection() failed: %d\n", res);
    return res;
  }

  return mainLoopRCManualModeUpdate(_ce, _v4l2Src, _fbDst, _rc, _rover);
}


static int mainLoop(CodecEngine* _ce, V4L2Input* _v4l2Src, FBOutput* _fbDst, RCInput* _rc, RoverOutput* _rover)
{
  int res;

  int maxFd = 0;
  fd_set fdsIn;
  FD_ZERO(&fdsIn);

  FD_SET(_v4l2Src->m_fd, &fdsIn);
  if (maxFd < _v4l2Src->m_fd)
    maxFd = _v4l2Src->m_fd;

  FD_SET(_rc->m_serverFd, &fdsIn);
  if (maxFd < _rc->m_serverFd)
    maxFd = _rc->m_serverFd;

  if (_rc->m_stdinFd != -1)
  {
    FD_SET(_rc->m_stdinFd, &fdsIn);
    if (maxFd < _rc->m_stdinFd)
      maxFd = _rc->m_stdinFd;
  }

  if (_rc->m_eventInputFd != -1)
  {
    FD_SET(_rc->m_eventInputFd, &fdsIn);
    if (maxFd < _rc->m_eventInputFd)
      maxFd = _rc->m_eventInputFd;
  }

  if (_rc->m_connectionFd != -1)
  {
    FD_SET(_rc->m_connectionFd, &fdsIn);
    if (maxFd < _rc->m_connectionFd)
      maxFd = _rc->m_connectionFd;
  }

  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if ((res = select(maxFd+1, &fdsIn, NULL, NULL, &timeout)) < 0)
  {
    res = errno;
    fprintf(stderr, "select() failed: %d\n", res);
    return res;
  }

  bool hasAnything = false;;
  if (FD_ISSET(_rc->m_serverFd, &fdsIn))
  {
    if ((res = mainLoopRCServer(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCServer() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  if (_rc->m_stdinFd != -1 && FD_ISSET(_rc->m_stdinFd, &fdsIn))
  {
    if ((res = mainLoopRCStdin(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCStdin() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  if (_rc->m_eventInputFd != -1 && FD_ISSET(_rc->m_eventInputFd, &fdsIn))
  {
    if ((res = mainLoopRCEventInput(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCEventInput() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  if (_rc->m_connectionFd != -1 && FD_ISSET(_rc->m_connectionFd, &fdsIn))
  {
    if ((res = mainLoopRCConnection(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopRCConnection() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  // don't go for video frame if did any other action
  if (!hasAnything && FD_ISSET(_v4l2Src->m_fd, &fdsIn))
  {
    if ((res = mainLoopV4L2Frame(_ce, _v4l2Src, _fbDst, _rc, _rover)) != 0)
    {
      fprintf(stderr, "mainLoopV4L2Frame() failed: %d\n", res);
      return res;
    }
    hasAnything = true;
  }

  return 0;
}


