PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_update
	mov	_var_02, #0
L__0003
	cmps	_var_02, #10 wc,wz
 if_ae	jmp	#L__0004
	rdlong	_var_04, arg1
	add	_var_04, #1
	wrlong	_var_04, arg1
	add	_var_02, #1
	add	arg1, #4
	jmp	#L__0003
L__0004
_update_ret
	ret

result1
	long	0
COG_BSS_START
	fit	496
	org	COG_BSS_START
_var_02
	res	1
_var_04
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
