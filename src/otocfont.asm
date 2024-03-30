	org	32768

	jr	skptst

	ld	de,16384
	ld	b,8
fill:
	ld	a,(de)
	scf
	rla
	ld	(de),a
	djnz	fill
	ret

skptst:
	ld	hl,49152
	ld	de,15616

full:
	ld	a,1
	ld	(iter),a

char:	push	de
	ld	b,8

col:	ld	a,(iter)
	ld	c,a	
	ld	a,(de)
bitn:	rla
; or rra if you want to mirror
	dec	c
	jr	nz,bitn	

	jr	c,setbit

	ld	a,(hl)
	scf
	ccf
	rla
	ld	(hl),a
	
	jr	cyc
	

setbit: ld	a,(hl)
	scf
	rla
	ld	(hl),a

cyc:	inc	de
	djnz	col

	inc	hl

	pop	de

	ld	a,(iter)
	inc	a
	ld	(iter),a

	sub	9
	jr	nz,char

	inc	de
	inc	de
	inc	de
	inc	de
	inc	de
	inc	de
	inc	de
	inc	de


	ld	a,h
	cp	195
	jp	nz,full

	ret

iter:	db	0	
