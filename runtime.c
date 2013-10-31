#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>

#include "internal/runtime.h"
#include "internal/thread_input.h"
#include "internal/thread_video.h"
#include "internal/thread_rover.h"




static const RuntimeConfig s_runtimeConfig = {
  .m_verbose = false,
  .m_codecEngineConfig = { "dsp_server.xe674", "vidtranscode_cv" },
  .m_v4l2Config        = { "/dev/video0", 352, 288, V4L2_PIX_FMT_YUYV },
  .m_fbConfig          = { "/dev/fb0" },
  .m_rcConfig          = { 4444, false, "/dev/input/by-path/platform-gpio-keys-event", false,
                           7, 13, 85, 15, 80, 20 },
  .m_roverConfig       = { { 2, 0x48, 0x14, -100, -1, 0, 1, 100 }, //chasis left1
                           { 2, 0x48, 0x15, -100, -1, 0, 1, 100 }, //chasis left2
                           { 2, 0x48, 0x16, -100, -1, 0, 1, 100 }, //chasis right1
                           { 2, 0x48, 0x17, -100, -1, 0, 1, 100 }, //chasis right2
                           { "/sys/class/pwm/ecap.0/duty_ns",     2300000, 1600000, 0, 1400000, 700000  },   //hand up-down1
                           { "/sys/class/pwm/ecap.1/duty_ns",     700000,  1400000, 0, 1600000, 2300000 },   //hand up-down2
                           { "/sys/class/pwm/ehrpwm.1:1/duty_ns", 700000,  1400000, 0, 1600000, 2300000 } }, //arm squeeze
  .m_driverConfig      = { 0, 50, 600 }
};




void runtimeReset(Runtime* _runtime)
{
  memset(_runtime, 0, sizeof(*_runtime));

  _runtime->m_config = s_runtimeConfig;

  memset(&_runtime->m_modules.m_codecEngine,  0, sizeof(_runtime->m_modules.m_codecEngine));
  memset(&_runtime->m_modules.m_v4l2Input,    0, sizeof(_runtime->m_modules.m_v4l2Input));
  _runtime->m_modules.m_v4l2Input.m_fd = -1;
  memset(&_runtime->m_modules.m_fbOutput,     0, sizeof(_runtime->m_modules.m_fbOutput));
  _runtime->m_modules.m_fbOutput.m_fd = -1;
  memset(&_runtime->m_modules.m_rcInput,      0, sizeof(_runtime->m_modules.m_rcInput));
  _runtime->m_modules.m_rcInput.m_stdinFd = -1;
  _runtime->m_modules.m_rcInput.m_eventInputFd = -1;
  _runtime->m_modules.m_rcInput.m_serverFd = -1;
  _runtime->m_modules.m_rcInput.m_connectionFd = -1;
  memset(&_runtime->m_modules.m_roverOutput,  0, sizeof(_runtime->m_modules.m_roverOutput));
  _runtime->m_modules.m_roverOutput.m_motorChasisLeft1.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisLeft2.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisRight1.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorChasisRight2.m_i2cBusFd = -1;
  _runtime->m_modules.m_roverOutput.m_motorHand1.m_fd = -1;
  _runtime->m_modules.m_roverOutput.m_motorHand2.m_fd = -1;
  _runtime->m_modules.m_roverOutput.m_motorArm.m_fd = -1;
  memset(&_runtime->m_modules.m_driverOutput, 0, sizeof(_runtime->m_modules.m_driverOutput));

  memset(&_runtime->m_threads, 0, sizeof(_runtime->m_threads));
  _runtime->m_threads.m_terminate = true;

  pthread_mutex_init(&_runtime->m_state.m_mutex, NULL);
  memset(&_runtime->m_state.m_targetParams,        0, sizeof(_runtime->m_state.m_targetParams));
  memset(&_runtime->m_state.m_targetLocation,      0, sizeof(_runtime->m_state.m_targetLocation));
  memset(&_runtime->m_state.m_driverManualControl, 0, sizeof(_runtime->m_state.m_driverManualControl));
}




bool runtimeParseArgs(Runtime* _runtime, int _argc, char* const _argv[])
{
  int opt;
  int longopt;
  RuntimeConfig* cfg;

  static const char* s_optstring = "vh";
  static const struct option s_longopts[] =
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
    { "target-zero-x",		1,	NULL,	0   }, // 57
    { "target-zero-y",		1,	NULL,	0   },
    { "target-zero-mass",	1,	NULL,	0   },
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

  if (_runtime == NULL)
    return false;

  cfg = &_runtime->m_config;


  while ((opt = getopt_long(_argc, _argv, s_optstring, s_longopts, &longopt)) != -1)
  {
    switch (opt)
    {
      case 'v':	cfg->m_verbose = true;					break;

      case 0:
        switch (longopt)
        {
          case 0: cfg->m_codecEngineConfig.m_serverPath = optarg;	break;
          case 1: cfg->m_codecEngineConfig.m_codecName = optarg;	break;

          case 2: cfg->m_v4l2Config.m_path = optarg;			break;
          case 3: cfg->m_v4l2Config.m_width = atoi(optarg);		break;
          case 4: cfg->m_v4l2Config.m_height = atoi(optarg);		break;
          case 5:
            if      (!strcasecmp(optarg, "rgb888"))	cfg->m_v4l2Config.m_format = V4L2_PIX_FMT_RGB24;
            else if (!strcasecmp(optarg, "rgb565"))	cfg->m_v4l2Config.m_format = V4L2_PIX_FMT_RGB565;
            else if (!strcasecmp(optarg, "rgb565x"))	cfg->m_v4l2Config.m_format = V4L2_PIX_FMT_RGB565X;
            else if (!strcasecmp(optarg, "yuv444"))	cfg->m_v4l2Config.m_format = V4L2_PIX_FMT_YUV32;
            else if (!strcasecmp(optarg, "yuv422"))	cfg->m_v4l2Config.m_format = V4L2_PIX_FMT_YUYV;
            else
            {
              fprintf(stderr, "Unknown v4l2 format '%s'\n"
                              "Known formats: rgb888, rgb565, rgb565x, yuv444, yuv422\n",
                      optarg);
              return false;
            }
            break;

          case 6: cfg->m_fbConfig.m_path = optarg;						break;

          case 7   : cfg->m_roverConfig.m_motorHand1.m_path = optarg;				break;
          case 7 +1: cfg->m_roverConfig.m_motorHand1.m_powerBackFull = atoi(optarg);		break;
          case 7 +2: cfg->m_roverConfig.m_motorHand1.m_powerBackZero = atoi(optarg);		break;
          case 7 +3: cfg->m_roverConfig.m_motorHand1.m_powerNeutral = atoi(optarg);		break;
          case 7 +4: cfg->m_roverConfig.m_motorHand1.m_powerForwardZero = atoi(optarg);		break;
          case 7 +5: cfg->m_roverConfig.m_motorHand1.m_powerForwardFull = atoi(optarg);		break;

          case 13  : cfg->m_roverConfig.m_motorHand2.m_path = optarg;				break;
          case 13+1: cfg->m_roverConfig.m_motorHand2.m_powerBackFull = atoi(optarg);		break;
          case 13+2: cfg->m_roverConfig.m_motorHand2.m_powerBackZero = atoi(optarg);		break;
          case 13+3: cfg->m_roverConfig.m_motorHand2.m_powerNeutral = atoi(optarg);		break;
          case 13+4: cfg->m_roverConfig.m_motorHand2.m_powerForwardZero = atoi(optarg);		break;
          case 13+5: cfg->m_roverConfig.m_motorHand2.m_powerForwardFull = atoi(optarg);		break;

          case 19  : cfg->m_roverConfig.m_motorArm.m_path = optarg;				break;
          case 19+1: cfg->m_roverConfig.m_motorArm.m_powerBackFull = atoi(optarg);		break;
          case 19+2: cfg->m_roverConfig.m_motorArm.m_powerBackZero = atoi(optarg);		break;
          case 19+3: cfg->m_roverConfig.m_motorArm.m_powerNeutral = atoi(optarg);		break;
          case 19+4: cfg->m_roverConfig.m_motorArm.m_powerForwardZero = atoi(optarg);		break;
          case 19+5: cfg->m_roverConfig.m_motorArm.m_powerForwardFull = atoi(optarg);		break;

          case 25  : cfg->m_roverConfig.m_motorChasisLeft1.m_mspI2CBusId    = atoi(optarg);	break;
          case 25+1: cfg->m_roverConfig.m_motorChasisLeft1.m_mspI2CDeviceId = atoi(optarg);	break;
          case 25+2: cfg->m_roverConfig.m_motorChasisLeft1.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 25+3: cfg->m_roverConfig.m_motorChasisLeft1.m_powerBackFull  = atoi(optarg);	break;
          case 25+4: cfg->m_roverConfig.m_motorChasisLeft1.m_powerBackZero  = atoi(optarg);	break;
          case 25+5: cfg->m_roverConfig.m_motorChasisLeft1.m_powerNeutral   = atoi(optarg);	break;
          case 25+6: cfg->m_roverConfig.m_motorChasisLeft1.m_powerForwardZero = atoi(optarg);	break;
          case 25+7: cfg->m_roverConfig.m_motorChasisLeft1.m_powerForwardFull = atoi(optarg);	break;

          case 33  : cfg->m_roverConfig.m_motorChasisLeft2.m_mspI2CBusId    = atoi(optarg);	break;
          case 33+1: cfg->m_roverConfig.m_motorChasisLeft2.m_mspI2CDeviceId = atoi(optarg);	break;
          case 33+2: cfg->m_roverConfig.m_motorChasisLeft2.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 33+3: cfg->m_roverConfig.m_motorChasisLeft2.m_powerBackFull  = atoi(optarg);	break;
          case 33+4: cfg->m_roverConfig.m_motorChasisLeft2.m_powerBackZero  = atoi(optarg);	break;
          case 33+5: cfg->m_roverConfig.m_motorChasisLeft2.m_powerNeutral   = atoi(optarg);	break;
          case 33+6: cfg->m_roverConfig.m_motorChasisLeft2.m_powerForwardZero = atoi(optarg);	break;
          case 33+7: cfg->m_roverConfig.m_motorChasisLeft2.m_powerForwardFull = atoi(optarg);	break;

          case 41  : cfg->m_roverConfig.m_motorChasisRight1.m_mspI2CBusId    = atoi(optarg);	break;
          case 41+1: cfg->m_roverConfig.m_motorChasisRight1.m_mspI2CDeviceId = atoi(optarg);	break;
          case 41+2: cfg->m_roverConfig.m_motorChasisRight1.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 41+3: cfg->m_roverConfig.m_motorChasisRight1.m_powerBackFull  = atoi(optarg);	break;
          case 41+4: cfg->m_roverConfig.m_motorChasisRight1.m_powerBackZero  = atoi(optarg);	break;
          case 41+5: cfg->m_roverConfig.m_motorChasisRight1.m_powerNeutral   = atoi(optarg);	break;
          case 41+6: cfg->m_roverConfig.m_motorChasisRight1.m_powerForwardZero = atoi(optarg);	break;
          case 41+7: cfg->m_roverConfig.m_motorChasisRight1.m_powerForwardFull = atoi(optarg);	break;

          case 49  : cfg->m_roverConfig.m_motorChasisRight2.m_mspI2CBusId    = atoi(optarg);	break;
          case 49+1: cfg->m_roverConfig.m_motorChasisRight2.m_mspI2CDeviceId = atoi(optarg);	break;
          case 49+2: cfg->m_roverConfig.m_motorChasisRight2.m_mspI2CMotorCmd = atoi(optarg);	break;
          case 49+3: cfg->m_roverConfig.m_motorChasisRight2.m_powerBackFull  = atoi(optarg);	break;
          case 49+4: cfg->m_roverConfig.m_motorChasisRight2.m_powerBackZero  = atoi(optarg);	break;
          case 49+5: cfg->m_roverConfig.m_motorChasisRight2.m_powerNeutral   = atoi(optarg);	break;
          case 49+6: cfg->m_roverConfig.m_motorChasisRight2.m_powerForwardZero = atoi(optarg);	break;
          case 49+7: cfg->m_roverConfig.m_motorChasisRight2.m_powerForwardFull = atoi(optarg);	break;

          case 57  : cfg->m_driverConfig.m_zeroX    = atoi(optarg);				break;
          case 57+1: cfg->m_driverConfig.m_zeroY    = atoi(optarg);				break;
          case 57+2: cfg->m_driverConfig.m_zeroMass = atoi(optarg);				break;

          case 60  : cfg->m_rcConfig.m_port = atoi(optarg);					break;
          case 60+1: cfg->m_rcConfig.m_stdin = atoi(optarg);					break;
          case 60+2: cfg->m_rcConfig.m_eventInput = optarg;					break;
          case 60+3: cfg->m_rcConfig.m_manualMode = atoi(optarg);				break;

          case 64  : cfg->m_rcConfig.m_targetDetectHue = atoi(optarg);				break;
          case 64+1: cfg->m_rcConfig.m_targetDetectHueTolerance = atoi(optarg);			break;
          case 66  : cfg->m_rcConfig.m_targetDetectSat = atoi(optarg);				break;
          case 66+1: cfg->m_rcConfig.m_targetDetectSatTolerance = atoi(optarg);			break;
          case 68  : cfg->m_rcConfig.m_targetDetectVal = atoi(optarg);				break;
          case 68+1: cfg->m_rcConfig.m_targetDetectValTolerance = atoi(optarg);			break;

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




void runtimeArgsHelpMessage(Runtime* _runtime, int _argc, char* const _argv[])
{
  if (_runtime == NULL)
    return;

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
                  "   --target-zero-x         <target-center-X>\n"
                  "   --target-zero-y         <target-center-Y>\n"
                  "   --target-zero-mass      <target-center-mass>\n"
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
}




int runtimeInit(Runtime* _runtime)
{
  int res = 0;
  int exit_code = 0;
  bool verbose;

  if (_runtime == NULL)
    return EINVAL;

  verbose = runtimeCfgVerbose(_runtime);

  if ((res = codecEngineInit(verbose)) != 0)
  {
    fprintf(stderr, "codecEngineInit() failed: %d\n", res);
    exit_code = res;
  }

  if ((res = v4l2InputInit(verbose)) != 0)
  {
    fprintf(stderr, "v4l2InputInit() failed: %d\n", res);
    exit_code = res;
  }

  if ((res = fbOutputInit(verbose)) != 0)
  {
    fprintf(stderr, "fbOutputInit() failed: %d\n", res);
    exit_code = res;
  }

  if ((res = rcInputInit(verbose)) != 0)
  {
    fprintf(stderr, "rcInputInit() failed: %d\n", res);
    exit_code = res;
  }

  if ((res = roverOutputInit(verbose)) != 0)
  {
    fprintf(stderr, "roverOutputInit() failed: %d\n", res);
    exit_code = res;
  }

  if ((res = driverOutputInit(verbose)) != 0)
  {
    fprintf(stderr, "driverOutputInit() failed: %d\n", res);
    exit_code = res;
  }

  return exit_code;
}




int runtimeFini(Runtime* _runtime)
{
  int res;

  if (_runtime == NULL)
    return EINVAL;

  if ((res = driverOutputFini()) != 0)
    fprintf(stderr, "driverOutputFini() failed: %d\n", res);

  if ((res = roverOutputFini()) != 0)
    fprintf(stderr, "roverOutputFini() failed: %d\n", res);

  if ((res = rcInputFini()) != 0)
    fprintf(stderr, "rcInputFini() failed: %d\n", res);

  if ((res = fbOutputFini()) != 0)
    fprintf(stderr, "fbOutputFini() failed: %d\n", res);

  if ((res = v4l2InputFini()) != 0)
    fprintf(stderr, "v4l2InputFini() failed: %d\n", res);

  if ((res = codecEngineFini()) != 0)
    fprintf(stderr, "codecEngineFini() failed: %d\n", res);

  return 0;
}




int runtimeStart(Runtime* _runtime)
{
  int res;
  int exit_code = 0;
  RuntimeThreads* rt;

  if (_runtime == NULL)
    return EINVAL;

  rt = &_runtime->m_threads;
  rt->m_terminate = false;

  if ((res = pthread_create(&rt->m_inputThread, NULL, &threadInput, _runtime)) != 0)
  {
    fprintf(stderr, "pthread_create(input) failed: %d\n", res);
    exit_code = res;
    goto exit;
  }

  if ((res = pthread_create(&rt->m_videoThread, NULL, &threadVideo, _runtime)) != 0)
  {
    fprintf(stderr, "pthread_create(video) failed: %d\n", res);
    exit_code = res;
    goto exit_join_input_thread;
  }

  if ((res = pthread_create(&rt->m_roverThread, NULL, &threadRover, _runtime)) != 0)
  {
    fprintf(stderr, "pthread_create(rover) failed: %d\n", res);
    exit_code = res;
    goto exit_join_video_thread;
  }

  return 0;


 //exit_join_rover_thread:
  runtimeSetTerminate(_runtime);
  pthread_cancel(rt->m_roverThread);
  pthread_join(rt->m_roverThread, NULL);

 exit_join_video_thread:
  runtimeSetTerminate(_runtime);
  pthread_cancel(rt->m_videoThread);
  pthread_join(rt->m_videoThread, NULL);

 exit_join_input_thread:
  runtimeSetTerminate(_runtime);
  pthread_cancel(rt->m_inputThread);
  pthread_join(rt->m_inputThread, NULL);

 exit:
  runtimeSetTerminate(_runtime);
  return exit_code;
}




int runtimeStop(Runtime* _runtime)
{
  RuntimeThreads* rt;

  if (_runtime == NULL)
    return EINVAL;

  rt = &_runtime->m_threads;

  runtimeSetTerminate(_runtime);
  pthread_join(rt->m_roverThread, NULL);
  pthread_join(rt->m_videoThread, NULL);
  pthread_join(rt->m_inputThread, NULL);

  return 0;
}




bool runtimeCfgVerbose(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return false;

  return _runtime->m_config.m_verbose;
}

const CodecEngineConfig* runtimeCfgCodecEngine(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_codecEngineConfig;
}

const V4L2Config* runtimeCfgV4L2Input(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_v4l2Config;
}

const FBConfig* runtimeCfgFBOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_fbConfig;
}

const RCConfig* runtimeCfgRCInput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_rcConfig;
}

const RoverConfig* runtimeCfgRoverOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_roverConfig;
}

const DriverConfig* runtimeCfgDriverOutput(const Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_config.m_driverConfig;
}




CodecEngine* runtimeModCodecEngine(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_codecEngine;
}

V4L2Input* runtimeModV4L2Input(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_v4l2Input;
}

FBOutput* runtimeModFBOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_fbOutput;
}

RCInput* runtimeModRCInput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_rcInput;
}

RoverOutput* runtimeModRoverOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_roverOutput;
}

DriverOutput* runtimeModDriverOutput(Runtime* _runtime)
{
  if (_runtime == NULL)
    return NULL;

  return &_runtime->m_modules.m_driverOutput;
}




bool runtimeGetTerminate(Runtime* _runtime)
{
  if (_runtime == NULL)
    return true;

  return _runtime->m_threads.m_terminate;
}

void runtimeSetTerminate(Runtime* _runtime)
{
  if (_runtime == NULL)
    return;

  _runtime->m_threads.m_terminate = true;
}

int runtimeGetTargetDetectParams(Runtime* _runtime, TargetDetectParams* _targetParams)
{
  if (_runtime == NULL || _targetParams == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  *_targetParams = _runtime->m_state.m_targetParams;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}

int runtimeSetTargetDetectParams(Runtime* _runtime, const TargetDetectParams* _targetParams)
{
  if (_runtime == NULL || _targetParams == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  _runtime->m_state.m_targetParams = *_targetParams;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}

int runtimeGetTargetLocation(Runtime* _runtime, TargetLocation* _targetLocation)
{
  if (_runtime == NULL || _targetLocation == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  *_targetLocation = _runtime->m_state.m_targetLocation;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}

int runtimeSetTargetLocation(Runtime* _runtime, const TargetLocation* _targetLocation)
{
  if (_runtime == NULL || _targetLocation == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  _runtime->m_state.m_targetLocation = *_targetLocation;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}

int runtimeGetDriverManualControl(Runtime* _runtime, DriverManualControl* _manualControl)
{
  if (_runtime == NULL || _manualControl == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  *_manualControl = _runtime->m_state.m_driverManualControl;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}

int runtimeSetDriverManualControl(Runtime* _runtime, const DriverManualControl* _manualControl)
{
  if (_runtime == NULL || _manualControl == NULL)
    return EINVAL;

  pthread_mutex_lock(&_runtime->m_state.m_mutex);
  _runtime->m_state.m_driverManualControl = *_manualControl;
  pthread_mutex_unlock(&_runtime->m_state.m_mutex);
  return 0;
}


