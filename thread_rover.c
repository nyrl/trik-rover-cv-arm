#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <sys/select.h>

#include "internal/thread_rover.h"
#include "internal/runtime.h"
#include "internal/module_rover.h"
#include "internal/module_driver.h"




static int threadRoverSelectLoop(Runtime* _runtime, RoverOutput* _rover, DriverOutput* _driver)
{
  int res;
  static const struct timespec s_selectTimeout = { .tv_sec=0, .tv_nsec=10*1000*1000 }; // 10ms loop

  if (_runtime == NULL || _rover == NULL || _driver == NULL)
    return EINVAL;

  if ((res = pselect(0, NULL, NULL, NULL, &s_selectTimeout, NULL)) < 0)
  {
    res = errno;
    fprintf(stderr, "pselect() failed: %d\n", res);
    return res;
  }

  DriverManualControl manualControl;
  if ((res = runtimeGetDriverManualControl(_runtime, &manualControl)) != 0)
  {
    fprintf(stderr, "runtimeGetDriverManualControl() failed: %d\n", res);
    return res;
  }

  TargetLocation targetLocation;
  if ((res = runtimeGetTargetLocation(_runtime, &targetLocation)) != 0)
  {
    fprintf(stderr, "runtimeGetTargetLocation() failed: %d\n", res);
    return res;
  }

  if ((res = driverOutputControl(_driver, &manualControl, &targetLocation)) != 0)
  {
    fprintf(stderr, "driverOutputControl() failed: %d\n", res);
    return res;
  }

  RoverControl roverControl;
  if ((res = driverOutputGetRoverControl(_driver, &roverControl)) != 0)
  {
    fprintf(stderr, "driverOutputGetRoverControl() failed: %d\n", res);
    return res;
  }

  if ((res = roverOutputControl(_rover, &roverControl)) != 0)
  {
    fprintf(stderr, "roverOutputControl() failed: %d\n", res);
    return res;
  }

  return 0;
}




void* threadRover(void* _arg)
{
  int res = 0;
  int exit_code = 0;
  Runtime* runtime = (Runtime*)_arg;
  RoverOutput* rover;
  DriverOutput* driver;

  if (runtime == NULL)
  {
    exit_code = EINVAL;
    goto exit;
  }

  if (   (rover  = runtimeModRoverOutput(runtime))  == NULL
      || (driver = runtimeModDriverOutput(runtime)) == NULL)
  {
    exit_code = EINVAL;
    goto exit;
  }


  if ((res = roverOutputOpen(rover, runtimeCfgRoverOutput(runtime))) != 0)
  {
    fprintf(stderr, "roverOutputOpen() failed: %d\n", res);
    exit_code = res;
    goto exit;
  }

  if ((res = driverOutputOpen(driver, runtimeCfgDriverOutput(runtime))) != 0)
  {
    fprintf(stderr, "driverOutputOpen() failed: %d\n", res);
    exit_code = res;
    goto exit_rover_close;
  }


  if ((res = roverOutputStart(rover)) != 0)
  {
    fprintf(stderr, "roverOutputStart() failed: %d\n", res);
    exit_code = res;
    goto exit_driver_close;
  }

  if ((res = driverOutputStart(driver)) != 0)
  {
    fprintf(stderr, "driverOutputStart() failed: %d\n", res);
    exit_code = res;
    goto exit_rover_stop;
  }


  printf("Entering rover thread loop\n");
  while (!runtimeGetTerminate(runtime))
  {
    if ((res = threadRoverSelectLoop(runtime, rover, driver)) != 0)
    {
      fprintf(stderr, "threadRoverSelectLoop() failed: %d\n", res);
      exit_code = res;
      goto exit_driver_stop;
    }
  }
  printf("Left rover thread loop\n");


exit_driver_stop:
  if ((res = driverOutputStop(driver)) != 0)
    fprintf(stderr, "driverOutputStop() failed: %d\n", res);

exit_rover_stop:
  if ((res = roverOutputStop(rover)) != 0)
    fprintf(stderr, "roverOutputStop() failed: %d\n", res);


exit_driver_close:
  if ((res = driverOutputClose(driver)) != 0)
    fprintf(stderr, "driverOutputClose() failed: %d\n", res);

exit_rover_close:
  if ((res = roverOutputClose(rover)) != 0)
    fprintf(stderr, "roverOutputClose() failed: %d\n", res);


exit:
  runtimeSetTerminate(runtime);
  return (void*)exit_code;
}




