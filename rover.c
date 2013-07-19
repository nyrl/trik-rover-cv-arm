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

#include "internal/rover.h"



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

  _motor->m_powerBack    = _config->m_powerBack;
  _motor->m_powerNeutral = _config->m_powerNeutral;
  _motor->m_powerForward = _config->m_powerForward;

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

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor1, &_config->m_motor1)) != 0)
    return res;

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor2, &_config->m_motor2)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motor1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor3, &_config->m_motor3)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motor2);
    do_roverCloseMotor(_rover, &_rover->m_motor1);
    return res;
  }

  if ((res = do_roverOpenMotor(_rover, &_rover->m_motor4, &_config->m_motor4)) != 0)
  {
    do_roverCloseMotor(_rover, &_rover->m_motor3);
    do_roverCloseMotor(_rover, &_rover->m_motor2);
    do_roverCloseMotor(_rover, &_rover->m_motor1);
    return res;
  }

  return 0;
}

static int do_roverClose(RoverOutput* _rover)
{
  if (_rover == NULL)
    return EINVAL;

  do_roverCloseMotor(_rover, &_rover->m_motor4);
  do_roverCloseMotor(_rover, &_rover->m_motor3);
  do_roverCloseMotor(_rover, &_rover->m_motor2);
  do_roverCloseMotor(_rover, &_rover->m_motor1);

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
      pwm = _motor->m_powerBack;
    else
      pwm = _motor->m_powerNeutral + ((_motor->m_powerBack-_motor->m_powerNeutral)*(-_power))/100;
  }
  else
  {
    if (_power > 100)
      pwm = _motor->m_powerForward;
    else
      pwm = _motor->m_powerNeutral + ((_motor->m_powerForward-_motor->m_powerNeutral)*_power)/100;
  }

  if (dprintf(_motor->m_fd, "%d\n", pwm) < 0)
  {
    res = errno;
    fprintf(stderr, "dprintf(%d) failed: %d\n", pwm, res);
    return res;
  }
  fsync(_motor->m_fd);

  return 0;
}

int do_roverCtrlChasisSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  chasis->m_motorLeft  = &_rover->m_motor1;
  chasis->m_motorRight = &_rover->m_motor2;
  chasis->m_lastSpeed = 0;
  chasis->m_lastYaw = 0;
  chasis->m_zeroX = _config->m_zeroX;
  chasis->m_zeroMass = _config->m_zeroMass;

  return 0;
}

int do_roverCtrlHandSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  hand->m_motor      = &_rover->m_motor3;
  hand->m_lastSpeed = 0;
  hand->m_zeroY = _config->m_zeroY;

  return 0;
}

int do_roverCtrlArmSetup(RoverOutput* _rover, const RoverConfig* _config)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  arm->m_motor      = &_rover->m_motor4;

  return 0;
}

int do_roverCtrlChasisStart(RoverOutput* _rover)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  chasis->m_lastSpeed = 0;
  chasis->m_lastYaw = 0;

  return 0;
}

int do_roverCtrlHandStart(RoverOutput* _rover)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  hand->m_lastSpeed = 0;

  return 0;
}

int do_roverCtrlArmStart(RoverOutput* _rover)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

#warning TODO arm control
  (void)arm;

  return 0;
}


int do_roverCtrlChasisTarget(RoverOutput* _rover, int _targetX, int _targetY, int _targetMass)
{
  RoverControlChasis* chasis = &_rover->m_ctrlChasis;

  int yaw = (_targetX - chasis->m_zeroX);
  if (   (yaw < 0 && chasis->m_lastYaw < 0)
      || (yaw > 0 && chasis->m_lastYaw > 0))
    yaw += chasis->m_lastYaw/5;

  int speed = (chasis->m_zeroMass - _targetMass);
  if (speed < 0)
    speed /= 100;
  else
    speed /= 50;

  if (   (speed < 0 && chasis->m_lastSpeed < 0)
      || (speed > 0 && chasis->m_lastSpeed > 0))
  {
    int speedAdj = chasis->m_lastSpeed/10;
    if (speedAdj < -10)
      speed += -10;
    else if (chasis->m_lastSpeed/10 > 10)
      speed += 10;
    else
      speed += speedAdj;
  }

  chasis->m_lastYaw = yaw;
  chasis->m_lastSpeed = speed;

  do_roverMotorSetPower(_rover, chasis->m_motorLeft, speed+yaw);
  do_roverMotorSetPower(_rover, chasis->m_motorRight, speed-yaw);

  return 0;
}

int do_roverCtrlHandTarget(RoverOutput* _rover, int _targetX, int _targetY, int _targetMass)
{
  RoverControlHand* hand = &_rover->m_ctrlHand;

  int speed = -(_targetY - hand->m_zeroY);
  if (   (speed < 0 && hand->m_lastSpeed < 0)
      || (speed > 0 && hand->m_lastSpeed > 0))
    speed += hand->m_lastSpeed/5;

  hand->m_lastSpeed = speed;

  do_roverMotorSetPower(_rover, hand->m_motor, speed);

  return 0;
}

int do_roverCtrlArmTarget(RoverOutput* _rover, int _targetX, int _targetY, int _targetMass)
{
  RoverControlArm* arm = &_rover->m_ctrlArm;

  do_roverMotorSetPower(_rover, arm->m_motor, 0);

#warning TODO arm control

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

  do_roverMotorSetPower(_rover, &_rover->m_motor1, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor3, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor4, 0);

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

  do_roverMotorSetPower(_rover, &_rover->m_motor1, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor2, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor3, 0);
  do_roverMotorSetPower(_rover, &_rover->m_motor4, 0);

  return 0;
}

int roverOutputControl(RoverOutput* _rover, int _targetX, int _targetY, int _targetMass)
{
  if (_rover == NULL)
    return EINVAL;
  if (!_rover->m_opened)
    return ENOTCONN;


#warning TODO state machine: prepare (unsqueeze arm) and woof, run on target, throw out and woof
  do_roverCtrlChasisTarget(_rover, _targetX, _targetY, _targetMass);
  do_roverCtrlHandTarget(_rover, _targetX, _targetY, _targetMass);
  do_roverCtrlArmTarget(_rover, _targetX, _targetY, _targetMass);


  return 0;
}
