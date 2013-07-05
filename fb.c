#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "internal/fb.h"



static int do_fbOutputOpen(FBOutput* _fb, const char* _path)
{
  int res;

  if (_fb == NULL || _path == NULL)
    return EINVAL;

  _fb->m_fd = open(_path, O_RDWR|O_SYNC, 0);
  if (_fb->m_fd < 0)
  {
    res = errno;
    fprintf(stderr, "open(%s) failed: %d\n", _path, res);
    _fb->m_fd = -1;
    return res;
  }

  return 0;
}

static int do_fbOutputClose(FBOutput* _fb)
{
  int res;
  if (_fb == NULL)
    return EINVAL;

  if (close(_fb->m_fd) != 0)
  {
    res = errno;
    fprintf(stderr, "close() failed: %d\n", res);
    return res;
  }

  return 0;
}




int fbOutputInit(bool _verbose)
{
  (void)_verbose;
  return 0;
}

int fbOutputFini()
{
  return 0;
}

int fbOutputOpen(FBOutput* _fb, const FBConfig* _config)
{
  int ret = 0;

  if (_fb == NULL)
    return EINVAL;
  if (_fb->m_fd != -1)
    return EALREADY;

  ret = do_fbOutputOpen(_fb, _config->m_path);
  if (ret != 0)
    goto exit;

  return 0;


 exit_close:
  do_fbOutputClose(_fb);
 exit:
  return ret;
}

int fbOutputClose(FBOutput* _fb)
{
  if (_fb == NULL)
    return EINVAL;
  if (_fb->m_fd == -1)
    return EALREADY;

  do_fbOutputClose(_fb);

  return 0;
}

int fbOutputStart(FBOutput* _fb)
{
  if (_fb == NULL)
    return EINVAL;
  if (_fb->m_fd == -1)
    return ENOTCONN;

  return 0;
}

int fbOutputStop(FBOutput* _fb)
{
  if (_fb == NULL)
    return EINVAL;
  if (_fb->m_fd == -1)
    return ENOTCONN;

  return 0;
}


