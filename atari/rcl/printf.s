;  printf.s - RespeQt printf library
;
;  Copyright (c) 2016 by Jonathan Halliday <fjc@atari8.co.uk>
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;

//
//	Tiny Printf
//	Temp1: String pointer
//	Temp3: Arg pointer
//	ArgIndex: index into args (offset from start of string)
//	StringIndex: index into string
//


//
//	Get pointer to next arg in Temp3
//

	.proc GetNextArg
	ldy ArgIndex
	lda (Temp1),y
	sta Temp3
	iny
	lda (Temp1),y
	sta Temp3+1
	iny
	sty ArgIndex
	ldy #0		; leave 0 in Y
	rts
	.endp


//
//	Create arg pointer
//

	.proc GetArgPointer
	jsr StrLen	; step past string argument
	iny		; skip trailing NUL
	sty ArgIndex
	rts
	.endp




//
//	In-line entry point
//

	.proc Printf
	pla		; get address of in-line string
	clc
	adc #1
	tay		; save LSB	
	pla		; get MSB
	adc #0
	tax		; put MSB in X
	tya		; put LSB in A
	jsr PrintfAX	; Main expects string address in A,X
Return
	lda Temp1
	clc
	adc ArgIndex
	sta Temp1
	bcc @+
	inc Temp1+1
@
	jmp (Temp1)
	.endp



//
//	Get string length
//

	.proc StrLen
	ldy #$FF
@
	iny
	lda (Temp1),y
	bne @-
	rts
	.endp
	

	
	




	.proc PrintfAX
	stax Temp1
NoAX
	lda #0
	sta StringIndex
	jsr GetArgPointer
Loop
	ldy StringIndex
	lda (Temp1),y
	beq Done
	cmp #'%'
	bne PrintChar
	inc StringIndex
	iny
	lda (Temp1),y
	beq Done
	cmp #'%'
	beq PrintChar
	jsr GetFieldWidth
	lda (Temp1),y
	ldx PFControlTab
FindIt
	cmp PFControlTab,x
	beq FoundIt
	dex
	bne FindIt
	beq NextChar
	
FoundIt
	lda #> [NextChar-1]
	pha
	lda #< [NextChar-1]
	pha
	dex
	txa
	asl
	tax
	lda PFControlAddr+1,x
	pha
	lda PFControlAddr,x
	pha
	rts
;

PFControlTab
	.byte 5
	.byte 'cpsbx'
PFControlAddr
	.word PfChar-1
	.word PfPtr-1
	.word PfString-1
	.word PfByte-1
	.word PfHex-1

PrintChar
	jsr PutChar
NextChar
	inc StringIndex
	bne Loop
Done
	mva #0 Temp2+1
	rts
	.endp



	.proc GetFieldWidth
	lda #0
	sta FieldWidth
	sta LeadingZeroFlag
	tax
Loop
	lda (Temp1),y
	cmp #'9'+1
	bcs Done
	sec
	sbc #'0'
	bcc Done	
	bne @+
	cpx #0		; if first field width digit is zero, set leading zero flag
	bne @+
	dec LeadingZeroFlag
@
	sta Temp3
	lda FieldWidth
	asl @		; fieldwidth * 2
	sta Temp3+1
	asl @
	asl @		; fieldwidth * 8
	clc
	adc Temp3+1	; (fieldwidth*2) + (fieldwidth*8)
	adc Temp3	; add in units
	sta FieldWidth
	inx
	iny
	bne Loop
Done
	sty StringIndex
	rts
	.endp
	
	

	.proc PfChar
	jsr GetNextArg	; leaves 0 in Y
	lda (Temp3),y
	ldx FieldWidth
	bne Loop
	inx
Loop
	jsr PutChar	; doesn't clobber registers
	dex
	bne Loop
	rts
	.endp



	.proc PfPtr
	jsr GetNextArg
	ldy #1
	lda (Temp3),y
	tax
	dey
	lda (Temp3),y
	sta Temp3
	stx Temp3+1
	jmp PFPrintStr
	.endp



	.proc PfString
	jsr GetNextArg
	jmp PFPrintStr
	.endp


	
//
//	Display Hex/BCD
//

	.proc PfHex
	jsr GetNextArg
	lda (Temp3),y
	pha
	lsr @
	lsr @
	lsr @
	lsr @
	bne NotZero
	bit LeadingZeroFlag
	bpl @+
NotZero
	tay
	lda HexTable,y
	jsr PutChar
@
	pla
	and #$0F
	tay
	lda HexTable,y
	jmp PutChar
HexTable
	.byte '0123456789ABDCEF'
	.endp




//
//	Display decimal byte
//

	.proc PfByte
	jsr GetNextArg
	lda (Temp3),y
	ldx #0
Loop1
	cmp #100
	bcc HundredsDone
	sbc #100
	inx
	bne Loop1
HundredsDone
	pha	; save remainder
	txa
	seq
	jsr PutDigit
	pla
	ldx #0
Loop
	cmp #10
	bcc TensDone
	sbc #10
	inx
	bne Loop
TensDone
	pha
	txa
	seq
	jsr PutDigit
	pla
PutDigit
	clc
	adc #'0'
	jmp PutChar
	.endp	
	
	
	
	

//
//	Print plain string in A,X
//
	
	.proc PutStrAX
	stax Temp3	; fall into PFPrintStr
	.endp


	.proc PFPrintStr
	ldy #0
@
	lda (Temp3),y
	beq Done
	jsr PutChar
	iny
	bne @-
Done
	rts
	.endp
	
	

	
//
//	Put character
//

	.proc PutChar
	sty Temp4
	stx Temp4+1
	pha
	lda #0
	tax
	sta icblen,x
	sta icblen+1,x
	mva #$0B iccom,x
	pla
	jsr ciov
	ldy Temp4
	ldx Temp4+1
	rts
	.endp

	
	
	.proc ToUpper
	cmp #'z'+1
	bcs NLow
	cmp #'a'
	bcc NLow
	sbc #32
NLow
	rts
	.endp
	
