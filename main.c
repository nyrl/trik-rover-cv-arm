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
static FBConfig s_cfgFBOutput = { "/dev/fb" };




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
            if (!strcasecmp(optarg, "rgb888")) s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB24;
            else if (!strcasecmp(optarg, "rgb565")) s_cfgV4L2Input.m_format = V4L2_PIX_FMT_RGB565;
            else if (!strcasecmp(optarg, "yuv422")) s_cfgV4L2Input.m_format = V4L2_PIX_FMT_YUYV;
            else
            {
              fprintf(stderr, "Unknown v4l2 format '%s'\n"
                              "Known formats: rgb888, rgb565, yuv422\n",
                      optarg);
              return false;
            }
            break;

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

  V4L2Input v4l2src;
  memset(&v4l2src, 0, sizeof(v4l2src));
  v4l2src.m_fd = -1;
  if ((res = v4l2InputOpen(&v4l2src, &s_cfgV4L2Input)) != 0)
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


#warning TODO FB buffer size
  if ((res = codecEngineStart(&codecEngine, &s_cfgCodecEngine, v4l2src.m_imageFormat.fmt.pix.sizeimage, 0)) != 0)
  {
    fprintf(stderr, "codecEngineStart() failed: %d\n", res);
    exit_code = EX_PROTOCOL;
    goto exit_fb_close;
  }

  if ((res = v4l2InputStart(&v4l2src)) != 0)
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
    fd_set fdsIn;
    FD_ZERO(&fdsIn);
    FD_SET(v4l2src.m_fd, &fdsIn);
    int maxFd = v4l2src.m_fd+1;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if ((res = select(maxFd, &fdsIn, NULL, NULL, &timeout)) < 0)
    {
      fprintf(stderr, "select() failed: %d\n", errno);
      exit_code = EX_OSERR;
      goto exit_mainloop;
    }

    if (FD_ISSET(v4l2src.m_fd, &fdsIn))
    {
      const void* frameSrcPtr;
      size_t frameSrcSize;
      size_t frameSrcIndex;
      if ((res = v4l2InputGetFrame(&v4l2src, &frameSrcPtr, &frameSrcSize, &frameSrcIndex)) != 0)
      {
        fprintf(stderr, "v4l2InputGetFrame() failed: %d\n", res);
        exit_code = res;
        goto exit_mainloop;
      }

#if 1
      printf("Frame %p, %zu [%zu]\n", frameSrcPtr, frameSrcSize, frameSrcIndex);
#endif

#warning transcode

#if 0
  /* use engine to encode, then decode the data */
  transcode(transcoder, in, out);
#endif



      if ((res = v4l2InputPutFrame(&v4l2src, frameSrcIndex)) != 0)
      {
        fprintf(stderr, "v4l2InputPutFrame() failed: %d\n", res);
        exit_code = res;
        goto exit_mainloop;
      }
    }
  }
  printf("Left main loop\n");





exit_mainloop:

exit_fb_stop:
  if ((res = fbOutputStop(&fbDst)) != 0)
    fprintf(stderr, "fbOutputStop() failed: %d\n", res);

exit_v4l2_stop:
  if ((res = v4l2InputStop(&v4l2src)) != 0)
    fprintf(stderr, "v4l2InputStop() failed: %d\n", res);

exit_ce_stop:
  if ((res = codecEngineStop(&codecEngine)) != 0)
    fprintf(stderr, "codecEngineStop() failed: %d\n", res);


exit_fb_close:
  if ((res = fbOutputClose(&fbDst)) != 0)
    fprintf(stderr, "fbOutputClose() failed: %d\n", res);

exit_v4l2_close:
  if ((res = v4l2InputClose(&v4l2src)) != 0)
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
    fprintf(stderr, "ceFini() failed: %d\n", res);


exit:
  return exit_code;
}


