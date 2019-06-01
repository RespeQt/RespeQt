;  cli.s - RespeQt client
;
;  Copyright (c) 2018 by Jonathan Halliday <fjc@atari8.co.uk>
;  Copyright (c) 2019 by Jochen Schäfer <jochen@joschs-robotics.de>
;
;  Assemble this program with MADS (http://mads.atari8.info)
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

	icl 'cli.inc'
	icl 'macros.inc'
	
	
	org $2000
	
.proc Start
	jsr About

; Parser part

; Main input loop
MainLoop
	jsr printf
	.byte '>',0
	jsr GetInput
	mva #0 BufOff
	jsr GetNextParam
	bcs MainLoop
	jsr MatchCommand
	bcc GotCmd
	jsr printf
	.byte 155,'Command not found!',155,0
	jmp MainLoop
	
GotCmd
	asl @
	tay
	lda #> [MainLoop-1]
	pha
	lda #< [MainLoop-1]
	pha
	lda JumpTable+1,y
	pha
	lda JumpTable,y
	pha
	rts ; Jump to matched command through stack
.endp	

//
//	Get next character from arg
//

.proc GetNextChar
	lda ArgBuf,y
	jsr ToUpper
	iny
	cmp #155
	rts
.endp


// The next three .proc have to stay together to work.

//
//	Get drive spec and allow '*'
//
// The following three proc have to stay together.
.proc GetDrvWC
	sec
	.byte $24 ; OPCODE for ZP BIT, jumps over following clc
.endp

//
//	Get drive spec, disallowing '*'
//

.proc GetDrv
	clc
.endp

//
//	Get next character from arg buffer and see if it's a valid drive ID
//	This includes 1-9 and A-O, and optionally '*'.
//	Server expects everything to have $30 subtracted. Wildcard character, meanwhile, ends up as 0.
//	This means when converting the resulting drive ID back to integer form, we add $30 regardless,
//

.proc GetDriveID
	ror Temp1	; wildcard flag by rotating carry into bit 7
	jsr GetNextChar
	bcs Abort
	bit Temp1 ; We test whether * is allowed by looking at bit 7
	bpl @+ ; Flag N is set by bit 7, so we don't allow * and jump to device detection
	cmp #'*'
	bne @+ ; Not *, so we jump to device detection
	lda #0	; * -> 0
	clc
	rts
@
	cmp #'9'+1
	bcs NotDigit
	sbc #'0'-1
	bcc Bad
	beq Bad		; disallow 0
	clc
	rts
NotDigit
	cmp #'A'	; handle A-I for 1-9
	bcc Bad
	cmp #'J'
	bcs @+
	sbc #'A'-11	; carry is clear, i.e. A contains 'A'-'I'
	clc
	rts
@
	cmp #'O'+1
	bcs Bad
	sbc #'0'-1	; carry is clear, so subtract one less
	bcc Bad
OK
	clc
	rts
Bad
	sec
Abort
	rts
	.endp
	

.proc MatchCommand
	mva #0 tmp1
Loop1
	lda tmp1
	asl
	tay
	lda Commands,y
	sta ptr1
	lda Commands+1,y
	sta ptr1+1
	ldy #0
Loop2
	lda (ptr1),y ; Look in the command text table
	beq Match ; If we reached the end of the command text, A = 0
	cmp ArgBuf,y ; Compare that with the argument buffer
	bne Next
	iny
	bne Loop2
Match
	; We set the argument flag bit 0 for following space
	sec
	ror ArgumentFlag	; set bit 7 of argument flag
	; Now we look at the next char in the argument buffer for a space
	lda ArgBuf,y
	beq MatchExit ; With string end (=0), we handle that like space.
	cmp #$20 ; Is this space?
	beq MatchExit ; Space, flag is already set, we don't care for the index
	lsr ArgumentFlag	; clear argument flag
	sty ArgumentIndex
	
MatchExit
	lda tmp1
	clc
	rts
Next
	inc tmp1
	lda tmp1
	cmp #[.len[JumpTable]/2] ; We let MADS calculate the number of commands.
	bcc Loop1
	sec
	rts
.endp



.proc GetNextFilename
	bcs Done

	ldx #$ff
@
	inx
	iny
	lda ArgBuf,x
	sta FileSpec-1,y
	bne @-
	clc

Done
	rts
.endp


.proc AddDefDrive
	ldy #$ff
@
	iny
	lda DriveSpec,y
	sta FileSpec,y
	bne @-
	rts
.endp



.proc GetNextParam
	mva #0 ArgBuf
	ldx BufOff
Loop1
	lda Inbuff,x
	beq Done
	cmp #$9B
	beq Done
	cmp #$20
	bne Start
	inx
	bne Loop1
Start
	ldy #0
Loop2
	lda InBuff,x
	beq EndArg
	cmp #$9B
	beq EndArg
	cmp #$20
	beq EndArg
	jsr ToUpper
	sta ArgBuf,y
	inx
	iny
	bne Loop2
EndArg
	stx BufOff
	lda #0
	sta ArgBuf,y
	clc
	rts
Done	; no more args
	sec
	rts
.endp




.proc GetInput
	ldx #0
	mva #5 iccom
	mwa #InBuff icbadr
	mwa #64 icblen
	jmp ciov
.endp




.proc GetCurrentPath
	jsr DoCIO
	.byte 1,48,0,0
	.word DriveSpec,InBuff
	rts
.endp


;; When matching commands, we already checked whether a space is trailing or not
;; When this routine returns Y points to the correct char in ArgBuf
.proc PrepareNextParam
; If the user typed the parameter without a space,
; we have to check before we fetch another parameter (A!=0, then no space)
	bit ArgumentFlag	; test bits 6 and 7 of argument flag
	bmi FetchParam	; branch if bit 7 set (N = 1)
	; Now we need to restore the Y to get the next char
	ldy ArgumentIndex
	rts
 
FetchParam
	; Trailing space, so we fetch the next param
	jsr GetNextParam
	ldy #0
	rts
.endp	

;
;	Get next byte arg (returned in a,y)
;

.proc GetNextByte
	ldy #0
	lda (ptr1),y
	inw ptr1
	rts
.endp

;
;	Get next word arg (returned in a,y)
;

.proc GetNextWord
	ldy #0
	lda (ptr1),y
	pha
	iny
	lda (ptr1),y
	pha
	adw ptr1 #2
	pla
	tay
	pla
	rts
.endp

//
//	Make drive ID
//	Pass drive number in A
//

.proc MakeDriveID
	clc
	adc #'0'	; everything gets $30 added
	cmp #':'
	bmi NotLetter
	adc #6
NotLetter
	rts
.endp

.proc About
	jsr printf
	.byte 'RespeQt Client Terminal ', 155
	.byte '(c)2018, FJC, (c)2019, JoSch',155
	.byte 155,0
	jsr printf
	.byte 'Commands:',155
	.byte ' DT: Show time/date',155
	.byte ' DM fname.ext:   Existing Image Mount',155
	.byte ' DN fname.ext.x: New Image Mount',155,0
	jsr printf
	.byte ' DU [d/*]: Unmount disk/all disks',155	
	.byte ' DS d d:  Swap Disks',155
	.byte ' DA [d/*]: Auto commit disk/all disks',155,155
	.byte ' CLS',155
	.byte ' ? [Command]: Show usage and help',155
	.byte ' X: Exit CLI',155,155
	.byte 0
	rts
.endp

.proc JumpTable
	.word Mount-1
	.word MountNew-1
	.word Umount-1
	.word DateTime-1
	.word Swap-1
	.word CLS-1
	.word Help-1
	.word Exit-1
	.word Toggle-1
.endp

Commands
	.word txtMOUNT
	.word txtMOUNTNEW
	.word txtUMOUNT
	.word txtDATETIME
	.word txtSWAP
	.word txtCLS
	.word txtHelp
	.word txtExit
	.word txtTOGGLE

txtDATETIME
	.byte 'DT',0
txtExit
	.byte 'X',0
txtUMOUNT
	.byte 'DU',0
txtMOUNT
	.byte 'DM',0
txtSWAP
	.byte 'DS',0
txtTOGGLE
	.byte 'DA',0
txtCLS
	.byte 'CLS',0
txtHelp
	.byte '?',0
txtMOUNTNEW
	.byte 'DN',0

CommandHelps
	.word helpMOUNT-1
	.word helpMOUNTNEW-1
	.word helpUMOUNT-1
	.word helpDATETIME-1
	.word helpSWAP-1
	.word helpCLS-1
	.word helpHelp-1
	.word helpExit-1
	.word helpTOGGLE-1

.proc helpMOUNT
	jsr printf 
	.byte 'DM fname.ext',155
	.byte 'Mount the image file in the next free slot',155,0
	rts
.endp

.proc helpMOUNTNEW
	jsr printf 
	.byte 'DN fname.ext.x',155
	.byte 'Create a image and mount it in the next free slot',155
	.byte 155
	.byte ' x: 1:SSSD 2:SSED 3:SSDD',155
	.byte '    4:DSDD 5:DDHD 6:QDHD',155,0
	rts
.endp

.proc helpUMOUNT
	jsr printf
	.byte 'DU [d/*]', 155
	.byte 'Unmount named slot or all slots',155,0
	rts
.endp


.proc helpDATETIME
	jsr printf
	.byte 'DT',155
	.byte 'Show time and date from the RespeQt host',155,0
	rts
.endp

.proc helpSWAP
	jsr printf
	.byte 'DS d d',155
	.byte 'Swap both slots with each other',155,0
	rts
.endp

.proc helpCLS
	jsr printf
	.byte 'CLS',155
	.byte 'Clear the screen',155,0
	rts
.endp

.proc helpHelp
	jmp About
.endp

.proc helpExit
	jsr printf
	.byte 'X',155
	.byte 'Exit this program',155,0
	rts
.endp

.proc helpTOGGLE
	jsr printf
	.byte 'DA [d/*]',155
	.byte 'Auto commit given slot or all slots',155,0
	rts
.endp


; Commands
.proc Exit
	pla ; We still have the main loop address on the stack, so remove it.
	pla
	rts
.endp

.proc Mount
; We look for two following parameters with or without spaces
; If we get them, we calculate the drive id.
	jsr PrepareNextParam
	jsr GetNextFilename
	
	ldy #$FF	
CopyLoop ; Now copy the FileSpec into IOBuf for DCB
	iny
	mva FileSpec,y IOBuf,y
	bne CopyLoop
	
	lda #DCB.MOUNT
	jsr SetupDCB
	mva #0 DAUX1
	sta DAUX2
	jsr SIOV
	bmi Error
	
	; We fetch the last mounted slot
	lda #DCB.GetDrvNum
	jsr SetupDCB
	jsr SIOV
	bmi Error
	
	lda IOBuf
	clc
	adc #1
	jsr MakeDriveID
	sta IOBuf

	jsr printf
	.byte 'Image %s was successfully mounted on slot %c',155,0
	.word FileSpec, IOBuf

	clc
	rts
	
Error	
	jsr Printf
	.byte 'Error while mounting!',155,0

	sec
	rts
.endp

.proc MountNew
	jsr NotImplemented
	rts
.endp

.proc Umount
	jsr PrepareNextParam
	jsr GetDrvWC
	bcs Error

	sta temp1
	lda #DCB.Unmount
	jsr SetupDCB
	mva temp1 DAUX2
	jsr SIOV
	bmi Error
	
	jsr Printf
	.byte 'Unmounted',155,0
	clc
	rts

Error
	jsr Printf
	.byte 'Error unmounting slot(s)!',155,0	
	sec
	rts
.endp


.proc Help
	jsr PrepareNextParam
	; we now use ArgPointer to point into ArgBuf by ArgumentIndex, 
	; because indirect indexed access is only allowed with Y.
	; The comparison below has both to be indirect.
	sty ArgumentIndex
	clc
		
	mva #0 tmp1
Loop1
	lda #<ArgBuf
	adc ArgumentIndex
	sta ptr2
	lda #>ArgBuf
	adc #0
	sta ptr2+1

	lda tmp1
	asl
	tay
	lda Commands,y
	sta ptr1
	lda Commands+1,y
	sta ptr1+1
	ldy #0
Loop2
	lda (ptr1),y ; Look in the command text table
	beq Match ; A=0, we reached the end of the command, we ignore the rest of input.
	cmp (ptr2),y ; Compare that with the argument buffer
	bne Next
	iny
	bne Loop2
	
Match
	; We load the command search index, double it, then index into the help jump table.
	lda tmp1
	asl
	tay
	lda CommandHelps+1,y
	pha
	lda CommandHelps,y
	pha
	rts
	
Next
	inc tmp1
	lda tmp1
	cmp #[.len[JumpTable]/2] ; We let MADS calculate the number of commands.
	bcc Loop1
	
	jsr printf
	.byte 'No help for keyword found', 155,0
	rts
.endp

.proc Swap
; We look for two following parameters with or without spaces
; If we get them, we calculate the drive id.
	jsr PrepareNextParam
	jsr GetDrv
	bcs Error
	sta tmp1
	jsr PrepareNextParam
	jsr GetDrv
	bcs Error
	sta tmp2
	lda #DCB.Swap
	jsr SetupDCB
	mva tmp1 DAUX1
	mva tmp2 DAUX2
	jsr SIOV
	bpl OK	
	
Error
	jsr Printf
	.byte 'Error swapping disk images!',155,0
	sec
	rts
	
OK
	lda DAUX1
	jsr MakeDriveID
	sta tmp1
	lda DAUX2
	jsr MakeDriveID
	sta tmp2
	jsr Printf
	.byte 'Swapped drive %c with drive %c.',155,0
	.word tmp1, tmp2
	clc
	rts
.endp


.proc Toggle
	jsr PrepareNextParam
; Now calculate which drive id was given
	jsr GetDrvWC
	bcs Error
	sta tmp1
	lda #DCB.AutoToggle
	jsr SetupDCB
	mva tmp1 DAUX1
	jsr SIOV
	bpl OK
Error
	jsr Printf
	.byte 'Error toggling auto-commit!',155,0
	sec
	rts
OK
	lda DAUX1
	cmp #0
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 'Auto-commit toggled on drive %c',155,0
	.word DriveID1
	clc
	rts	
AllDrives
	jsr Printf
	.byte 'Auto-commit toggled on all drives',155,0
	clc
	rts
.endp


.proc DateTime
	lda #DCB.GetTD
	jsr SetupDCB
	jsr SIOV
	bpl DateTimeOK
	jsr Printf
	.byte 'Error getting date/time!',155,0
	sec
	rts
DateTimeOK
	jsr Printf
	.byte 'Current date and time is %b-%b-%b %b:%b:%b',155,0
	.word IOBuf+2, IOBuf+1, IOBuf, IOBuf+3, IOBuf+4, IOBuf+5
	clc
	rts
.endp

.proc NotImplemented
	jsr printf
	.byte 'Feature not implemented yet',155,0
	clc
	rts
.endp




.proc CLS
	jsr printf
	.byte 125,0
	rts
.endp




; IO and misc helpers
.proc CloseChannel
	tya
	pha
	jsr DoCIO
	.byte 1,12,0,0
	.word 0,0
	pla
	tay
	rts
.endp



.proc Errors
	jsr CloseChannel
	sty tmp2
	cpy #150
	beq BadPath
	cpy #170
	beq BadFile
	cpy #128
	beq BreakAbort
	jsr printf
	.byte 155,'System error: $%x!',155,0
	.word tmp2
	rts
BadPath
	jsr printf
	.byte 155,'Path not found!',155,0
	rts
BadFile
	jsr printf
	.byte 155,'File not found!',155,0
	rts
BreakAbort
	jsr printf
	.byte 155,'Break Abort!',155,0
	rts
.endp





//
//	Set up DCB for SIO call
//	Pass DCB number in A
//
	
.proc SetUpDCB
	tay
	ldx DCBIndex,y	; we could multiply by 10 then add nine, but a table is easier
	ldy #9
Loop
	lda DCBTable,x
	sta DComnd,y
	dex
	dey
	bpl Loop
	mva #rcldev DDevic	; ddevic and dunit are common to all
	mva #$01 DUnit
	rts
DCBIndex
	.byte 9,19,29,39,49,59,69
.endp

DCBTable

DCBGetTD
	.byte Cmd.GetTD		; command
	.byte $40		; dstats
	.word IOBuf		; buffer address
	.byte $06,$00		; timeout, dunuse
	.word $06		; buffer length
	.byte $00,$00		; aux1, aux2
DCBSwap
	.byte Cmd.Swap
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBUnmount
	.byte Cmd.Unmount
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBMount
	.byte Cmd.Mount
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0C
	.byte $00,$00
DCBCreateAndMount
	.byte Cmd.CreateAndMount
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0E
	.byte $00,$00
DCBAutoToggle
	.byte Cmd.AutoToggle
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00
DCBGetDrvNum
	.byte Cmd.Mount
	.byte $40
	.word IOBuf
	.byte $06,$00
	.word $01
	.byte $01,$00




;
;	DoCIO channel, command, icaux1, icaux2, address, length
;

.proc DoCIO
	pla
	clc
	adc #1
	sta ptr1
	pla
	adc #0
	sta ptr1+1
	
	jsr GetNextByte
	asl @
	asl @
	asl @
	asl @
	tax
	
	jsr GetNextByte
	sta iccom,x
	
	jsr GetNextByte
	sta icaux1,x
	
	jsr GetNextByte
	sta icaux2,x
	
	jsr GetNextWord
	sta icbadr,x
	tya
	sta icbadr+1,x
	
	jsr GetNextWord
	sta icblen,x
	tya
	sta icblen+1,x
		
	jsr ciov
	jmp (ptr1)
.endp


	icl 'printf.s'

; Globals

DriveSpec
	.byte 'D1:',0
	
AllSpec
	.byte '*.*',0




BufOff
	.ds 1
ArgBuf
	.ds 64
Filename
	.ds 64
InBuff
	.ds 64
FileSpec
	.ds 64

ArgumentFlag
	.byte 0 ; Bit 0 = Is a space after command? 0 = No, 1 = Yes
ArgumentIndex
	.byte 0 ; We store the y index for using when no space was given.
CreateFlag
	.byte 0
Slot
	.byte 0
Drive
	.byte 0
DriveID1
	.byte 0
DriveID2
	.byte 0

	
IOBuf
	.ds 256

	run Start
	

