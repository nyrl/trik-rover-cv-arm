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

  int reuseAddr = 1;
  if (setsockopt(_rc->m_serverFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) != 0)
    fprintf(stderr, "setsockopt(%d, SO_REUSEADDR) failed:\n", errno);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_port);
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

static int do_openStdin(RCInput* _rc, bool m_stdin)
{
  int res;
  struct termios ts;

  if (!m_stdin)
  {
    _rc->m_stdinFd = -1;
    return 0;
  }

  _rc->m_stdinFd = 0;
  if ((res = tcgetattr(_rc->m_stdinFd, &ts)) != 0)
  {
    res = errno;
    fprintf(stderr, "tcgetattr() failed: %d\n", res);
    _rc->m_stdinFd = -1;
    return res;
  }

  ts.c_lflag &= ~ICANON;
  ts.c_cc[VMIN] = 0;
  ts.c_cc[VTIME] = 0;

  if ((res = tcsetattr(_rc->m_stdinFd, TCSANOW, &ts)) != 0)
  {
    res = errno;
    fprintf(stderr, "tcsetattr() failed: %d\n", res);
    _rc->m_stdinFd = -1;
    return res;
  }

  return 0;
}

static int do_closeStdin(RCInput* _rc)
{
  return 0;
}


static int do_openEventInput(RCInput* _rc, const char* _eventInput)
{
  int res;
  if (_rc == NULL)
    return EINVAL;

  if (_eventInput == NULL)
  {
    _rc->m_eventInputFd = -1;
    return 0;
  }

  _rc->m_eventInputFd = open(_eventInput, O_RDONLY|O_SYNC, 0);
  if (_rc->m_eventInputFd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", _eventInput, res);
    _rc->m_eventInputFd = -1;
    return res;
  }

  return 0;
}

static int do_closeEventInput(RCInput* _rc)
{
  int res;
  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_eventInputFd == -1)
    return 0;

  if (close(_rc->m_eventInputFd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    return res;
  }

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

  // not possible without closing socket

  return 0;
}

static int do_setupDriverManualControl(RCInput* _rc, bool _manualMode)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_manualControl.m_manualMode = _manualMode;

  return 0;
}

static int do_startDriverManualControl(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_manualControlUpdated = true;
  _rc->m_manualControl.m_ctrlChasisLR = 0;
  _rc->m_manualControl.m_ctrlChasisFB = 0;
  _rc->m_manualControl.m_ctrlHand     = 0;
  _rc->m_manualControl.m_ctrlArm      = 0;

  return 0;
}

static void do_resetDriverManualControl(RCInput* _rc)
{
  if (_rc == NULL)
    return;

  _rc->m_manualControlUpdated = true;
  _rc->m_manualControl.m_ctrlChasisLR = 0;
  _rc->m_manualControl.m_ctrlChasisFB = 0;
  _rc->m_manualControl.m_ctrlHand     = 0;
  _rc->m_manualControl.m_ctrlArm      = 0;
}

static int do_stopDriverManualControl(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_manualControlUpdated = false;

  return 0;
}

static int do_setupTargetDetectParams(RCInput* _rc,
                                      int _targetDetectHue, int _targetDetectHueTolerance,
                                      int _targetDetectSat, int _targetDetectSatTolerance,
                                      int _targetDetectVal, int _targetDetectValTolerance)
{
  if (_rc == NULL)
    return EINVAL;

  _rc->m_targetDetectHue          = _targetDetectHue;
  _rc->m_targetDetectHueTolerance = _targetDetectHueTolerance;
  _rc->m_targetDetectSat          = _targetDetectSat;
  _rc->m_targetDetectSatTolerance = _targetDetectSatTolerance;
  _rc->m_targetDetectVal          = _targetDetectVal;
  _rc->m_targetDetectValTolerance = _targetDetectValTolerance;

  return 0;
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


static int makeValueRange(int _val, int _adj, int _min, int _max)
{
  _val += _adj;
  if (_val > _max)
    return _max;
  else if (_val < _min)
    return _min;
  else
    return _val;
}

static int makeValueWrap(int _val, int _adj, int _min, int _max)
{
  _val += _adj;
  while (_val > _max)
    _val -= (_max-_min);
  while (_val < _min)
    _val += (_max-_min);

  return _val;
}

static void adjustValueRange(int* _val, int _adj, int _min, int _max)
{
  *_val = makeValueRange(*_val, _adj, _min, _max);
}

static void adjustValueWrap(int* _val, int _adj, int _min, int _max)
{
  *_val = makeValueWrap(*_val, _adj, _min, _max);
}

static int do_readStdin(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_stdinFd == -1)
    return ENOTCONN;

  char key;
  if ((res = read(_rc->m_stdinFd, &key, 1)) != 1)
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
    case '1': adjustValueWrap(&_rc->m_targetDetectHue, +1, 0, 360);           _rc->m_targetDetectParamsUpdated = true; break;
    case '2': adjustValueWrap(&_rc->m_targetDetectHue, -1, 0, 360);           _rc->m_targetDetectParamsUpdated = true; break;
    case '!': adjustValueRange(&_rc->m_targetDetectHueTolerance, +1, 0, 179); _rc->m_targetDetectParamsUpdated = true; break;
    case '@': adjustValueRange(&_rc->m_targetDetectHueTolerance, -1, 0, 179); _rc->m_targetDetectParamsUpdated = true; break;
    case '3': adjustValueRange(&_rc->m_targetDetectSat, +1, 0, 100);          _rc->m_targetDetectParamsUpdated = true; break;
    case '4': adjustValueRange(&_rc->m_targetDetectSat, -1, 0, 100);          _rc->m_targetDetectParamsUpdated = true; break;
    case '#': adjustValueRange(&_rc->m_targetDetectSatTolerance, +1, 0, 100); _rc->m_targetDetectParamsUpdated = true; break;
    case '$': adjustValueRange(&_rc->m_targetDetectSatTolerance, -1, 0, 100); _rc->m_targetDetectParamsUpdated = true; break;
    case '5': adjustValueRange(&_rc->m_targetDetectVal, +1, 0, 100);          _rc->m_targetDetectParamsUpdated = true; break;
    case '6': adjustValueRange(&_rc->m_targetDetectVal, -1, 0, 100);          _rc->m_targetDetectParamsUpdated = true; break;
    case '%': adjustValueRange(&_rc->m_targetDetectValTolerance, +1, 0, 100); _rc->m_targetDetectParamsUpdated = true; break;
    case '^': adjustValueRange(&_rc->m_targetDetectValTolerance, -1, 0, 100); _rc->m_targetDetectParamsUpdated = true; break;
    case 'w': adjustValueRange(&_rc->m_manualControl.m_ctrlChasisFB, +5, -100, 100); _rc->m_manualControlUpdated = true; break;
    case 'W': _rc->m_manualControl.m_ctrlChasisFB = 0;                               _rc->m_manualControlUpdated = true; break;
    case 's': adjustValueRange(&_rc->m_manualControl.m_ctrlChasisFB, -5, -100, 100); _rc->m_manualControlUpdated = true; break;
    case 'S': _rc->m_manualControl.m_ctrlChasisFB = 0;                               _rc->m_manualControlUpdated = true; break;
    case 'a': adjustValueRange(&_rc->m_manualControl.m_ctrlChasisLR, -5, -100, 100); _rc->m_manualControlUpdated = true; break;
    case 'A': _rc->m_manualControl.m_ctrlChasisLR = 0;                               _rc->m_manualControlUpdated = true; break;
    case 'd': adjustValueRange(&_rc->m_manualControl.m_ctrlChasisLR, +5, -100, 100); _rc->m_manualControlUpdated = true; break;
    case 'D': _rc->m_manualControl.m_ctrlChasisLR = 0;                               _rc->m_manualControlUpdated = true; break;
    case 'z': adjustValueRange(&_rc->m_manualControl.m_ctrlArm, +5, -100, 100);      _rc->m_manualControlUpdated = true; break;
    case 'Z': _rc->m_manualControl.m_ctrlArm = 0;                                    _rc->m_manualControlUpdated = true; break;
    case 'x': adjustValueRange(&_rc->m_manualControl.m_ctrlArm, -5, -100, 100);      _rc->m_manualControlUpdated = true; break;
    case 'X': _rc->m_manualControl.m_ctrlArm = 0;                                    _rc->m_manualControlUpdated = true; break;
    case 'r': adjustValueRange(&_rc->m_manualControl.m_ctrlHand, +5, -100, 100);     _rc->m_manualControlUpdated = true; break;
    case 'R': _rc->m_manualControl.m_ctrlHand = 0;                                   _rc->m_manualControlUpdated = true; break;
    case 'f': adjustValueRange(&_rc->m_manualControl.m_ctrlHand, -5, -100, 100);     _rc->m_manualControlUpdated = true; break;
    case 'F': _rc->m_manualControl.m_ctrlHand = 0;                                   _rc->m_manualControlUpdated = true; break;
    case 'm': _rc->m_manualControl.m_manualMode = !_rc->m_manualControl.m_manualMode;_rc->m_manualControlUpdated = true; break;
  };

  fprintf(stderr, "Target detection: hue %d [%d], sat %d [%d], val %d [%d]\n",
          _rc->m_targetDetectHue, _rc->m_targetDetectHueTolerance,
          _rc->m_targetDetectSat, _rc->m_targetDetectSatTolerance,
          _rc->m_targetDetectVal, _rc->m_targetDetectValTolerance);
  fprintf(stderr, "Manual control: %s chasis %d %d, hand %d, arm %d\n",
          _rc->m_manualControl.m_manualMode?"manual":"auto",
          _rc->m_manualControl.m_ctrlChasisLR,
          _rc->m_manualControl.m_ctrlChasisFB,
          _rc->m_manualControl.m_ctrlHand,
          _rc->m_manualControl.m_ctrlArm);
  return 0;
}

static int do_readEventInput(RCInput* _rc)
{
  int res;
  struct input_event evt;

  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_eventInputFd == -1)
    return ENOTCONN;

  if ((res = read(_rc->m_eventInputFd, &evt, sizeof(evt))) != sizeof(evt))
  {
    if (res >= 0)
      res = E2BIG;
    else
      res = errno;
    fprintf(stderr, "read(event-input) failed: %d\n", res);
    return res;
  }

  if (   evt.type == EV_KEY
      && evt.code == KEY_F2
      && evt.value == 0) // key release
  {
    _rc->m_manualControl.m_manualMode = !_rc->m_manualControl.m_manualMode;
    _rc->m_manualControlUpdated = true;
    fprintf(stderr, "%s control\n", _rc->m_manualControl.m_manualMode?"manual":"auto");
  }

  return 0;
}

static int do_acceptConnection(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_connectionFd != -1)
  {
    fprintf(stderr, "Replacing existing remote control connection\n");
    close(_rc->m_connectionFd);
  }

  _rc->m_connectionFd = accept(_rc->m_serverFd, NULL, NULL);
  if (_rc->m_connectionFd < 0)
  {
    res = errno;
    _rc->m_connectionFd = -1;
    fprintf(stderr, "accept() failed: %d\n", res);
    return res;
  }

  _rc->m_readBufferUsed = 0;
  do_resetDriverManualControl(_rc);

  return 0;
}

static int do_dropConnection(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_connectionFd != -1)
  {
    fprintf(stderr, "Dropping existing remote control connection\n");
    close(_rc->m_connectionFd);
  }

  _rc->m_connectionFd = -1;

  _rc->m_readBufferUsed = 0;
  do_resetDriverManualControl(_rc);

  return 0;
}

static int do_readConnection(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if (_rc->m_connectionFd == -1)
    return ENOTCONN;

  if (_rc->m_readBuffer == NULL || _rc->m_readBufferSize == 0)
    return EBUSY;

  if (_rc->m_readBufferUsed >= _rc->m_readBufferSize-1)
  {
    fprintf(stderr, "Control buffer overflow\n");
    _rc->m_readBufferUsed = 0;
  }

  size_t available = _rc->m_readBufferSize-_rc->m_readBufferUsed-1;
  ssize_t rd = read(_rc->m_connectionFd, _rc->m_readBuffer+_rc->m_readBufferUsed, available);
  if (rd < 0)
  {
    res = errno;
    fprintf(stderr, "read(%d, %zu) failed: %d\n", _rc->m_connectionFd, available, res);
    do_dropConnection(_rc);
    return res;
  }
  else if (rd == 0)
    return do_dropConnection(_rc);

  _rc->m_readBufferUsed += rd;
  _rc->m_readBuffer[_rc->m_readBufferUsed] = '\0';

  char* parseAt = _rc->m_readBuffer;
  char* parseTill;
  while ((parseTill = strchr(parseAt, '\n')) != NULL)
  {
    *parseTill = '\0';
    if (strncmp(parseAt, "pad 1 ", strlen("pad 1 ")) == 0)
    {
      parseAt += strlen("pad 1 ");
      if (strncmp(parseAt, "up", strlen("up")) == 0)
      {
        _rc->m_manualControl.m_ctrlChasisLR = 0;
        _rc->m_manualControl.m_ctrlChasisFB = 0;
        _rc->m_manualControlUpdated = true;
      }
      else
      {
        int lr;
        int fb;
        if (sscanf(parseAt, "%d %d", &lr, &fb) == 2)
        {
          _rc->m_manualControl.m_ctrlChasisLR = makeValueRange(0, lr, -100, 100);
          _rc->m_manualControl.m_ctrlChasisFB = makeValueRange(0, fb, -100, 100);
          _rc->m_manualControlUpdated = true;
        }
        else
          fprintf(stderr, "Failed to parse pad 1 arguments '%s'\n", parseAt);
      }
    }
    else if (strncmp(parseAt, "pad 2 ", strlen("pad 2 ")) == 0)
    {
      parseAt += strlen("pad 2 ");
      if (strncmp(parseAt, "up", strlen("up")) == 0)
      {
        _rc->m_manualControl.m_ctrlHand = 0;
        _rc->m_manualControl.m_ctrlArm = 0;
        _rc->m_manualControlUpdated = true;
      }
      else
      {
        int hand;
        int arm;
        if (sscanf(parseAt, "%d %d", &arm, &hand) == 2)
        {
          _rc->m_manualControl.m_ctrlHand = makeValueRange(0, hand, -100, 100);
          _rc->m_manualControl.m_ctrlArm  = makeValueRange(0, arm,  -100, 100);
        }
        else
          fprintf(stderr, "Failed to parse pad 2 arguments '%s'\n", parseAt);
      }
    }
    else if (strncmp(parseAt, "btn ", strlen("btn ")) == 0)
    {
      parseAt += strlen("btn ");
      int btn;
      if (sscanf(parseAt, "%d down", &btn) == 1)
      {
        if (btn == 1)
          _rc->m_manualControl.m_manualMode = true;
        else if (btn == 2)
          _rc->m_manualControl.m_manualMode = false;
        _rc->m_manualControlUpdated = true;
      }
      else
        fprintf(stderr, "Failed to parse btn arguments '%s'\n", parseAt);
    }
    else
      fprintf(stderr, "Unable to parse remote control command '%s'\n", parseAt);

    parseAt = parseTill+1;
  }

  _rc->m_readBufferUsed -= (parseAt-_rc->m_readBuffer);
  memmove(_rc->m_readBuffer, parseAt, _rc->m_readBufferUsed);

  fprintf(stderr, "Manual control: %s chasis %d %d, hand %d, arm %d\n",
          _rc->m_manualControl.m_manualMode?"manual":"auto",
          _rc->m_manualControl.m_ctrlChasisLR,
          _rc->m_manualControl.m_ctrlChasisFB,
          _rc->m_manualControl.m_ctrlHand,
          _rc->m_manualControl.m_ctrlArm);

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
  if (_rc->m_eventInputFd != -1 || _rc->m_serverFd != -1)
    return EALREADY;

  if ((res = do_openServerFd(_rc, _config->m_port)) != 0)
    return res;

  if ((res = do_openStdin(_rc, _config->m_stdin)) != 0)
  {
    do_closeServerFd(_rc);
    return res;
  }

  if ((res = do_openEventInput(_rc, _config->m_eventInput)) != 0)
  {
    do_closeStdin(_rc);
    do_closeServerFd(_rc);
    return res;
  }

  if ((res = do_setupDriverManualControl(_rc, _config->m_manualMode)) != 0)
  {
    do_closeEventInput(_rc);
    do_closeStdin(_rc);
    do_closeServerFd(_rc);
  }

  if ((res = do_setupTargetDetectParams(_rc,
                                        _config->m_targetDetectHue, _config->m_targetDetectHueTolerance,
                                        _config->m_targetDetectSat, _config->m_targetDetectSatTolerance,
                                        _config->m_targetDetectVal, _config->m_targetDetectValTolerance)) != 0)
  {
    do_closeEventInput(_rc);
    do_closeStdin(_rc);
    do_closeServerFd(_rc);
  }

  _rc->m_readBufferSize = 1000;
  _rc->m_readBuffer = malloc(_rc->m_readBufferSize);

  _rc->m_connectionFd = -1;

  return 0;
}

int rcInputClose(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return EALREADY;

  free(_rc->m_readBuffer);
  _rc->m_readBuffer = NULL;
  _rc->m_readBufferSize = 0;

  do_closeEventInput(_rc);
  do_closeStdin(_rc);
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

  if ((res = do_startDriverManualControl(_rc)) != 0)
  {
    do_unlistenServerFd(_rc);
    return res;
  }

  if ((res = do_startTargetDetectParams(_rc)) != 0)
  {
    do_stopDriverManualControl(_rc);
    do_unlistenServerFd(_rc);
    return res;
  }

  return 0;
}

int rcInputStop(RCInput* _rc)
{
  if (_rc == NULL)
    return EINVAL;
  if (_rc->m_serverFd == -1)
    return ENOTCONN;

  do_stopTargetDetectParams(_rc);
  do_stopDriverManualControl(_rc);

  do_unlistenServerFd(_rc);

  return 0;
}

int rcInputReadStdin(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_readStdin(_rc)) != 0)
    return res;

  return 0;
}

int rcInputReadEventInput(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_readEventInput(_rc)) != 0)
    return res;

  return 0;
}

int rcInputAcceptConnection(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_acceptConnection(_rc)) != 0)
    return res;

  return 0;
}

int rcInputReadConnection(RCInput* _rc)
{
  int res;

  if (_rc == NULL)
    return EINVAL;

  if ((res = do_readConnection(_rc)) != 0)
    return res;

  return 0;
}


int rcInputGetManualControl(RCInput* _rc,
                            DriverManualControl* _manualControl)
{
  if (_rc == NULL || _manualControl == NULL)
    return EINVAL;

  if (!_rc->m_manualControlUpdated)
    return ENODATA;

  _rc->m_manualControlUpdated = false;
  *_manualControl = _rc->m_manualControl;

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
  _targetParams->m_detectHueFrom = makeValueWrap( _rc->m_targetDetectHue, -_rc->m_targetDetectHueTolerance, 0, 360);
  _targetParams->m_detectHueTo   = makeValueWrap( _rc->m_targetDetectHue, +_rc->m_targetDetectHueTolerance, 0, 360);
  _targetParams->m_detectSatFrom = makeValueRange(_rc->m_targetDetectSat, -_rc->m_targetDetectSatTolerance, 0, 100);
  _targetParams->m_detectSatTo   = makeValueRange(_rc->m_targetDetectSat, +_rc->m_targetDetectSatTolerance, 0, 100);
  _targetParams->m_detectValFrom = makeValueRange(_rc->m_targetDetectVal, -_rc->m_targetDetectValTolerance, 0, 100);
  _targetParams->m_detectValTo   = makeValueRange(_rc->m_targetDetectVal, +_rc->m_targetDetectValTolerance, 0, 100);

  return 0;
}


