PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_onetwo
	mov	result1, #1
	mov	result2, #2
_onetwo_ret
	ret

result1
	long	0
result2
	long	0
COG_BSS_START
	fit	496
	org	COG_BSS_START
arg1
	res	1
arg2
	res	1
arg3
	res	1
arg4
	res	1
	fit	496
