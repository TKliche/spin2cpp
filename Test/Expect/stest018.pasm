stest018_count
	mov	stest018_count_i_, #0
L_001_
	mov	OUTA, stest018_count_i_
	add	stest018_count_i_, #1
	cmp	stest018_count_i_, #4 wc,wz
  if_ne	jmp	#L_001_
stest018_count_ret
	ret

stest018_count_i_
	long	0