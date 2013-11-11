#include "config.h"
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
  _driver->m_zeroSize = _config->m_zeroSize;

  return 0;
}

static int do_driverCtrlChasisSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  (void)_config;

  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft = 0;
  chasis->m_motorSpeedRight = 0;

  return 0;
}

static int do_driverCtrlHandSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  (void)_config;

  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlArmSetup(DriverOutput* _driver, const DriverConfig* _config)
{
  (void)_config;

  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlChasisStart(DriverOutput* _driver)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;

  chasis->m_motorSpeedLeft = 0;
  chasis->m_motorSpeedRight = 0;

  return 0;
}

static int do_driverCtrlHandStart(DriverOutput* _driver)
{
  DriverCtrlHand* hand = &_driver->m_ctrlHand;

  hand->m_motorSpeed = 0;

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
  (void)_driver;
  (void)_ctrlChasisLR;
  (void)_ctrlChasisFB;

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
  (void)_driver;

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
  (void)_driver;

  return 0;
}


static int sign(int _v)
{
  return (_v < 0) ? -1 : ((_v > 0) ? 1 : 0);
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

static int powerDelayed(int _power, int _current_power, int _delay)
{
  if (sign(_power) != sign(_current_power))
    _current_power = 0;

  if (abs(_power) < abs(_current_power))
    return _power;

  return _current_power + (_power-_current_power)/_delay + sign(_power);
}


static int do_driverCtrlChasisTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetSize)
{
  DriverCtrlChasis* chasis = &_driver->m_ctrlChasis;
  int yaw;
  int speed;
  int backFactor;

  yaw = powerProportional(_targetX, -100, _driver->m_zeroX, 100);

  speed = -powerProportional(_targetSize, 0, _driver->m_zeroSize, 100); // back/forward based on ball size
  backFactor = powerProportional(_targetY, -100, _driver->m_zeroY, 100); // move back if ball is too low
  if (backFactor >= 50)
    speed -= (50-20)*2 + (backFactor-50)*4;
  else if (backFactor >= 20)
    speed -= (backFactor-20)*2;

  int powerL = speed+yaw;
  int powerR = speed-yaw;

  powerL = powerDelayed(powerL, chasis->m_motorSpeedLeft,  20);
  powerR = powerDelayed(powerR, chasis->m_motorSpeedRight, 20);

  chasis->m_motorSpeedLeft  = powerL;
  chasis->m_motorSpeedRight = powerR;

  return 0;
}

static int do_driverCtrlHandTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetSize)
{
  (void)_targetX;
  (void)_targetSize;

  DriverCtrlHand* hand = &_driver->m_ctrlHand;
  int power;

  power = -powerProportional(_targetY, -100, _driver->m_zeroY, 100);

  power = powerDelayed(power, hand->m_motorSpeed, 20);

  hand->m_motorSpeed = power;

  return 0;
}

static int do_driverCtrlArmTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetSize)
{
  (void)_targetX;
  (void)_targetY;
  (void)_targetSize;

  DriverCtrlArm* arm = &_driver->m_ctrlArm;

  arm->m_motorSpeed = 0;

  return 0;
}

static int do_driverCtrlTracking(DriverOutput* _driver, int _targetX, int _targetY, int _targetSize)
{
  int diffX = powerProportional(_targetX, -100, _driver->m_zeroX, 100);
  int diffY = powerProportional(_targetY, -100, _driver->m_zeroY, 100);
  int diffSize = powerProportional(_targetSize, 0, _driver->m_zeroSize, 100);

  bool hasLock = (   abs(diffX) <= 10
                  && abs(diffY) <= 10
                  && abs(diffSize) <= 10);
  if (!hasLock)
    _driver->m_stateEntryTime.tv_sec = 0;

  if (s_verbose)
    fprintf(stderr, "%d %d %d %s (%d->%d %d->%d %d->%d)\n",
            diffX, diffY, diffSize,
            hasLock?" ### LOCK ### ":"",
            _targetX, _driver->m_zeroX,
            _targetY, _driver->m_zeroY,
            _targetSize, _driver->m_zeroSize);

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

  arm->m_motorSpeed = 100;

  return 0;
}

static int do_driverCtrlSqueezing(DriverOutput* _driver)
{
  (void)_driver;
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
  else if (_ms < 3500)
  {
    chasis->m_motorSpeedLeft  =  50;
    chasis->m_motorSpeedRight = -50;
  }
  else if (_ms < 4500)
  {
    chasis->m_motorSpeedLeft  = 0;
    chasis->m_motorSpeedRight = 0;
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
  (void)_driver;
  (void)_ms;

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

#warning Relocate
int do_driverControlManual(DriverOutput* _driver, const DriverManualControl* _manualControl)
{
  if (_driver == NULL || _manualControl == NULL)
    return EINVAL;

  if (_driver->m_state != StateManual)
  {
    fprintf(stderr, "*** MANUAL MODE ***\n");
    _driver->m_state = StateManual;
    _driver->m_stateEntryTime.tv_sec = 0;
  }

  do_driverCtrlChasisManual(_driver, _manualControl->m_ctrlChasisLR, _manualControl->m_ctrlChasisFB);
  do_driverCtrlHandManual(_driver, _manualControl->m_ctrlHand);
  do_driverCtrlArmManual(_driver, _manualControl->m_ctrlArm);

  return 0;
}

#warning Relocate
int do_driverControlAuto(DriverOutput* _driver, const TargetLocation* _targetLocation)
{
  if (_driver == NULL || _targetLocation == NULL)
    return EINVAL;

  if (_driver->m_stateEntryTime.tv_sec == 0)
    clock_gettime(CLOCK_MONOTONIC, &_driver->m_stateEntryTime);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  long long msPassed = (now.tv_sec  - _driver->m_stateEntryTime.tv_sec ) * 1000
                     + (now.tv_nsec - _driver->m_stateEntryTime.tv_nsec) / 1000000;

#warning Migrate target location checks from here


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
      if (_targetLocation->m_targetSize > 0)
      {
        fprintf(stderr, "*** FOUND TARGET ***\n");
        _driver->m_state = StateTracking;
        _driver->m_stateEntryTime.tv_sec = 0;
      }
      break;

    case StateTracking:
      do_driverCtrlChasisTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetSize);
      do_driverCtrlHandTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetSize);
      do_driverCtrlArmTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetSize);
      do_driverCtrlTracking(_driver, _targetLocation->m_targetX, _targetLocation->m_targetY, _targetLocation->m_targetSize);
      if (_targetLocation->m_targetSize <= 0)
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
      if (_targetLocation->m_targetSize <= 0)
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

int driverOutputControl(DriverOutput* _driver,
                        const DriverManualControl* _manualControl,
                        const TargetLocation* _targetLocation)
{
  if (_driver == NULL)
    return EINVAL;

  if (_manualControl != NULL && _manualControl->m_manualMode)
    return do_driverControlManual(_driver, _manualControl);
  else
    return do_driverControlAuto(_driver, _targetLocation);
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



