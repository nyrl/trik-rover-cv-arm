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


static int do_listenServerFd(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if (listen(_rc->m_serverFd, 1) != 0)
  {
    res = errno;
    fprintf(stderr, "listen() failed: %d\n", res);
    return res;
  }

  return 0;
}

static int do_unlistenServerFd(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

#warning Seems to be impossible without closing socket?..

  return 0;
}


static int do_readStdio(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  char key;
  if ((res = read(0, &key, 1)) != 1)
  {
    if (res >= 0)
      res = E2BIG;
    else
      res = errno;
    fprintf(stderr, "read(stdin) failed: %d\n", res);
    return res;
  }

  switch (key)
  {
    case '1': _rc->m_autoTargetDetectHue += 1.0f; break;
    case '2': _rc->m_autoTargetDetectHue -= 1.0f; break;
    case '!': _rc->m_autoTargetDetectHueTolerance += 1.0f; break;
    case '@': _rc->m_autoTargetDetectHueTolerance -= 1.0f; break;
    case '3': _rc->m_autoTargetDetectSat += 0.05f; break;
    case '4': _rc->m_autoTargetDetectSat -= 0.05f; break;
    case '#': _rc->m_autoTargetDetectSatTolerance += 0.05f; break;
    case '$': _rc->m_autoTargetDetectSatTolerance -= 0.05f; break;
    case '5': _rc->m_autoTargetDetectVal += 0.05f; break;
    case '6': _rc->m_autoTargetDetectVal -= 0.05f; break;
    case '%': _rc->m_autoTargetDetectValTolerance += 0.05f; break;
    case '^': _rc->m_autoTargetDetectValTolerance -= 0.05f; break;
    case 'w': _rc->m_manualCtrlChasisFB += 10; break;
    case 'W': _rc->m_manualCtrlChasisFB = 0; break;
    case 's': _rc->m_manualCtrlChasisFB -= 10; break;
    case 'S': _rc->m_manualCtrlChasisFB = 0; break;
    case 'a': _rc->m_manualCtrlChasisLR -= 10; break;
    case 'A': _rc->m_manualCtrlChasisLR = 0; break;
    case 'd': _rc->m_manualCtrlChasisLR += 10; break;
    case 'D': _rc->m_manualCtrlChasisLR = 0; break;
    case 'z': _rc->m_manualCtrlArm += 10; break;
    case 'Z': _rc->m_manualCtrlArm = 0; break;
    case 'x': _rc->m_manualCtrlArm -= 10; break;
    case 'X': _rc->m_manualCtrlArm = 00; break;
    case 'r': _rc->m_manualCtrlHand += 10; break;
    case 'R': _rc->m_manualCtrlHand = 0; break;
    case 'f': _rc->m_manualCtrlHand -= 10; break;
    case 'F': _rc->m_manualCtrlHand = 00; break;
    case 'm': _rc->m_manualMode = !_rc->m_manualMode; break;
  };

  fprintf(stderr, "Target detection: hue %f [%f], sat %f [%f], val %f [%f]\n",
          _rc->m_autoTargetDetectHue, _rc->m_autoTargetDetectHueTolerance,
          _rc->m_autoTargetDetectSat, _rc->m_autoTargetDetectSatTolerance,
          _rc->m_autoTargetDetectVal, _rc->m_autoTargetDetectValTolerance);
  fprintf(stderr, "Manual control: %s chasis %d %d, hand %d, arm %d\n",
          _rc->m_manualMode?"manual":"auto",
          _rc->m_manualCtrlChasisLR,
          _rc->m_manualCtrlChasisFB,
          _rc->m_manualCtrlHand,
          _rc->m_manualCtrlArm);
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
  int res;

  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

  if ((res = do_listenServerFd(_rc)) != 0)
    return res;

  _rc->m_manualCtrlChasisLR = 0;
  _rc->m_manualCtrlChasisFB = 0;
  _rc->m_manualCtrlHand = 0;
  _rc->m_manualCtrlArm = 0;

  return 0;
}

int rcInputStop(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

  if ((res = do_unlistenServerFd(_rc)) != 0)
    return res;

  return 0;
}

int rcInputGetStdin(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_readStdio(_rc)) != 0)
    return res;

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

