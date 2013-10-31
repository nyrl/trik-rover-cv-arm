#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <sys/select.h>

#include "internal/thread_input.h"
#include "internal/runtime.h"
#include "internal/module_rc.h"




static int threadInputSelectLoop(Runtime* _runtime, RCInput* _rc)
{
  int res;
  int maxFd = 0;
  fd_set fdsIn;
  static const struct timespec s_selectTimeout = { .tv_sec=1, .tv_nsec=0 };

  if (_runtime == NULL || _rc == NULL)
    return EINVAL;

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


  if ((res = pselect(maxFd+1, &fdsIn, NULL, NULL, &s_selectTimeout, NULL)) < 0)
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

  DriverManualControl driverManualControl;
  if ((res = rcInputGetManualControl(_rc, &driverManualControl)) != 0)
  {
    if (res != ENODATA)
    {
      fprintf(stderr, "rcInputGetManualControl() failed: %d\n", res);
      return res;
    }
  }
  else
  {
    if ((res = runtimeSetDriverManualControl(_runtime, &driverManualControl)) != 0)
    {
      fprintf(stderr, "runtimeSetDriverManualControl() failed: %d\n", res);
      return res;
    }
  }

  TargetDetectParams targetParams;
  if ((res = rcInputGetTargetDetectParams(_rc, &targetParams)) != 0)
  {
    if (res != ENODATA)
    {
      fprintf(stderr, "rcInputGetTargetDetectParams() failed: %d\n", res);
      return res;
    }
  }
  else
  {
    if ((res = runtimeSetTargetDetectParams(_runtime, &targetParams)) != 0)
    {
      fprintf(stderr, "runtimeSetTargetDetectParams() failed: %d\n", res);
      return res;
    }
  }

  return 0;
}




void* threadInput(void* _arg)
{
  int res = 0;
  intptr_t exit_code = 0;
  Runtime* runtime = (Runtime*)_arg;
  RCInput* rc;

  if (runtime == NULL)
  {
    exit_code = EINVAL;
    goto exit;
  }

  if ((rc = runtimeModRCInput(runtime)) == NULL)
  {
    exit_code = EINVAL;
    goto exit;
  }


  if ((res = rcInputOpen(rc, runtimeCfgRCInput(runtime))) != 0)
  {
    fprintf(stderr, "rcInputOpen() failed: %d\n", res);
    exit_code = res;
    goto exit;
  }


  if ((res = rcInputStart(rc)) != 0)
  {
    fprintf(stderr, "rcInputStart() failed: %d\n", res);
    exit_code = res;
    goto exit_rc_close;
  }


  printf("Entering input thread loop\n");
  while (!runtimeGetTerminate(runtime))
  {
    if ((res = threadInputSelectLoop(runtime, rc)) != 0)
    {
      fprintf(stderr, "threadInputSelectLoop() failed: %d\n", res);
      exit_code = res;
      goto exit_rc_stop;
    }
  }
  printf("Left input thread loop\n");


 exit_rc_stop:
  if ((res = rcInputStop(rc)) != 0)
    fprintf(stderr, "rcInputStop() failed: %d\n", res);


 exit_rc_close:
  if ((res = rcInputClose(rc)) != 0)
    fprintf(stderr, "rcInputClose() failed: %d\n", res);


 exit:
  runtimeSetTerminate(runtime);
  return (void*)exit_code;
}




