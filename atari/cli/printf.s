
;
;	Get Pointer to next argument
;

	.proc PFGetNextArg
	ldy #1
	lda (ptr4),y
	tax
	dey
	lda (ptr4),y
	stax ptr2
	adw ptr4 #2
	rts
	.endp
	

;
;	Get integer (number of bytes in A)
;

	.proc PFGetNum
	pha
	jsr PFGetNextArg
	lda #0
	sta n1+1
	pla
	tay
Loop1
	lda (ptr2),y
	sta n1,y
	dey
	bpl Loop1
	rts
	.endp
	


;
;	In-line entry point
;

	.proc Printf
	pla
	tay
	pla
	tax
	iny
	sne
	inx
	tya
	jsr PrintfAX
Return
	jmp (ptr4)
	.endp
	
	
	
	.proc PrintfAX
	stax ptr1
	jsr StrLen
	tya
	sec		; add 1 extra to skip string terminator
	adc ptr1
	sta ptr4
	lda #0
	adc ptr1+1
	sta ptr4+1
Loop
	ldy #0
	sty Length
	sty FWid

	lda (ptr1),y		; get a character
	bne NotEOS
	rts

NotEOS
	tax
	and #127
	cmp #'%'		; formatting?
	beq GotControlChar
	txa
	bne Printf1		; else print it
GotControlChar
	inw ptr1		; bump string pointer
	jsr PfGetFieldWidth
	ldy #0
	lda (ptr1),y 		; get control character
	jsr ToUpper
	ldx #6
FindIt
	cmp PFControlTab-1,x
	beq FoundIt
	dex
	bne FindIt
	beq Printf1
	
FoundIt
	lda #> [Printf2-1]
	pha
	lda #< [Printf2-1]
	pha
	dex
	txa
	asl @
	tax
	lda PFControlAddr+1,x
	pha
	lda PFControlAddr,x
	pha
	rts


PFControlTab
	.byte 'BCDSXP'
PFControlAddr
	.word PfByte-1
	.word PfChar-1
	.word PfDec-1
	.word PfString-1
	.word PfHex-1
	.word PfPtr-1
	
Printf1
	jsr PutChar
Printf2
	inc ptr1
	bne Loop
	inc ptr1+1
	bne Loop	; bump index
	.endp


;
;	Print character
;

	.proc PfChar
	jsr PFGetNextArg	; point to next arg
;	ldy #0
	lda (ptr2),y	; Y = 0
PFCharLoop
	jsr PutChar
	dec FWid
	bpl PFCharLoop
	rts
	.endp


;
;	String (%s)
;

	.proc PfString
	jsr PFGetNextArg
	jmp PFPrintStr
	.endp


;
;	Byte (%b)
;
	
	.proc PfByte
	lda #0
	.byte $2C
	.endp

;
;	Decimal integer (%d)
;
	
	.proc PfDec
	lda #1
	.endp
	
	

	.proc PFNum
	pha
	jsr PFGetNum
	pla
	asl @
	tax
	inx
	jsr PFCnDec
PrtNum
	mwa #TempBuf ptr2
	bne PFPrintStr.Ps2
	.endp


	.proc PfPtr
	jsr PfGetNextArg
	ldy #1
	lda (ptr2),y
	tax
	dey
	lda (ptr2),y
	sta ptr2
	stx ptr2+1
	.endp


	
	.proc PFPrintStr
	ldax ptr2
	jsr StrLen
	sta Length
Ps2
	lda FWid
	beq NoPad
	sec
	sbc Length
	bcc Abbreviate
	sta FWid
	beq NoPad
	bit LJFlag
	bmi NoPad
	jsr DoPad
NoPad
	ldy #0
PNumLp
	lda (ptr2),y
	beq PNumDn
	jsr PutChar
	iny
	bne PNumLp
PNumDn
	bit LJFlag
	bmi DoPad
	rts
	
Abbreviate
	eor #$FF	; turn negative result into positive number
;	clc
	adc #4		; carry is already clear
	sta Temp1	; number of characters to omit, including ellipsis
	lda FWid
	lsr @		; get mid point
	tax
	dex
	ldy #0
@
	lda (ptr2),y
	jsr PutChar
	iny
	dex
	bne @-
	lda #'.'
	ldx #3
@
	jsr PutChar
	dex
	bne @-
	tya
	clc
	adc Temp1	; skip omitted characters
	tay
@
	lda (ptr2),y
	beq Done
	jsr PutChar
	iny
	bne @-
Done
	rts
	.endp
	
	
	
	.proc DoPad
	ldy FWid
	beq Done
	lda #32
@
	jsr PutChar
	dey
	bne @-
Done
	rts
	.endp



;
;	Hex value (%x)
;
	
	.proc PfHex
	lda #0
	jsr PfGetNum	; returns n1 in A
;	lda n1
	pha
	lsr @
	lsr @
	lsr @
	lsr @
	jsr HexOut
	pla
	and #$0F
	.endp
	

	.proc HexOut
	tax
	lda HexTable,x
	jmp PutChar
	.endp
	



	.proc PfCnDec
	lsr Temp1	; leading zero flag
	lda #0
	sta Length
ValLoop
	ldy #0
SubN
	lda n1
	sec
	sbc VTable1,x
	pha
	lda n1+1
	sbc VTable2,x
	bcc LessThan
	sta n1+1
	pla
	sta n1
	iny
	bne SubN
LessThan
	pla
	tya
	jsr DigOut
	dex
	bpl ValLoop
	stx Temp1	; always print last zero (x = $FF)
	lda n1
	jsr DigOut
	lda #0
	sta TempBuf,y
	rts
	.endp
	



	.proc DigOut
DigOut
	bne NZero
	bit Temp1
	bpl Skip0
NZero
	sec
	ror Temp1
	clc
	adc #'0'
	ldy Length
	sta TempBuf,y
	iny
	sty Length
Skip0
	rts
	.endp
	


	
	.proc PfGetFieldWidth
	lsr Temp1+1
	lsr LJFlag
	lda (ptr1),y
	cmp #'-'
	bne @+
	ror LJFlag	; C = 1
	inw ptr1
	lda (ptr1),y
@
	cmp #'0'
	bne Digt2
	ror Temp1+1	; C = 1
Digt2
	jsr AscHex
	sta FWid
	rts
	.endp
	


	
	.proc AscHex
	ldy #0
	sty Temp1+1
Loop
	lda (ptr1),y
	sec
	sbc #'0'
	bcc Done
	cmp #10
	bcs Done
	sta Temp1
	asl Temp1+1		; * 2
	lda Temp1+1
	asl @
	asl @		; * 8
	clc
	adc Temp1+1
	adc Temp1	; add in digit
	sta Temp1+1
	inc ptr1	; bump arg pointer
	bne Loop
	inc ptr1+1
	bne Loop
Done
	lda Temp1+1		; return value
	rts
	.endp
	



;
;	Get length of string in A,X
;
	
	.proc StrLen
	sta ptr2
	stx ptr2+1
	ldy #255
StrLenLoop
	iny
	lda (ptr2),y
	bne StrLenLoop
	tya
	rts
	.endp
	
	
	
	.local PutChar
	sty tmp4
	pha
	lda #0
	tax
	sta icblen
	sta icblen+1
	lda #11
	sta iccom
	pla
	jsr ciov
	ldy tmp4
	rts
	.endl
	
	

.proc ToUpper
	cmp #'z'+1
	bcs @+
	cmp #'a'
	bcc @+
	sbc #32
@
	rts
.endp
   

HexTable
	.byte '0123456789ABCDEF'

//	8-bit = 1, 16-bit = 3, 24-bit = 6, 32-bit = 8

VTable1
	.byte $0a,$64,$e8,$10 ; ,$a0,$40,$80,$00,$00
VTable2
	.byte $00,$00,$03,$27 ; ,$86,$42,$96,$e1,$ca

LJFlag
	.byte 0
FWid
	.byte 0
Length
	.byte 0
N1
	.word 0
Temp1
	.word 0
TempBuf
	.ds 20
