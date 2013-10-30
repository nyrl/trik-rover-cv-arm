#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include "internal/module_driver.h"


static bool s_verbose = false;


static int do_driverCtrlSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  _driver->m_zeroX    = _config->m_zeroX;
  _driver->m_zeroY    = _config->m_zeroY;
  _driver->m_zeroMass = _config->m_zeroMass;

  return 0;
}

static int do_driverCtrlChasisSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft = 0;
  chasis->m_motorSpeedRight = 0;
  chasis->m_lastSpeed = 0;
  chasis->m_lastYaw = 0;

  return 0;
}

static int do_driverCtrlHandSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;
  hand->m_lastSpeed = 0;

  return 0;
}

static int do_driverCtrlArmSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlChasisStart(DriverOutput* _driver)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft = 0;
  chasis->m_motorSpeedRight = 0;
  chasis->m_lastSpeed = 0;
  chasis->m_lastYaw = 0;

  return 0;
}

static int do_driverCtrlHandStart(DriverOutput* _driver)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;
  hand->m_lastSpeed = 0;

  return 0;
}

static int do_driverCtrlArmStart(DriverOutput* _driver)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}


static int do_driverCtrlChasisGetControl(const DriverOutput* _driver,
                                         int* _ctrlChasisLeft, int* _ctrlChasisRight)
{
  const DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  if (_ctrlChasisLeft)
    *_ctrlChasisLeft = chasis->m_motorSpeedLeft;
  if (_ctrlChasisRight)
    *_ctrlChasisRight = chasis->m_motorSpeedRight;

  return 0;
}

static int do_driverCtrlHandGetControl(const DriverOutput* _driver,
                                       int* _ctrlHand)
{
  const DriverCtrlHand* hand = &_driver->m_ctrlHand;

  if (_ctrlHand)
    *_ctrlHand = hand->m_motorSpeed;

  return 0;
}

static int do_driverCtrlArmGetControl(const DriverOutput* _driver,
                                      int* _ctrlArm)
{
  const DriverCtrlArm* arm = &_driver->m_ctrlArm;

  if (_ctrlArm)
    *_ctrlArm = arm->m_motorSpeed;

  return 0;
}


static int do_driverCtrlChasisManual(DriverOutput* _driver, int _ctrlChasisLR, int _ctrlChasisFB)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft  = _ctrlChasisFB+_ctrlChasisLR;
  chasis->m_motorSpeedRight = _ctrlChasisFB-_ctrlChasisLR;

  return 0;
}

static int do_driverCtrlHandManual(DriverOutput* _driver, int _ctrlHand)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = _ctrlHand;

  return 0;
}

static int do_driverCtrlArmManual(DriverOutput* _driver, int _ctrlArm)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = _ctrlArm;

  return 0;
}

static int do_driverCtrlManual(DriverOutput* _driver, int _ctrlChasisLR, int _ctrlChasisFB)
{
  return 0;
}


static int do_driverCtrlChasisPreparing(DriverOutput* _driver)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft  = 0;
  chasis->m_motorSpeedRight = 0;

  return 0;
}

static int do_driverCtrlHandPreparing(DriverOutput* _driver)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlArmPreparing(DriverOutput* _driver)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = -100;

  return 0;
}

static int do_driverCtrlPreparing(DriverOutput* _driver)
{
  return 0;
}


static int do_driverCtrlChasisSearching(DriverOutput* _driver)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft  = 0;
  chasis->m_motorSpeedRight = 0;

  return 0;
}

static int do_driverCtrlHandSearching(DriverOutput* _driver)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlArmSearching(DriverOutput* _driver)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlSearching(DriverOutput* _driver)
{
  return 0;
}


static int sign(int _v)
{
  return (_v < 0) ? -1 : ((_v > 0) ? 0 : 1);
}

static int powerIntegral(int _power, int _lastPower, int _percent)
{
  if (sign(_power) == sign(_lastPower))
    _power += (_lastPower * _percent) / 100 + sign(_lastPower);

  return _power;
}

static int powerProportional(int _val, int _min, int _zero, int _max)
{
  int adj = _val - _zero;
  if (adj > 0)
  {
    if (_val >= _max)
      return +100;
    else
      return (+100*(_val-_zero)) / (_max-_zero); // _max!=_zero, otherwise (_val>=_max) matches
  }
  else if (adj < 0)
  {
    if (_val <= _min)
      return -100;
    else
      return (-100*(_val-_zero)) / (_min-_zero); // _min!=_zero, otherwise (_val<=_min) matches
  }
  else
    return 0;
}


static int do_driverCtrlChasisTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetMass)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;
  int yaw;
  int speed;
  int backSpeed;

  yaw = powerProportional(_targetX, -100, _driver->m_zeroX, 100);
  yaw = powerIntegral(yaw, chasis->m_lastYaw, 10);

  speed = powerProportional(_targetMass, 0, _driver->m_zeroMass, 10000); // back/forward based on ball size
  backSpeed = powerProportional(_targetY, -100, _driver->m_zeroY, 100); // move back/forward if ball leaves range
  if (backSpeed >= 20)
    speed += (backSpeed-20)*3;
  speed = powerIntegral(speed, chasis->m_lastSpeed, 10);

  chasis->m_lastYaw = yaw;
  chasis->m_lastSpeed = speed;

  int powerL = (-speed+yaw);
  if (powerL >= 30)
    powerL = 30+(powerL-30)/2;
  else if (powerL <= -30)
    powerL = -30+(powerL+30)/2;

  int powerR = (-speed-yaw);
  if (powerR >= 30)
    powerR = 30+(powerR-30)/2;
  else if (powerR <= -30)
    powerR = -30+(powerR+30)/2;

  chasis->m_motorSpeedLeft  = powerL;
  chasis->m_motorSpeedRight = powerR;

  return 0;
}

static int do_driverCtrlHandTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetMass)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;
  int power;

  power = powerProportional(_targetY, -100, _driver->m_zeroY, 100);
  power = powerIntegral(power, hand->m_lastSpeed, 10);

  hand->m_lastSpeed = power;

  hand->m_motorSpeed = power;

  return 0;
}

static int do_driverCtrlArmTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetMass)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetMass)
{
  int diffX = powerProportional(_targetX, -100, _driver->m_zeroX, 100);
  int diffY = powerProportional(_targetY, -100, _driver->m_zeroY, 100);
  int diffMass = powerProportional(_targetMass, 0, _driver->m_zeroMass, 10000);

  bool hasLock = (   abs(diffX) <= 10
                  && abs(diffY) <= 10
                  && abs(diffMass) <= 10);
  if (!hasLock)
    _driver->m_stateEntryTime.tv_sec = 0;

  if (s_verbose)
    fprintf(stderr, "%d %d %d %s (%d->%d %d->%d %d->%d)\n",
            diffX, diffY, diffMass,
            hasLock?" ### LOCK ### ":"",
            _targetX, _driver->m_zeroX,
            _targetY, _driver->m_zeroY,
            _targetMass, _driver->m_zeroMass);

  return 0;
}


static int do_driverCtrlChasisSqueezing(DriverOutput* _driver)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft  = 0;
  chasis->m_motorSpeedRight = 0;

  return 0;
}

static int do_driverCtrlHandSqueezing(DriverOutput* _driver)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlArmSqueezing(DriverOutput* _driver)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlSqueezing(DriverOutput* _driver)
{
#warning Check lock is stable

  return 0;
}


static int do_driverCtrlChasisReleasing(DriverOutput* _driver, int _ms)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  if (_ms < 2000)
  {
    chasis->m_motorSpeedLeft  = 0;
    chasis->m_motorSpeedRight = 0;
  }
  else if (_ms < 4000)
  {
    chasis->m_motorSpeedLeft  =  50;
    chasis->m_motorSpeedRight = -50;
  }
  else
  {
    chasis->m_motorSpeedLeft  = -50;
    chasis->m_motorSpeedRight =  50;
  }

  return 0;
}

static int do_driverCtrlHandReleasing(DriverOutput* _driver, int _ms)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  if (_ms < 4000)
    hand->m_motorSpeed =  100;
  else
    hand->m_motorSpeed = -100;

  return 0;
}

static int do_driverCtrlArmReleasing(DriverOutput* _driver, int _ms)
{
  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  if (_ms < 3000)
    arm->m_motorSpeed = 0;
  else
    arm->m_motorSpeed = -100;

  return 0;
}

static int do_driverCtrlReleasing(DriverOutput* _driver, int _ms)
{
  return 0;
}



int driverOutputInit(bool _verbose)
{
  s_verbose = _verbose;

  return 0;
}

int driverOutputFini()
{
  return 0;
}

int driverOutputOpen(DriverOutput* _driver, const DriverConfig* _config)
{
  int res = 0;

  if (_driver == NULL)
    return EINVAL;

  if ((res = do_driverCtrlSetup(_driver, _config)) != 0)
    return res;

  if ((res = do_driverCtrlChasisSetup(_driver, _config)) != 0)
    return res;

  if ((res = do_driverCtrlHandSetup(_driver, _config)) != 0)
    return res;

  if ((res = do_driverCtrlArmSetup(_driver, _config)) != 0)
    return res;

  return 0;
}

int driverOutputClose(DriverOutput* _driver)
{
  if (_driver == NULL)
    return EINVAL;

  return 0;
}

int driverOutputStart(DriverOutput* _driver)
{
  int res;

  if (_driver == NULL)
    return EINVAL;

  _driver->m_state = StatePreparing;
  _driver->m_stateEntryTime.tv_sec = 0;
  _driver->m_stateEntryTime.tv_nsec = 0;

  if ((res = do_driverCtrlChasisStart(_driver)) != 0)
    return res;
  if ((res = do_driverCtrlHandStart(_driver)) != 0)
    return res;
  if ((res = do_driverCtrlArmStart(_driver)) != 0)
    return res;

  return 0;
}

int driverOutputStop(DriverOutput* _driver)
{
  if (_driver == NULL)
    return EINVAL;

  return 0;
}

int driverOutputControlManual(DriverOutput* _driver, const DriverManualControl* _manualControl)
{
  if (_driver == NULL || _manualControl == NULL)
    return EINVAL;

  if (_driver->m_state != StateManual)
  {
    fprintf(stderr, "*** MANUAL MODE ***\n");
    _driver->m_state = StateManual;
    _driver->m_stateEntryTime.tv_sec = 0;
  }

#warning Migrate target location checks from here

  do_driverCtrlChasisManual(_driver, _manualControl->m_ctrlChasisLR, _manualControl->m_ctrlChasisFB);
  do_driverCtrlHandManual(_driver, _manualControl->m_ctrlHand);
  do_driverCtrlArmManual(_driver, _manualControl->m_ctrlArm);

  return 0;
}

int driverOutputControlAuto(DriverOutput* _driver, const TargetLocation* _targetLocation)
{
  if (_driver == NULL || _targetLocation == NULL)
    return EINVAL;

  if (_driver->m_stateEntryTime.tv_sec == 0)
    clock_gettime(CLOCK_MONOTONIC, &_driver->m_stateEntryTime);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  long long msPassed = (now.tv_sec  - _driver->m_stateEntryTime.tv_sec ) * 1000
                     + (now.tv_nsec - _driver->m_stateEntryTime.tv_nsec) / 1000000;

  switch (_driver->m_state)
  {
    case StateManual:
      do_driverCtrlChasisManual(_driver, 0, 0);
      do_driverCtrlHandManual(_driver, 0);
      do_driverCtrlArmManual(_driver, 0);
      do_driverCtrlManual(_driver, 0, 0);
      fprintf(stderr, "*** LEFT MANUAL MODE, PREPARING ***\n");
      _driver->m_state = StatePreparing;
      _driver->m_stateEntryTime.tv_sec = 0;
      break;

    case StatePreparing:
      do_driverCtrlChasisPreparing(_driver);
      do_driverCtrlHandPreparing(_driver);
      do_driverCtrlArmPreparing(_driver);
      do_driverCtrlPreparing(_driver);
      if (msPassed > 5000)
      {
        fprintf(stderr, "*** PREPARED, SEARCHING ***\n");
        _driver->m_state = StateSearching;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;

    case StateSearching:
      do_driverCtrlChasisSearching(_driver);
      do_driverCtrlHandSearching(_driver);
      do_driverCtrlArmSearching(_driver);
      do_driverCtrlSearching(_driver);
      if (_targetLocation->m_targetMass > 0)
      {
        fprintf(stderr, "*** FOUND TARGET ***\n");
        _driver->m_state = StateTracking;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;

    case StateTracking:
      do_driverCtrlChasisTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetMass);
      do_driverCtrlHandTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetMass);
      do_driverCtrlArmTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetMass);
      do_driverCtrlTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetMass);
      if (_targetLocation->m_targetMass <= 0)
      {
        fprintf(stderr, "*** LOST TARGET ***\n");
        _driver->m_state = StateSearching;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      else if (msPassed > 2000)
      {
        fprintf(stderr, "*** LOCKED TARGET ***\n");
        _driver->m_state = StateSqueezing;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;

    case StateSqueezing:
      do_driverCtrlChasisSqueezing(_driver);
      do_driverCtrlHandSqueezing(_driver);
      do_driverCtrlArmSqueezing(_driver);
      do_driverCtrlSqueezing(_driver);
      if (_targetLocation->m_targetMass <= 0)
      {
        fprintf(stderr, "*** LOCK FAILED ***\n");
        _driver->m_state = StatePreparing;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      else if (msPassed > 5000)
      {
        fprintf(stderr, "*** RELEASING TARGET ***\n");
        _driver->m_state = StateReleasing;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;

    case StateReleasing:
      do_driverCtrlChasisReleasing(_driver, msPassed);
      do_driverCtrlHandReleasing(_driver, msPassed);
      do_driverCtrlArmReleasing(_driver, msPassed);
      do_driverCtrlReleasing(_driver, msPassed);
      if (msPassed > 6000)
      {
        fprintf(stderr, "*** DONE ***\n");
        _driver->m_state = StatePreparing;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;
  }

  return 0;
}

int driverOutputGetRoverControl(const DriverOutput* _driver,
                                RoverControl* _roverControl)
{
  int res;

  if (_driver == NULL)
    return EINVAL;

  if ((res = do_driverCtrlChasisGetControl(_driver, &_roverControl->m_chasisLeftSpeed, &_roverControl->m_chasisRightSpeed)) != 0)
    return res;
  if ((res = do_driverCtrlHandGetControl(_driver, &_roverControl->m_handSpeed)) != 0)
    return res;
  if ((res = do_driverCtrlArmGetControl(_driver, &_roverControl->m_armSpeed)) != 0)
    return res;

  return 0;
}



