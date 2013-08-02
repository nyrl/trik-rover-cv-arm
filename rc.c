#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
#include <termios.h>
#include <netdb.h>

#include "internal/rc.h"




static int do_openServerFd(RCInput* _rc, int _port)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  _rc->m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (_rc->m_serverFd < 0)
  {
    res = errno;
    _rc->m_serverFd = -1;
    fprintf(stderr, "socket(AF_INET, SOCK_STREAM) failed: %d\n", res);
    return res;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = _port;
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(_rc->m_serverFd, (struct sockaddr*)&addr, sizeof(addr)) != 0)
  {
    res = errno;
    fprintf(stderr, "bind(%d) failed: %d\n", _port, res);
    close(_rc->m_serverFd);
    _rc->m_serverFd = -1;
    return res;
  }

  return 0;
}

static int do_closeServerFd(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  close(_rc->m_serverFd);
  _rc->m_serverFd = -1;

  return 0;
}

static int do_openStdio(RCInput* _rc)
{
  int res;
  struct termios ts;

  if ((res = tcgetattr(0, &ts)) != 0)
  {
    res = errno;
    fprintf(stderr, "tcgetattr() failed: %d\n", res);
    return res;
  }

  ts.c_lflag &= ~ICANON;
  ts.c_cc[VMIN] = 0;
  ts.c_cc[VTIME] = 0;

  if ((res = tcsetattr(0, TCSANOW, &ts)) != 0)
  {
    res = errno;
    fprintf(stderr, "tcsetattr() failed: %d\n", res);
    return res;
  }

  return 0;
}

static int do_closeStdio(RCInput* _rc)
{
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
  if (_rc->m_serverFd != -1)
    return EALREADY;

  if ((res = do_openServerFd(_rc, _config->m_port)) != 0)
    return res;

  if ((res = do_openStdio(_rc)) != 0)
  {
    do_closeServerFd(_rc);
    return res;
  }

  _rc->m_connectionFd = -1;
  _rc->m_manualMode = _config->m_manualMode;
  _rc->m_autoTargetDetectHue          = _config->m_autoTargetDetectHue;
  _rc->m_autoTargetDetectHueTolerance = _config->m_autoTargetDetectHueTolerance;
  _rc->m_autoTargetDetectSat          = _config->m_autoTargetDetectSat;
  _rc->m_autoTargetDetectSatTolerance = _config->m_autoTargetDetectSatTolerance;
  _rc->m_autoTargetDetectVal          = _config->m_autoTargetDetectVal;
  _rc->m_autoTargetDetectValTolerance = _config->m_autoTargetDetectValTolerance;

  return 0;
}

int rcInputClose(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return EALREADY;

  do_closeStdio(_rc);
  do_closeServerFd(_rc);

  return 0;
}

int rcInputStart(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

#warning TODO

  _rc->m_manualCtrlChasisLR = 0;
  _rc->m_manualCtrlChasisFB = 0;
  _rc->m_manualCtrlHand = 0;
  _rc->m_manualCtrlArm = 0;

  return 0;
}

int rcInputStop(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

#warning TODO

  return 0;
}

int rcInputGetStdin(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

#warning Temporary: print key
  char key;
  if ((read(0, &key, 1)) != 1)
  {
    fprintf(stderr, "cannot read from stdin, errno %d\n", errno);
    return EFAULT;
  }

  printf("Key: %02x '%c'\n", key, key);

  return 0;
}

int rcInputAcceptConnection(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

#warning Temporary
  fprintf(stderr, "incoming connection!\n");
  close(accept(_rc->m_serverFd, NULL, NULL));


  return 0;
}


bool rcInputIsManualMode(RCInput* _rc)
{
  if (_rc == NULL)
    return false;

  return _rc->m_manualMode;
}

int rcInputGetManualCommand(RCInput* _rc, int* _ctrlChasisLR, int* _ctrlChasisFB, int* _ctrlHand, int* _ctrlArm)
{
  if (_rc == NULL)
    return EINVAL;

  if (!_rc->m_manualMode)
    return EADDRINUSE;

  if (_ctrlChasisLR)
    *_ctrlChasisLR = _rc->m_manualCtrlChasisLR;
  if (_ctrlChasisFB)
    *_ctrlChasisFB = _rc->m_manualCtrlChasisFB;
  if (_ctrlHand)
    *_ctrlHand = _rc->m_manualCtrlHand;
  if (_ctrlArm)
    *_ctrlArm = _rc->m_manualCtrlArm;

  return 0;
}

int rcInputGetAutoTargetDetect(RCInput* _rc,
                               float* _detectHueFrom, float* _detectHueTo,
                               float* _detectSatFrom, float* _detectSatTo,
                               float* _detectValFrom, float* _detectValTo)
{
  if (_rc == NULL)
    return EINVAL;

  // valid both in auto and manual mode!
  *_detectHueFrom = _rc->m_autoTargetDetectHue-_rc->m_autoTargetDetectHueTolerance;
  *_detectHueTo   = _rc->m_autoTargetDetectHue+_rc->m_autoTargetDetectHueTolerance;
  while (*_detectHueFrom < 0.0f)
    *_detectHueFrom += 360.0f;
  while (*_detectHueFrom >= 360.0f)
    *_detectHueFrom -= 360.0f;
  while (*_detectHueTo < 0.0f)
    *_detectHueTo += 360.0f;
  while (*_detectHueTo >= 360.0f)
    *_detectHueTo -= 360.0f;

  *_detectSatFrom = _rc->m_autoTargetDetectSat-_rc->m_autoTargetDetectSatTolerance;
  *_detectSatTo   = _rc->m_autoTargetDetectSat+_rc->m_autoTargetDetectSatTolerance;
  *_detectValFrom = _rc->m_autoTargetDetectVal-_rc->m_autoTargetDetectValTolerance;
  *_detectValTo   = _rc->m_autoTargetDetectVal+_rc->m_autoTargetDetectValTolerance;

  return 0;
}

