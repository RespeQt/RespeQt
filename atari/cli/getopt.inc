
	org $2000
	
.proc test
	jsr printf
	.byte 'Start',155,0

.var argopt testopt .word
.var arginput testinput .word
.var argresult testresult .word
	
	getopt argopt arginput argresult
	bcs Error
	
	jsr printf
	.byte 'Success!',155,0
	rts

Error
	jsr printf
	.byte 'Error!',155,0
	rts
	
.endp

testopt .byte 'AS:X:'
testinput .byte '-A -S -X ABC DEF GHIJ'
testresult .ds 32

	icl 'printf.s'
	icl 'cli.inc'
	icl 'macros.inc'
	
	run test



	
;; Getopt
;; Returns parameterized input, returns with carry set, if parameters are empty or error.
.proc getopt ( .word option, input, result ) .var
	.var option .word
	.var input .word
	.var result .word

	; Test, whether one of the string is empty, then we quit with 
	strlen option
	bcs Error
	
	strlen input
	bcs Error
	
	; Now we parse the options
	parseOption option
	bcs Error


Error
	sec
	rts

.endp


;; Tests, how long the C type string is.
;; Returns the length in Y. Carry is set, if empty.
.proc getopt_strlen (.word str) .VAR
	.var str .word
	
	ldy #0
Loop
	lda str,y
	cmp #0
	beq Done
	iny
	bne Loop
	
Done
	cpy #0
	beq Empty
	clc
	rts

Empty
	sec
	rts
	
.endp
	
;; Parses the input like GNU GETOPT	
;; We assume, that option is not empty
.proc parseOption (.word option) .VAR
	.var option .word
	
	ldy #-1
	ldx #0
	
Loop
	; We test, whether we reached the last storage item. We cancel the options parsing
	cpx #.LEN[OptStorage] ; MADS calculates for us the storage size
	beq Error
	iny
	lda option, y
	cmp #0 ; We reached the end of the supplied options
	beq Done
	cmp #':'
	beq RecordMandatory
	cmp #'A'
	bcc Error
	cmp #'Z'+1
	bcc RecordLetter
	
RecordLetter
	inx
	inx
	sta OptStorage[0].letter,x
	jmp Loop

RecordMandatory
    ; We need to test, whether : comes after a letter
	lda OptStorage[0].letter,x
	cmp #0
	beq Error
	; Now set Bit 0
	ror OptStorage[0].flags,x
	sec
	rol OptStorage[0].flags,x
	inx
	inx
	jmp Loop	
	
Done
	clc
	rts
	
Error
	sec
	rts
	
.endp

.STRUCT Options
	.byte letter
	.byte flags
.ENDS

OptStorage dta Options[15] (0, 0)
