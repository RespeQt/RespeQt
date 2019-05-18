stax	.macro
	sta :1
	stx :1+1
	.endm

	

ldax .macro	" "	; load a,x pair
	.if :1 = '#'
		lda #< :2
		ldx #> :2
	.else
		lda :2
		ldx :2+1
	.endif
	.endm



sbl 	.macro 	" " ; 32-bit subtraction
	.if .not [:0 = 4 .or :0 = 6]
		.error 'Wrong Number of Arguments!'
	.else
		.if :0 = 4 ; two arguments
			.if :3 = '#' ; immediate mode
				lda :2
				sec
				sbc #[:4 & $FF]
				sta :2
				lda :2+1
				sbc #[[:4 >> 8 ] & $FF]
				sta :2+1
				lda :2+2
				sbc #[[:4 >> 16] & $FF]
				sta :2+2
				lda :2+3
				sbc #[:4 >> 24]
				sta :2+3
			.else
				ldy #0
				ldx #3
				sec
?1
				lda :2,y
				sbc :4,y
				sta :2,y
				iny
				dex
				bpl ?1
			.endif
		.else ; three arguments
			.if :3 = '#' ; immediate mode
				lda :2
				sec
				sbc #[:4 & $FF]
				sta :6
				lda :2+1
				sbc #[[:4 >> 8 ] & $FF]
				sta :6+1
				lda :2+2
				sbc #[[:4 >> 16] & $FF]
				sta :6+2
				lda :2+3
				sbc #[:4 >> 24]
				sta :6+3
			.else
				ldy #0
				ldx #3
				sec
?1
				lda :2,y
				sbc :4,y
				sta :6,y
				iny
				dex
				bpl ?1
			.endif		
		.endif
	.endif
	.endm
	


	
adl 	.macro 	" " ; 32-bit addition
	.if .not [:0 = 4 .or :0 = 6]
		.error 'Wrong Number of Arguments!'
	.else
		.if :0 = 4 ; two arguments
			.if :3 = '#' ; immediate mode
				lda :2
				clc
				adc #[:4 & $FF]
				sta :2
				lda :2+1
				adc #[[:4 >> 8 ] & $FF]
				sta :2+1
				lda :2+2
				adc #[[:4 >> 16] & $FF]
				sta :2+2
				lda :2+3
				adc #[:4 >> 24]
				sta :2+3
			.else
				ldy #0
				ldx #3
				clc
?1
				lda :2,y
				adc :4,y
				sta :2,y
				iny
				dex
				bpl ?1
			.endif
		.else ; three arguments
			.if :3 = '#' ; immediate mode
				lda :2
				clc
				adc #[:4 & $FF]
				sta :6
				lda :2+1
				adc #[[:4 >> 8 ] & $FF]
				sta :6+1
				lda :2+2
				adc #[[:4 >> 16] & $FF]
				sta :6+2
				lda :2+3
				adc #[:4 >> 24]
				sta :6+3
			.else
				ldy #0
				ldx #3
				clc
?1
				lda :2,y
				adc :4,y
				sta :6,y
				iny
				dex
				bpl ?1
			.endif		
		.endif
	.endif
	.endm
	


	
mda	.macro
	ldy #3
?1
	lda :1,y
	sta :2,y
	dey
	bpl ?1
	.endm



eqd	.macro ; see if two 32 bit values are the same
	ldy #4
?1
	lda :1-1,y
	cmp :2-1,y
	bne ?2
	dey
	bne ?1
?2
	.endm

	
zdb	.macro ; make 32 bit value 0
	lda #0
	ldy #3
?1
	sta :1,y
	dey
	bpl ?1
	.endm

ndb	.macro ; see if 32 bit value is zero
	lda :1
	ora :1+1
	ora :1+2
	ora :1+3
	.endm
	
sfg	.macro ; set flag (bit 7)
	sec
	ror :1
	.endm
	
Fill	.macro
;.ifndef EXE
	.if :1 = 1
		opt f+
	.else
		opt f-
	.endif
;.endif
	.endm
	
NextBank	.macro
.if .def EXE
	opt f+
	org $A000
.else
;	.align LongJSR,$FF
;	LJSRCode
	.align $BFF4,$FF
	lda #BaseBank
	sta Bank
	rts
	.word Start
	.byte 0
	.byte $04
.if build = hardware.SIDE
	.word CartInit
.else
	.word $BFF4
.endif
	nmb
	opt f-
	org $A000
	opt f+
.endif
	.endm
	
	.if 0
	
ljsr	.macro ; do a long JSR if target label is in a different bank
This
.if .def EXE
	jsr :1
.else
;	.if [=:1] = [=This]
		jsr :1
;	.else
;		jsr :LongJSR
;		.byte [=:1] + BaseBank
;		.word :1
;	.endif
.endif
	.endm


ljsr2	.macro ; do a long JSR always
.if .def EXE
	jsr :1
.else
	jsr :LongJSR
	.byte [=:1] + BaseBank
	.word :1
.endif
	.endm	
	
	
ljsr3	.macro ; do a long JSR if target label is in a different bank
This
.if .def EXE
	jsr :1
.else
	.if [=:1] = [=This]
		jsr :1
	.else
		jsr :LongJSR
		.byte [=:1] + BaseBank
		.word :1
	.endif
.endif
	.endm

	.endif
	
	
LJSRCode	.macro
;	sta BankTemp2
;	sty BankTemp2+1

	pla		; get return address LSB
	sta BankTemp1
	clc
	adc #4		; skip args and add 1, since we'll be returning with RTI
	tay
	pla		; get MSB
	sta BankTemp1+1
	adc #0
	pha		; push MSB
	tya
	pha		; push LSB

	lda CurrBank
	pha		; push current bank
	lda #> [Return-1]
	pha
	lda #< [Return-1]
	pha
	ldy #3
	lda (BankTemp1),y	; target msb
	pha
	dey
	lda (BankTemp1),y	; target lsb
	pha
	dey
	lda (BankTemp1),y	; target bank
	sta BANK
	sta CurrBank
;	lda BankTemp2	; get saved registers
;	ldy BankTemp2+1
	php		; push dummy flags
	rti		; pull flags and jump to target

; Subroutine exit wrapper

Return
	php ; save flags
;	sta BankTemp2	; save A
;	sty BankTemp2+1	; save Y
	pla		; get flags
	sta BankTemp1	; save flags
	pla		; get saved bank
	sta BANK
	sta CurrBank
	lda BankTemp1	; get flags
	pha		; push them
;	lda BankTemp2
;	ldy BankTemp2+1
	rti		; restore flags and exit
	.endm
	
	
	
pad	.macro
	.rept :1
	.byte :2
	.endr
	.endm
	
//
//	Insert blank lines of colour Pf2
//	:1 = number of lines
//

PadLine	.macro
	.rept :1
	.byte $0F+$40
	.word BlankLineData
	.endr
	.endm

//
//	Insert filled lines of colour Pf1
//	:1 = number of lines
//

Pf1Line	.macro
	.rept :1
	.byte $0F+$40
	.word FilledLineData
	.endr
	.endm
	
	
//
//	Repeat mode
//	:1 = mode, :2 = lines
//

Repeat	.macro
	.rept :2
	.byte :1
	.endr
	.endm


//
//	Menu jump table
//

Target	.macro
	.word :1-1
	.byte :2
	.endm
	

