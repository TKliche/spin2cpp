PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_count
	mov	_var_01, #0
L__0001
	mov	OUTA, _var_01
	add	_var_01, #1
	jmp	#L__0001
_count_ret
	ret

result1
	long	0
COG_BSS_START
	fit	496
	org	COG_BSS_START
_var_01
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
