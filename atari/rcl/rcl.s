;  rlc.s - RespeQt client
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

	icl 'rcl.inc'

	org $2000

Start
	jsr printf
	.byte 'RespeQt Client v.1.0 (c)2016 FJC',155
	.byte 155,0
	
	lda $0700
	cmp #'S'
	beq IsSparta
	
	jsr printf
	.byte 'SpartaDOS v.2.5 or higher required!',155,0
	jmp Exit
	
IsSparta
	ldy #5
	ldx #2
@
	lda (comtab),y
	sta Crunch,x
	dey
	dex
	bne @-

MainLoop
	jsr Crunch	; get next arg
	bne GotArg
	bit ArgFlag	; if no args, disaplay usage
	bpl BadArg
	rts		; otherwise we're done
GotArg
	sec
	ror ArgFlag	; if we get at least one argument, don't display usage instructions
	ldy #comfnam+2	; step past drive spec
	jsr GetNextChar
	cmp #'T'	; TD command?
	beq CheckTDCmd
	cmp #'D'
	beq CheckDskCmd
	
BadArg
	jmp Usage
	
;	Check TD command
	
.proc CheckTDCmd		; parse TD command
	jsr GetNextChar
	ldx #2
@
	cmp TDCmdList,x
	beq Execute
	dex
	bpl @-
	bmi BadArg
	.endp

;	Check disk command

.proc CheckDskCmd
	jsr GetNextChar
	ldx #5
@
	cmp DskCmdList,x
	beq Found
	dex
	bpl @-
	bmi BadArg
Found
	txa
	clc
	adc #3	; skip TD commands
	tax
.endp

.proc Execute
	lda #> [Return-1]
	pha
	lda #< [Return-1]
	pha
	lda CmdAddressHi,x
	pha
	lda CmdAddressLo,x
	pha
	rts
.endp
	
	
.proc Return
	bcc MainLoop
	rts
.endp
	
	


TDCmdList
	.byte 'SOF'	; 0 = set td, 1 = set td and enable td line, 2 = set td and disable td line
DskCmdList
	.byte 'MNUSA'	; 0 = mount, 1 = create and mount, 2 = unmount, 3 = swap, 4 = toggle auto-commit

CmdAddressLo
	.byte <[GetTD-1], <[GetTDOn-1], <[GetTDOff-1], <[Mount-1], <[CreateAndMount-1], <[UnMount-1], <[Swap-1], <[ToggleAutoCommit-1]
CmdAddressHi
	.byte >[GetTD-1], >[GetTDOn-1], >[GetTDOff-1], >[Mount-1], >[CreateAndMount-1], >[UnMount-1], >[Swap-1], >[ToggleAutoCommit-1]
	
	
//
//	Get next character from arg
//

.proc GetNextChar
	iny
	lda (comtab),y
	jsr ToUpper
	cmp #155
	rts
.endp

//
//	Get drive spec and allow '*'
//

.proc GetDrvWC
	sec
	.byte $24
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
//	Server expects everything to have $30 subtracted, so while 1-9 ($31-39) end up as $01-$09,
//	J-O ($4A-4F, for drives 10 through 15) are encoded as $1A-$1F and then have $10 subtracted
//	at the server end. Wildcard character, meanwhile, ends up as $FA (-6).
//	This means when converting the resulting drive ID back to integer form, we add $30 regardless,
//	changing $1A-$1F to $4A-$4F.
//	Might be nice if server accepted $01-$0F for the drive number and $00 for 'all drives'.
//

.proc GetDriveID
	ror Temp1	; wildcard flag
	jsr GetNextChar
	bcs Abort
	bit Temp1
	bpl @+
	cmp #'*'
	bne @+
	lda #$FA	; '*' - $30
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
	sbc #'A'-2	; carry is clear
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
	

//
//	TS: Time Set
//
	
.proc GetTD
	jmp SetTD	; read time/date from server and set clock
.endp


//
//	TO: Time set, enable TD line
//

.proc GetTDOn
	jsr SetTD
	bcs Abort
	ldy #1
	jsr TDLineOnOff
Abort
	rts
.endp


//
//	TF: Time set, disable TD line
//

.proc GetTDOff
	jsr SetTD
	bcs Abort
	ldy #0
	jsr TDLineOnOff
Abort
	rts
.endp




//
//	Read date and time from the server and set the clock
//

.proc SetTD
;	jsr Printf
;	.byte 'Polling server for date/time',155,0

	lda #DCB.GetTD
	jsr SetUpDCB
	jsr SIOV
	bpl OK5
	jsr Printf
	.byte 'No server response!',155,0
	sec
	rts
	
OK5	; got date and time from server, so attempt to set Sparta clock
	jsr Printf
	.byte 'Date received from server...',155,0
	
	lda $701
	cmp #$44
	bcs IsSDX2

	ldx #5
	ldy #$0D+$05
@
	lda IOBuf,x
	sta (comtab),y
	dey
	dex
	bpl @-
	
	lda portb
	pha
	and #$FE
	sta portb
	jsr I_SETTD	; this will fail if the vectors aren't there
	pla
	sta portb
	bcc TDSetOK
	bcs TDSetFailed
	
IsSDX2			; with SDX, use kernel
	ldx #5
@
	lda IOBuf,x
	sta $077B,x
	dex
	bpl @-
	
	ldy #$65
	
	lda #$10
	sta $0761
	jsr $0703
	beq TDSetOK
	
TDSetFailed
	jsr Printf
	.byte 'Error setting time/date!',155,0
	sec
	rts
	
TDSetOK
	jsr Printf
	.byte 'Time/date set',155,0
	clc
	rts
.endp



//
//	Enable or disable the TD Line
//	Y = 0: Turn off
//	Y = 1: Turn on
//

.proc TDLineOnOff
	lda $701
	cmp #$44
	bcs IsSDX1
	
	lda portb
	pha
	and #$FE
	sta portb
	jsr I_TDON
	pla
	sta portb
	bcs TDOnFailed
	rts
	
IsSDX1
	sty Temp2
	ldax Symbol	; under SDX, enable TD using kernel
	jsr $07EB	; get symbol address
	bmi TDOnFailed

	stax TDVec+1	; store address
	ldy Temp2
TDVec
	jsr $FFFF	; should really check for errors...
	clc
	rts
	
TDOnFailed
	jsr Printf
	.byte 'Error enabling TD Line!',155,0
	sec
	rts
.endp




//
//	DS: Disk Swap
//

.proc Swap
	jsr GetDrv
	bcs Error
	sta Slot
	jsr MakeDriveID	; make some drive IDs for later
	sta DriveID1
	jsr GetDrv
	bcs Error
	sta Drive
	jsr MakeDriveID
	sta DriveID2
	jsr GetNextChar
	bcc Error	; make sure next char is end of string

	lda #DCB.Swap
	jsr SetUpDCB
	mva Slot DAUX1
	mva Drive DAUX2
	jsr SIOV
	bpl OK1
	jsr Printf
	.byte 'Error swapping disks!',155,0
	sec
	rts
OK1
	jsr Printf
	.byte 'Swapped disk %c with %c',155,0
	.word DriveID1,DriveID2
	clc
	rts
Error
	jmp Usage
.endp
	
	
//
//	DU: Unmount disk
//

.proc UnMount
	jsr GetDrvWC
	bcs Error
	sta Drive
	jsr GetNextChar	; make sure next char is end of arg
	bcc Error

	lda #DCB.Unmount
	jsr SetUpDCB
	mva Drive DAUX2
	jsr SIOV
	bpl OK6	
	jsr Printf
	.byte 'Error unmounting volume!',155,0
	sec
	rts
OK6
	lda Drive
	cmp #-6		; bizarrely, '*' must be encoded as $FA (0 might be a better choice for 'all drives')
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 'Drive %c unmounted',155,0
	.word DriveID1
	clc
	rts
AllDrives
	jsr Printf
	.byte 'All drives unmounted',155,0
	clc
	rts
Error
	jmp Usage
.endp


//
//	DM: Mount disk
//

.proc Mount
	clc
	.byte $24	; BIT ZP
.endp


//
//	DN: Create and mount
//

.proc CreateAndMount
	sec
.endp


.proc DoMount
	ror CreateFlag	; c = 1: Create and Mount
	mva #$20 Temp1
	ldx #0
Loop1
	iny
	lda (comtab),y
	beq Fin
	cmp #155
	beq Fin
	jsr ToUpper
	bit Temp1	; don't add to filename if we already found second period
	bmi Store
	cmp #'.'
	bne NotPeriod
	asl Temp1
	bpl NotPeriod	; we terminate the printable copy of the filename on the second period we encounter
	lda #0		; if so, terminate filename
	sta FileName,x
	lda #'.'
	bne Store
NotPeriod
	sta FileName,x
Store
	sta IOBuf,x
	inx
	bne Loop1
Fin
	lda #0
	sta IOBuf,x
	sta FileName,x
	
	lda #DCB.Mount
	bit CreateFlag
	spl
	lda #DCB.CreateAndMount
	jsr SetUpDCB
	jsr SIOV
	bpl OK3
	jsr Printf
	.byte 'Error mounting image!',155,0
	sec
	rts

OK3			; image mounted, so now get drive number
	lda #DCB.GetDrvNum
	jsr SetUpDCB
	jsr SIOV
	bpl OK4
	
	jsr Printf
	.byte 'Error obtaining drive number!',155,0
	sec
	rts
OK4
	lda IOBuf
	clc
	adc #1	; bump drive number since server returns drive-1
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte '%s mounted on drive %c',155,0
	.word Filename,DriveID1
	clc
	rts
.endp



//
//	DA: Toggle Auto-commit
//

.proc ToggleAutoCommit
	jsr GetDrvWC
	bcs Error
	sta Drive
	jsr GetNextChar
	bcc Error

	lda #DCB.AutoToggle
	jsr SetUpDCB
	mva Drive DAUX1
	jsr SIOV
	bpl OK2
	jsr Printf
	.byte 'Error toggling auto-commit!',155,0
	sec
	rts
OK2
	lda Drive
	cmp #-6
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 'Auto-commit toggled on drive %c',155,0
	.word DriveID1
	clc
	rts
Error
	jmp Usage
AllDrives
	jsr Printf
	.byte 'Auto-commit toggled on all drives',155,0
	clc
	rts
	.endp



//
//	Show command usage
//

Usage
	jsr Printf
	.byte 'Usage: RCL command',155
	.byte 155
	.byte 'Commands:',155
	.byte 155
	.byte 'TS: Set time/date',155
	.byte 'TO: Set time/date and enable TD Line',155
	.byte 'TF: Set time/date and disable TD Line',155
	.byte 155,0
	
	jsr Printf
	.byte 'DMfname.ext:   Existing Image Mount',155
	.byte 'DNfname.ext.x: New Image Mount',155
	.byte 155
	.byte ' x: 1:SSSD 2:SSED 3:SSDD',155
	.byte '    4:DSDD 5:DDHD 6:QDHD',155
	.byte 155,0

	jsr Printf
	.byte 'DU[d/*]: Unmount disk/all disks',155
	.byte 'DS[dd]:  Swap Disks',155
	.byte 'DA[d/*]: Auto-commit disk/all disks',155
	.byte 0
	sec
	rts



	.proc Exit
	jsr Printf
	.byte 'Press a key to quit',155,0
	jmp GetKey
	.endp

	
	.proc Crunch
	jmp $FFFF
	.endp
	
	.proc GetKey
	lda $E425
	pha
	lda $E424
	pha
	rts
	.endp
	
	
//
//	Make drive ID
//	Pass drive number in A
//	This is unexpectedly simple since drive letters and numbers all have the same transition applied
//

	.proc MakeDriveID
;	cmp #10
;	bcs @+
	clc
	adc #'0'	; everything gets $30 added
	rts
;@
;	clc
;	adc #'A'-1
;	rts
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
	mva #$46 DDevic	; ddevic and dunit are common to all
	mva #$01 DUnit
	rts
DCBIndex
	.byte 9,19,29,39,49,59,69
	.endp
	
	
	
	icl 'printf.s'

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
	

Symbol
	.byte 'I_TDON  ',0
	
ArgFlag
	.byte 0
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
Filename
	.ds 16
	
IOBuf
	.ds 256

	run Start
