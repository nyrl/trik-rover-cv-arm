
#include <xdc/std.h>
#include <xdc/runtime/Diags.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/vidtranscode/vidtranscode.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include <libv4l2.h>

#include "internal/v4l2.h"

#warning Check BUFALIGN usage!
#ifndef BUFALIGN
#define BUFALIGN 128
#endif

#define ALIGN_UP(v, a) ((((v)+(a)-1)/(a))*(a))


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
static char* s_cfgCeServerName = "dsp_server.xe674";
static char* s_cfgCeCodecName = "vidtranscode_resample";
static V4L2Config s_cfgV4L2Input = { "/dev/video0", 640, 480, V4L2_PIX_FMT_RGB24 };




static bool parse_args(int _argc, char* const _argv[])
{
  int opt;
  int longopt;

  static const char* optstring = "s:c:vh";
  static const struct option longopts[] =
  {
    { "server",			1,	NULL,	's' },
    { "codec",			1,	NULL,	'c' },
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
      case 's':	s_cfgCeServerName = optarg;	break;
      case 'c':	s_cfgCeCodecName = optarg;	break;
      case 'v':	s_cfgVerbose = true;		break;

      case 0:
        switch (longopt)
        {
          case 2: s_cfgV4L2Input.m_path = optarg; break;
          case 3: s_cfgV4L2Input.m_width = atoi(optarg); break;
          case 4: s_cfgV4L2Input.m_height = atoi(optarg); break;
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

  CERuntime_init(); /* init Codec Engine */

  if (s_cfgVerbose)
  {
    Diags_setMask("xdc.runtime.Main+EX1234567");
    Diags_setMask(Engine_MODNAME"+EX1234567");
    v4l2_log_file = stderr;
  }

  Engine_Error ceError;
  Engine_Desc ceDesc;
  Engine_initDesc(&ceDesc);
  ceDesc.name = "dsp-server";
  ceDesc.remoteName = s_cfgCeServerName;
  ceError = Engine_add(&ceDesc);
  if (ceError != Engine_EOK)
  {
    fprintf(stderr, "Engine_add(%s) failed: %d\n", s_cfgCeServerName, errno);
    exit_code = EX_PROTOCOL;
    goto exit;
  }

  Engine_Handle ce = Engine_open("dsp-server", NULL, &ceError);
  if (ce == NULL)
  {
    fprintf(stderr, "Engine_open(dsp-server@%s) failed: %d\n", s_cfgCeServerName, errno);
    exit_code = EX_PROTOCOL;
    goto exit;
  }

  V4L2Input v4l2src;
  memset(&v4l2src, 0, sizeof(v4l2src));
  v4l2src.m_fd = -1;
  if ((res = v4l2inputOpen(&v4l2src, &s_cfgV4L2Input)) != 0)
  {
    fprintf(stderr, "v4l2inputOpen() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_engine_close;
  }

#warning TODO open dst?

  Memory_AllocParams ceAllocParams;
  ceAllocParams.type = Memory_CONTIGPOOL;
  ceAllocParams.flags = Memory_NONCACHED;
  ceAllocParams.align = BUFALIGN;
  ceAllocParams.seg = 0;

  const XDAS_Int32 ceSrcBufferSize = v4l2src.m_imageFormat.fmt.pix.sizeimage;
  XDAS_Int8* ceSrcBuffer = (XDAS_Int8*)Memory_alloc(ALIGN_UP(ceSrcBufferSize, BUFALIGN), &ceAllocParams);

#warning TODO dst size unknown
  const XDAS_Int32 ceDstBufferSize = v4l2src.m_imageFormat.fmt.pix.sizeimage;
  XDAS_Int8* ceDstBuffer = (XDAS_Int8*)Memory_alloc(ALIGN_UP(ceDstBufferSize, BUFALIGN), &ceAllocParams);

  if (ceSrcBuffer == NULL || ceDstBuffer == NULL)
  {
    fprintf(stderr, "Memory_alloc() failed for src/dst buffer\n");
    exit_code = EX_PROTOCOL;
    goto exit_v4l2close;
  }

  VIDTRANSCODE_Handle transcoder;
  transcoder = VIDTRANSCODE_create(ce, s_cfgCeCodecName, NULL);
  if (transcoder == NULL)
  {
    fprintf(stderr, "VIDTRANSCODE_create(%s) failed: %d\n", s_cfgCeCodecName, errno);
    exit_code = EX_PROTOCOL;
    goto exit_memory_free;
  }


  if ((res = v4l2inputStart(&v4l2src)) != 0)
  {
    fprintf(stderr, "v4l2inputStart() failed: %d\n", res);
    exit_code = EX_NOINPUT;
    goto exit_vidtranscode_delete;
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

    if ((res = select(maxFd, &fdsIn, NULL, NULL, &timeout)) == -1)
    {
      fprintf(stderr, "select() failed: %d\n", errno);
      exit_code = EX_OSERR;
      goto exit_v4l2stop;
    }

    if (FD_ISSET(v4l2src.m_fd, &fdsIn))
    {
      printf("Frame!\n");

#warning TODO capture frame
#if 0
int v4l2inputGetFrame(V4L2Input* _v4l2, void** _framePtr, size_t* _frameSize, size_t* _frameIndex);
int v4l2inputPutFrame(V4L2Input* _v4l2, size_t _frameIndex);
typedef struct V4L2Input
{
  int                    m_fd;
  struct v4l2_format     m_imageFormat;
} V4L2Input;
#endif

#if 0
  /* use engine to encode, then decode the data */
  transcode(transcoder, in, out);
#endif



      if ((res = v4l2inputPutFrame(&v4l2src, frameSrcIndex)) != 0)
      {
        fprintf(stderr, "v4l2inputPutFrame() failed: %d\n", res);
        exit_code = res;
        goto exit_mainloop;
      }
    }
  }
  printf("Left main loop\n");


exit_mainloop:
exit_v4l2stop:
  if ((res = v4l2inputStop(&v4l2src)) != 0)
    fprintf(stderr, "v4l2inputStop() failed: %d\n", res);

exit_vidtranscode_delete:
  VIDTRANSCODE_delete(transcoder);

exit_memory_free:
  Memory_free(ceSrcBuffer, ceSrcBufferSize, &ceAllocParams);
  Memory_free(ceDstBuffer, ceDstBufferSize, &ceAllocParams);

exit_v4l2close:
  if ((res = v4l2inputClose(&v4l2src)) != 0)
    fprintf(stderr, "v4l2inputClose() failed: %d\n", res);

exit_engine_close:
  Engine_close(ce);

exit:
  return exit_code;
}


