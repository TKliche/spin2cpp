PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_sum2
	rdlong	result1, arg1
	add	arg1, #4
	rdlong	_tmp002_, arg1
	add	result1, _tmp002_
_sum2_ret
	ret

result1
	long	0
COG_BSS_START
	fit	496
	org	COG_BSS_START
_tmp002_
	res	1
arg1
	res	1
arg2
	res	1
arg3
	res	1
arg4
	res	1
	fit	496
