PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_getptr
	add	objptr, #16
	mov	result1, objptr
	sub	objptr, #16
_getptr_ret
	ret

objptr
	long	@@@objmem
result1
	long	0
COG_BSS_START
	fit	496
objmem
	res	8
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
