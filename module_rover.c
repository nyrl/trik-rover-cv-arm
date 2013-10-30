#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>

#include "internal/module_rover.h"




static int do_roverOpenMotorMsp(RoverOutput* _rover,
                                RoverMotorMsp* _motor,
                                const RoverConfigMotorMsp* _config)
{
  int res;

  if (_rover == NULL || _motor == NULL || _config == NULL)
    return EINVAL;

  char busPath[100];
  snprintf(busPath, sizeof(busPath), "/dev/i2c-%d", _config->m_mspI2CBusId);
  _motor->m_i2cBusFd = open(busPath, O_RDWR);
  if (_motor->m_i2cBusFd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", busPath, res);
    _motor->m_i2cBusFd = -1;
    return res;
  }

  _motor->m_mspI2CDeviceId   = _config->m_mspI2CDeviceId;
  _motor->m_mspI2CMotorCmd   = _config->m_mspI2CMotorCmd;
  _motor->m_powerBackFull    = _config->m_powerBackFull;
  _motor->m_powerBackZero    = _config->m_powerBackZero;
  _motor->m_powerNeutral     = _config->m_powerNeutral;
  _motor->m_powerForwardZero = _config->m_powerForwardZero;
  _motor->m_powerForwardFull = _config->m_powerForwardFull;

  return 0;
}

static int do_roverCloseMotorMsp(RoverOutput* _rover,
                                 RoverMotorMsp* _motor)
{
  int res;

  if (_rover == NULL || _motor == NULL)
    return EINVAL;

  if (close(_motor->m_i2cBusFd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    return res;
  }
  _motor->m_i2cBusFd = -1;

  return 0;
}

static int do_roverOpenMotor(RoverOutput* _rover,
                             RoverMotor* _motor,
                             const RoverConfigMotor* _config)
{
  int res;

  if (_rover == NULL || _motor == NULL || _config == NULL || _config->m_path == NULL)
    return EINVAL;

  _motor->m_fd = open(_config->m_path, O_WRONLY|O_SYNC, 0);
  if (_motor->m_fd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", _config->m_path, res);
    _motor->m_fd = -1;
    return res;
  }

  _motor->m_powerBackFull    = _config->m_powerBackFull;
  _motor->m_powerBackZero    = _config->m_powerBackZero;
  _motor->m_powerNeutral     = _config->m_powerNeutral;
  _motor->m_powerForwardZero = _config->m_powerForwardZero;
  _motor->m_powerForwardFull = _config->m_powerForwardFull;

  return 0;
}

static int do_roverCloseMotor(RoverOutput* _rover,
                              RoverMotor* _motor)
{
  int res;

  if (_rover == NULL || _motor == NULL)
    return EINVAL;

  if (close(_motor->m_fd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    return res;
  }
  _motor->m_fd = -1;

  return 0;
}

static int do_roverOpen(RoverOutput* _rover,
                        const RoverConfig* _config)
{
  int res;

  if (_rover == NULL || _config == NULL)
    return EINVAL;

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorChasisLeft1, &_config->m_motorChasisLeft1)) != 0)
  {
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorChasisLeft2, &_config->m_motorChasisLeft2)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorChasisRight1, &_config->m_motorChasisRight1)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorChasisRight2, &_config->m_motorChasisRight2)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motorHand1, &_config->m_motorHand1)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motorHand2, &_config->m_motorHand2)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motorHand1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motorArm, &_config->m_motorArm)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motorHand2);
    do_roverCloseMotor(_rover, &_rover->m_motorHand1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft1);
    return res;
  }

  return 0;
}

static int do_roverClose(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;

  do_roverCloseMotor(_rover, &_rover->m_motorArm);
  do_roverCloseMotor(_rover, &_rover->m_motorHand2);
  do_roverCloseMotor(_rover, &_rover->m_motorHand1);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight2);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisLeft2);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorChasisRight1);

  return 0;
}

static int do_roverMotorSetPower(RoverOutput* _rover,
                                 RoverMotor* _motor,
                                 int _power)
{
  int res;

  if (_rover == NULL || _motor == NULL)
    return EINVAL;

  int pwm;

  if (_power == 0)
    pwm = _motor->m_powerNeutral;
  else if (_power < 0)
  {
    if (_power < -100)
      pwm = _motor->m_powerBackFull;
    else
      pwm = _motor->m_powerBackZero + ((_motor->m_powerBackFull-_motor->m_powerBackZero)*(-_power))/100;
  }
  else
  {
    if (_power > 100)
      pwm = _motor->m_powerForwardFull;
    else
      pwm = _motor->m_powerForwardZero + ((_motor->m_powerForwardFull-_motor->m_powerForwardZero)*_power)/100;
  }

  if (dprintf(_motor->m_fd, "%d\n", pwm) < 0)
  {
    res = errno;
    fprintf(stderr, "dprintf(%d, %d) failed: %d\n", _motor->m_fd, pwm, res);
    return res;
  }
  fsync(_motor->m_fd);

  return 0;
}

static int do_roverMotorMspSetPower(RoverOutput* _rover,
                                    RoverMotorMsp* _motor,
                                    int _power)
{
  int res;

  if (_rover == NULL || _motor == NULL)
    return EINVAL;

  int pwm;

  if (_power == 0) // neutral
    pwm = _motor->m_powerNeutral;
  else if (_power < 0) // back
  {
    if (_power < -100)
      pwm = _motor->m_powerBackFull;
    else
      pwm = _motor->m_powerBackZero + ((_motor->m_powerBackFull-_motor->m_powerBackZero)*(-_power))/100;
  }
  else // forward
  {
    if (_power > 100)
      pwm = _motor->m_powerForwardFull;
    else
      pwm = _motor->m_powerForwardZero + ((_motor->m_powerForwardFull-_motor->m_powerForwardZero)*_power)/100;
  }

  int devId = _motor->m_mspI2CDeviceId;
  if (ioctl(_motor->m_i2cBusFd, I2C_SLAVE, devId) != 0)
  {
    res = errno;
    fprintf(stderr, "ioctl(%d, I2C_SLAVE, %d) failed: %d\n", _motor->m_i2cBusFd, devId, res);
    return res;
  }

  unsigned char cmd[2];
  cmd[0] = _motor->m_mspI2CMotorCmd;
  cmd[1] = pwm;

  if ((res = write(_motor->m_i2cBusFd, &cmd, sizeof(cmd))) != sizeof(cmd))
  {
    if (res >= 0)
      res = E2BIG;
    else
      res = errno;
    fprintf(stderr, "write(%d) failed: %d\n", _motor->m_i2cBusFd, res);
    return res;
  }

  return 0;
}


static int do_roverMotorsStart(RoverOutput* _rover)
{
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft1, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft2, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight1, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand1, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorArm, 0);

  return 0;
}

static int do_roverMotorsStop(RoverOutput* _rover)
{
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft1, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft2, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight1, 0);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand1, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motorArm, 0);

  return 0;
}


static int do_roverMotorsPower(RoverOutput* _rover,
                               int _ctrlChasisLeft, int _ctrlChasisRight,
                               int _ctrlHand, int _ctrlArm)
{
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft1,  _ctrlChasisLeft);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisLeft2,  _ctrlChasisLeft);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight1, _ctrlChasisRight);
  do_roverMotorMspSetPower(_rover, &_rover->m_motorChasisRight2, _ctrlChasisRight);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand1, _ctrlHand);
  do_roverMotorSetPower(_rover, &_rover->m_motorHand2, _ctrlHand);
  do_roverMotorSetPower(_rover, &_rover->m_motorArm, _ctrlArm);

  return 0;
}




int roverOutputInit(bool _verbose)
{
  (void)_verbose;

  return 0;
}

int roverOutputFini()
{
  return 0;
}

int roverOutputOpen(RoverOutput* _rover, const RoverConfig* _config)
{
  int res = 0;

  if (_rover == NULL)
    return EINVAL;
  if (_rover->m_opened)
    return EALREADY;

  if ((res = do_roverOpen(_rover, _config)) != 0)
    return res;

  _rover->m_opened = true;

  return 0;
}

int roverOutputClose(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return EALREADY;

  do_roverClose(_rover);

  _rover->m_opened = false;

  return 0;
}

int roverOutputStart(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;

  do_roverMotorsStart(_rover);

  return 0;
}

int roverOutputStop(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;

  do_roverMotorsStop(_rover);

  return 0;
}

int roverOutputControl(RoverOutput* _rover,
                       const RoverControl* _roverControl)
{
  if (_rover == NULL || _roverControl == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;

  do_roverMotorsPower(_rover,
                      _roverControl->m_chasisLeftSpeed, _roverControl->m_chasisRightSpeed,
                      _roverControl->m_handSpeed,
                      _roverControl->m_armSpeed);

  return 0;
}
