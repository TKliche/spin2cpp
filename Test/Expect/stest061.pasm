PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_times1
	mov	result1, arg1
_times1_ret
	ret

_times3
	mov	result1, arg1
	shl	result1, #1
	add	result1, arg1
_times3_ret
	ret

_times4
	shl	arg1, #2
	mov	result1, arg1
_times4_ret
	ret

_times5
	mov	result1, arg1
	shl	result1, #2
	add	result1, arg1
_times5_ret
	ret

_times6
	mov	result1, arg1
	shl	result1, #1
	add	result1, arg1
	shl	result1, #1
_times6_ret
	ret

_times7
	mov	result1, arg1
	shl	result1, #3
	sub	result1, arg1
_times7_ret
	ret

_times9
	mov	result1, arg1
	shl	result1, #3
	add	result1, arg1
_times9_ret
	ret

_times10
	mov	result1, arg1
	shl	result1, #2
	add	result1, arg1
	shl	result1, #1
_times10_ret
	ret

_times11
	mov	muldiva_, arg1
	mov	muldivb_, #11
	call	#multiply_
	mov	result1, muldiva_
_times11_ret
	ret

_times12
	mov	result1, arg1
	shl	result1, #1
	add	result1, arg1
	shl	result1, #2
_times12_ret
	ret

_times15
	mov	result1, arg1
	shl	result1, #4
	sub	result1, arg1
_times15_ret
	ret

multiply_
       mov    itmp2_, muldiva_
       xor    itmp2_, muldivb_
       abs    muldiva_, muldiva_
       abs    muldivb_, muldivb_
	mov    result1, #0
mul_lp_
	shr    muldivb_, #1 wc,wz
 if_c	add    result1, muldiva_
	shl    muldiva_, #1
 if_ne	jmp    #mul_lp_
       shr    itmp2_, #31 wz
 if_nz neg    result1, result1
	mov    muldiva_, result1
multiply__ret
	ret

itmp1_
	long	0
itmp2_
	long	0
result1
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
muldiva_
	res	1
muldivb_
	res	1
	fit	496
