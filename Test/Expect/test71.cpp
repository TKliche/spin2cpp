#include <propeller.h>
#include "test71.h"

int32_t test71::Blah(void)
{
  int32_t _parm__0000[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  _parm__0000[1] = 0;
  do {
    Foo((int32_t)(&_parm__0000[0]), (int32_t)(&_parm__0000[2 + _parm__0000[1]]));
    _parm__0000[1] = (_parm__0000[1] + 1);
  } while (_parm__0000[1] <= 7);
  return _parm__0000[0];
}

int32_t test71::Foo(int32_t M, int32_t N)
{
  int32_t result = 0;
  _OUTA = (_OUTA | ((1<<N)));
  return result;
}
