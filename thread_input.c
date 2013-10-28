#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <sys/select.h>

#include "internal/thread_input.h"
#include "internal/runtime.h"
#include "internal/module_rc.h"




static int input_loop(Runtime* _runtime, RCInput* _rc)
{
  int res;
  int maxFd = 0;
  fd_set fdsIn;
  static const struct timespec s_select_timeout = { .tv_sec=1, .tv_nsec=0 };

  assert(_runtime != NULL && _rc != NULL);

  FD_ZERO(&fdsIn);

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


  if ((res = pselect(maxFd+1, &fdsIn, NULL, NULL, &s_select_timeout, NULL)) < 0)
  {
    res = errno;
    fprintf(stderr, "pselect() failed: %d\n", res);
    return res;
  }


  if (FD_ISSET(_rc->m_serverFd, &fdsIn))
  {
    if ((res = rcInputAcceptConnection(_rc)) != 0)
    {
      fprintf(stderr, "rcInputAcceptConnection() failed: %d\n", res);
      return res;
    }
  }

  if (_rc->m_stdinFd != -1 && FD_ISSET(_rc->m_stdinFd, &fdsIn))
  {
    if ((res = rcInputReadStdin(_rc)) != 0)
    {
      fprintf(stderr, "rcInputReadStdin() failed: %d\n", res);
      return res;
    }
  }

  if (_rc->m_eventInputFd != -1 && FD_ISSET(_rc->m_eventInputFd, &fdsIn))
  {
    if ((res = rcInputReadEventInput(_rc)) != 0)
    {
      fprintf(stderr, "rcInputReadEventInput() failed: %d\n", res);
      return res;
    }
  }

  if (_rc->m_connectionFd != -1 && FD_ISSET(_rc->m_connectionFd, &fdsIn))
  {
    if ((res = rcInputReadConnection(_rc)) != 0)
    {
      fprintf(stderr, "rcInputReadConnection() failed: %d\n", res);
      return res;
    }
  }


  bool ctrlManualMode;
  int ctrlChasisLR;
  int ctrlChasisFB;
  int ctrlHand;
  int ctrlArm;
  if ((res = rcInputGetManualControl(_rc, &ctrlManualMode, &ctrlChasisLR, &ctrlChasisFB, &ctrlHand, &ctrlArm)) != 0)
  {
    fprintf(stderr, "rcInputGetManualControl() failed: %d\n", res);
    return res;
  }

  if ((res = runtimeSetManualControl(_runtime, ctrlManualMode, ctrlChasisLR, ctrlChasisFB, ctrlHand, ctrlArm)) != 0)
  {
    fprintf(stderr, "runtimeSetManualControl() failed: %d\n", res);
    return res;
  }


  float detectHueFrom;
  float detectHueTo;
  float detectSatFrom;
  float detectSatTo;
  float detectValFrom;
  float detectValTo;
  if ((res = rcInputGetAutoTargetDetectParams(_rc,
                                              &detectHueFrom, &detectHueTo,
                                              &detectSatFrom, &detectSatTo,
                                              &detectValFrom, &detectValTo)) != 0)
  {
    fprintf(stderr, "rcInputGetAutoTargetDetectParams() failed: %d\n", res);
    return res;
  }

  if ((res = runtimeSetAutoTargetDetectParams(_runtime,
                                              detectHueFrom, detectHueTo,
                                              detectSatFrom, detectSatTo,
                                              detectValFrom, detectValTo)) != 0)
  {
    fprintf(stderr, "runtimeSetAutoTargetDetectParams() failed: %d\n", res);
    return res;
  }

  return 0;
}




int input_thread(void* _arg)
{
  int res = 0;
  int exit_code = EX_OK;
  Runtime* runtime = (Runtime*)_arg;

  if ((res = rcInputInit(runtimeCfgVerbose(runtime))) != 0)
  {
    fprintf(stderr, "rcInputInit() failed: %d\n", res);
    exit_code = EX_SOFTWARE;
    goto exit;
  }


  RCInput rc;
  memset(&rc, 0, sizeof(rc));
  rc.m_stdinFd = -1;
  rc.m_eventInputFd = -1;
  rc.m_serverFd = -1;
  rc.m_connectionFd = -1;
  if ((res = rcInputOpen(&rc, runtimeCfgRCInput(runtime))) != 0)
  {
    fprintf(stderr, "rcInputOpen() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_fini;
  }


  if ((res = rcInputStart(&rc)) != 0)
  {
    fprintf(stderr, "rcInputStart() failed: %d\n", res);
    exit_code = EX_IOERR;
    goto exit_rc_close;
  }


  printf("Entering input thread loop\n");
  while (!runtimeGetTerminate(runtime))
  {
    if ((res = input_loop(runtime, &rc)) != 0)
    {
      fprintf(stderr, "input_loop() failed: %d\n", res);
      exit_code = EX_SOFTWARE;
      break;
    }
  }
  printf("Left input thread loop\n");


//exit_rc_stop:
  if ((res = rcInputStop(&rc)) != 0)
    fprintf(stderr, "rcInputStop() failed: %d\n", res);


exit_rc_close:
  if ((res = rcInputClose(&rc)) != 0)
    fprintf(stderr, "rcInputClose() failed: %d\n", res);


exit_rc_fini:
  if ((res = rcInputFini()) != 0)
    fprintf(stderr, "rcInputFini() failed: %d\n", res);


exit:
  return exit_code;
}




