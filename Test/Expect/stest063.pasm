PUB main
  coginit(0, @entry, 0)
DAT
	org	0
entry

_blinky
	mov	arg2, ptr__dat__
	mov	arg1, #8
	mov	arg3, #240
	call	#__system___coginit
_blinky_ret
	ret

__system___coginit
	and	arg3, imm_65532_
	shl	arg3, #16
	and	arg2, imm_65532_
	shl	arg2, #2
	or	arg3, arg2
	and	arg1, #15
	or	arg3, arg1
	coginit	arg3 wr
	mov	result1, arg3
__system___coginit_ret
	ret

imm_65532_
	long	65532
ptr__dat__
	long	@@@_dat_
result1
	long	0
COG_BSS_START
	fit	496
	long
_dat_
	byte	$f0, $19, $bc, $a0, $04, $18, $fc, $28, $01, $1a, $fc, $a0, $0c, $1a, $bc, $2c
	byte	$0d, $ec, $bf, $a0, $0d, $e8, $bf, $a0, $00, $1c, $fc, $08, $0e, $1e, $bc, $a0
	byte	$f1, $1f, $bc, $80, $0e, $1e, $bc, $f8, $0d, $e8, $bf, $6c, $09, $00, $7c, $5c
	byte	$00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
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
