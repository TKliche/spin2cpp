/* {
   This is a test of comment parsing.
   The Spin compiler does not actually count parentheses if the comment is
   a 'doc' comment starting with '{{'; in that case it only searches for
   the closing comment symbol of two }'s in a row
 */
#include <propeller.h>
#include "test110.h"

void test110::Main(void)
{
  DIRA |= (1 << 0);
  OUTA |= (1 << 0);
}

/* 
  The end of the program
 */
