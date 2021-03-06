#!/bin/sh

if [ "$1" != "" ]; then
  SPIN2CPP=$1
else
  SPIN2CPP=../build/spin2cpp
fi

PROG="$SPIN2CPP -I../Lib"
CC=propeller-elf-gcc
ok="ok"
endmsg=$ok

# ASM mode compilation tests
for i in stest*.spin
do
  j=`basename $i .spin`
  # NOTE: optimize 250 is all optimizations except
  #   remove_unused_funcs (0x01)
  #   remove_hub_bss (0x04)
  $PROG --asm --optimize 250 --noheader $i
  if  diff -ub Expect/$j.pasm $j.pasm
  then
      rm -f $j.pasm
      echo $j passed
  else
      echo $j failed
      endmsg="TEST FAILURES"
  fi
done

# clean up
if [ "x$endmsg" = "x$ok" ]
then
  rm -f *.pasm
else
  echo $endmsg
  exit 1
fi
