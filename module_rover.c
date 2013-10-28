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

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorMsp1, &_config->m_motorMsp1)) != 0)
  {
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorMsp2, &_config->m_motorMsp2)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorMsp3, &_config->m_motorMsp3)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  if ((res = do_roverOpenMotorMsp(_rover, &_rover->m_motorMsp4, &_config->m_motorMsp4)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp3);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor1, &_config->m_motor1)) != 0)
  {
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp4);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp3);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor2, &_config->m_motor2)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motor1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp4);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp3);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor3, &_config->m_motor3)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motor2);
    do_roverCloseMotor(_rover, &_rover->m_motor1);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp4);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp3);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
    do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);
    return res;
  }

  return 0;
}

static int do_roverClose(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;

  do_roverCloseMotor(_rover, &_rover->m_motor3);
  do_roverCloseMotor(_rover, &_rover->m_motor2);
  do_roverCloseMotor(_rover, &_rover->m_motor1);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp4);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp3);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp2);
  do_roverCloseMotorMsp(_rover, &_rover->m_motorMsp1);

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

static int do_roverCtrlChasisSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  chasis->m_motorLeft1  = &_rover->m_motorMsp1;
  chasis->m_motorLeft2  = &_rover->m_motorMsp2;
  chasis->m_motorRight1 = &_rover->m_motorMsp3;
  chasis->m_motorRight2 = &_rover->m_motorMsp4;

  return 0;
}

static int do_roverCtrlHandSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  hand->m_motor1 = &_rover->m_motor1;
  hand->m_motor2 = &_rover->m_motor2;

  return 0;
}

static int do_roverCtrlArmSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  arm->m_motor = &_rover->m_motor3;

  return 0;
}


static int do_roverCtrlChasisStart(RoverOutput* _rover)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft1, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft2, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight1, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight2, 0);

  return 0;
}

static int do_roverCtrlHandStart(RoverOutput* _rover)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  do_roverMotorSetPower(_rover, hand->m_motor1, 0);
  do_roverMotorSetPower(_rover, hand->m_motor2, 0);

  return 0;
}

static int do_roverCtrlArmStart(RoverOutput* _rover)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  do_roverMotorSetPower(_rover, arm->m_motor, 0);

  return 0;
}


static int do_roverCtrlChasisStop(RoverOutput* _rover)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft1, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft2, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight1, 0);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight2, 0);

  return 0;
}

static int do_roverCtrlHandStop(RoverOutput* _rover)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  do_roverMotorSetPower(_rover, hand->m_motor1, 0);
  do_roverMotorSetPower(_rover, hand->m_motor2, 0);

  return 0;
}

static int do_roverCtrlArmStop(RoverOutput* _rover)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  do_roverMotorSetPower(_rover, arm->m_motor, 0);

  return 0;
}


static int do_roverCtrlChasisPower(RoverOutput* _rover, int _ctrlChasisLeft, int _ctrlChasisRight)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft1, _ctrlChasisLeft);
  do_roverMotorMspSetPower(_rover, chasis->m_motorLeft2, _ctrlChasisLeft);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight1, _ctrlChasisRight);
  do_roverMotorMspSetPower(_rover, chasis->m_motorRight2, _ctrlChasisRight);

  return 0;
}

static int do_roverCtrlHandPower(RoverOutput* _rover, int _ctrlHand)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  do_roverMotorSetPower(_rover, hand->m_motor1, _ctrlHand);
  do_roverMotorSetPower(_rover, hand->m_motor2, _ctrlHand);

  return 0;
}

static int do_roverCtrlArmPower(RoverOutput* _rover, int _ctrlArm)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  do_roverMotorSetPower(_rover, arm->m_motor, _ctrlArm);

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

  do_roverCtrlChasisSetup(_rover, _config);
  do_roverCtrlHandSetup(_rover, _config);
  do_roverCtrlArmSetup(_rover, _config);

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

  do_roverCtrlChasisStart(_rover);
  do_roverCtrlHandStart(_rover);
  do_roverCtrlArmStart(_rover);

  return 0;
}

int roverOutputStop(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;

  do_roverCtrlArmStop(_rover);
  do_roverCtrlHandStop(_rover);
  do_roverCtrlChasisStop(_rover);

  return 0;
}

int roverOutputControl(RoverOutput* _rover,
                       int _ctrlChasisLeft, int _ctrlChasisRight,
                       int _ctrlHand,       int _ctrlArm)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;

  do_roverCtrlChasisPower(_rover, _ctrlChasisLeft, _ctrlChasisRight);
  do_roverCtrlHandPower(_rover, _ctrlHand);
  do_roverCtrlArmPower(_rover, _ctrlArm);

  return 0;
}
