#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
#include <termios.h>
#include <netdb.h>
#include <linux/input.h>

#include "internal/module_rc.h"




static int do_openFifoInput(RCInput* _rc, const char* _fifoInputName)
{
  int res;
  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_fifoInputName != NULL)
    free(_rc->m_fifoInputName);
  _rc->m_fifoInputName = NULL;

  if (_fifoInputName == NULL)
  {
    _rc->m_fifoInputFd = -1;
    return 0;
  }

  if (mkfifo(_fifoInputName, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0)
  {
    res = errno;
    if (res != EEXIST)
      fprintf(stderr, "mkfifo(%s) failed, continuing: %d\n", _fifoInputName, res);
  }

  _rc->m_fifoInputFd = open(_fifoInputName, O_RDONLY|O_NONBLOCK);
  if (_rc->m_fifoInputFd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", _fifoInputName, errno);
    _rc->m_fifoInputFd = -1;
    unlink(_fifoInputName);
    return res;
  }
  _rc->m_fifoInputName = strdup(_fifoInputName);
  _rc->m_fifoInputReadBufferUsed = 0;

  return 0;
}

static int do_closeFifoInput(RCInput* _rc)
{
  int res;
  int exit_code = 0;

  if (_rc == NULL)
    return EINVAL;

  if (   _rc->m_fifoInputFd != -1
      && close(_rc->m_fifoInputFd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    exit_code = res;
  }

  if (_rc->m_fifoInputName != NULL)
  {
    if (unlink(_rc->m_fifoInputName) != 0)
    {
      res = errno;
      if (res != EBUSY)
      {
        fprintf(stderr, "unlink(%s) failed: %d\n", _rc->m_fifoInputName, res);
        exit_code = res;
      }
    }
    free(_rc->m_fifoInputName);
    _rc->m_fifoInputName = NULL;
  }

  return exit_code;
}


static int do_openFifoOutput(RCInput* _rc, const char* _fifoOutputName)
{
  int res;
  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_fifoOutputName != NULL)
    free(_rc->m_fifoOutputName);
  _rc->m_fifoOutputName = NULL;

  if (_fifoOutputName == NULL)
  {
    _rc->m_fifoOutputFd = -1;
    return 0;
  }

  if (mkfifo(_fifoOutputName, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) < 0)
  {
    res = errno;
    if (res != EEXIST)
      fprintf(stderr, "mkfifo(%s) failed, continuing: %d\n", _fifoOutputName, res);
  }

  _rc->m_fifoOutputFd = open(_fifoOutputName, O_WRONLY|O_NONBLOCK);
  if (_rc->m_fifoOutputFd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", _fifoOutputName, errno);
    _rc->m_fifoOutputFd = -1;
    unlink(_fifoOutputName);
    return res;
  }
  _rc->m_fifoOutputName = strdup(_fifoOutputName);

  return 0;
}

static int do_closeFifoOutput(RCInput* _rc)
{
  int res;
  int exit_code = 0;

  if (_rc == NULL)
    return EINVAL;

  if (   _rc->m_fifoOutputFd != -1
      && close(_rc->m_fifoOutputFd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    exit_code = res;
  }

  if (_rc->m_fifoOutputName != NULL)
  {
    if (unlink(_rc->m_fifoOutputName) != 0)
    {
      res = errno;
      if (res != EBUSY)
      {
        fprintf(stderr, "unlink(%s) failed: %d\n", _rc->m_fifoOutputName, res);
        exit_code = res;
      }
    }
    free(_rc->m_fifoOutputName);
    _rc->m_fifoOutputName = NULL;
  }

  return exit_code;
}


static int do_startTargetDetectParams(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_targetDetectParamsUpdated = true;

  return 0;
}

static int do_stopTargetDetectParams(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_targetDetectParamsUpdated = false;

  return 0;
}


static int do_readFifoInput(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_fifoInputFd == -1)
    return ENOTCONN;

  if (_rc->m_fifoInputReadBuffer == NULL || _rc->m_fifoInputReadBufferSize == 0)
    return EBUSY;

  if (_rc->m_fifoInputReadBufferUsed >= _rc->m_fifoInputReadBufferSize-1)
  {
    fprintf(stderr, "Input fifo overflow, truncated\n");
    _rc->m_fifoInputReadBufferUsed = 0;
  }

  const size_t available = _rc->m_fifoInputReadBufferSize - _rc->m_fifoInputReadBufferUsed - 1; //reserve space for appended trailing zero
  const ssize_t read_res = read(_rc->m_fifoInputFd, _rc->m_fifoInputReadBuffer+_rc->m_fifoInputReadBufferUsed, available);
  if (read_res < 0)
  {
    res = errno;
    fprintf(stderr, "read(%d, %zu) failed: %d\n", _rc->m_fifoInputFd, available, res);
#warning TODO
  }
  else if (read_res == 0)
  {
#warning TODO
  }
  else
  {
    _rc->m_fifoInputReadBufferUsed += read_res;
    _rc->m_fifoInputReadBuffer[_rc->m_fifoInputReadBufferUsed] = '\0';
  }


#warning TODO parse input data
  fprintf(stderr, "IN: %s\n", _rc->m_fifoInputReadBuffer);
  _rc->m_fifoInputReadBufferUsed = 0;

  return 0;
}


int rcInputInit(bool _verbose)
{
  (void)_verbose;
  return 0;
}

int rcInputFini()
{
  return 0;
}

int rcInputOpen(RCInput* _rc, const RCConfig* _config)
{
  int res = 0;

  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_fifoInputFd != -1 || _rc->m_fifoOutputFd != -1)
    return EALREADY;

  if ((res = do_openFifoInput(_rc, _config->m_fifoInput)) != 0)
    return res;

  if ((res = do_openFifoOutput(_rc, _config->m_fifoOutput)) != 0)
  {
    do_closeFifoInput(_rc);
    return res;
  }

  _rc->m_fifoInputReadBufferSize = 1000;
  _rc->m_fifoInputReadBufferUsed = 0;
  _rc->m_fifoInputReadBuffer = malloc(_rc->m_fifoInputReadBufferSize);

  return 0;
}

int rcInputClose(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_fifoInputFd == -1 && _rc->m_fifoOutputFd == -1)
    return EALREADY;

  if (_rc->m_fifoInputReadBuffer)
    free(_rc->m_fifoInputReadBuffer);
  _rc->m_fifoInputReadBuffer = NULL;
  _rc->m_fifoInputReadBufferSize = 0;

  do_closeFifoOutput(_rc);
  do_closeFifoInput(_rc);

  return 0;
}

int rcInputStart(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_fifoInputFd == -1 && _rc->m_fifoOutputFd == -1)
    return ENOTCONN;

  if ((res = do_startTargetDetectParams(_rc)) != 0)
    return res;

  return 0;
}

int rcInputStop(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_fifoInputFd == -1 && _rc->m_fifoOutputFd == -1)
    return ENOTCONN;

  do_stopTargetDetectParams(_rc);

  return 0;
}

int rcInputReadFifoInput(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_readFifoInput(_rc)) != 0)
    return res;

  return 0;
}


int rcInputGetTargetDetectParams(RCInput* _rc,
                                 TargetDetectParams* _targetParams)
{
  if (_rc == NULL)
    return EINVAL;

  if (!_rc->m_targetDetectParamsUpdated)
    return ENODATA;

  _rc->m_targetDetectParamsUpdated = false;
  _targetParams->m_detectHueFrom = _rc->m_targetDetectHueFrom;
  _targetParams->m_detectHueTo   = _rc->m_targetDetectHueTo;
  _targetParams->m_detectSatFrom = _rc->m_targetDetectSatFrom;
  _targetParams->m_detectSatTo   = _rc->m_targetDetectSatTo;
  _targetParams->m_detectValFrom = _rc->m_targetDetectValFrom;
  _targetParams->m_detectValTo   = _rc->m_targetDetectValTo;

  return 0;
}


