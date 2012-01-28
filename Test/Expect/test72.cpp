#include <stdlib.h>
#include <propeller.h>
#include "test72.h"

#include <setjmp.h>
typedef struct { jmp_buf jmp; int32_t val; } AbortHook__;
AbortHook__ *abortChain__;

int32_t test72::Top(int32_t X)
{
  int32_t result = 0;
  return __extension__({ AbortHook__ *stack__ = abortChain__, here__; int32_t tmp__; abortChain__ = &here__; if (setjmp(here__.jmp) == 0) tmp__ = Sub1(X); else tmp__ = here__.val; abortChain__ = stack__; tmp__; });
}

int32_t test72::Sub1(int32_t X)
{
  int32_t result = 0;
  return (2 * Sub2(X));
}

int32_t test72::Sub2(int32_t X)
{
  int32_t result = 0;
  if (X == 3) {
    if (!abortChain__) abort();
    abortChain__->val =  99;
    longjmp(abortChain__->jmp, 1);
  }
  return (X + 1);
}
