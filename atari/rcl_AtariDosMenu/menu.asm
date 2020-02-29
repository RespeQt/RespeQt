
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
	icl 'equ.asm'

	org $4000
Start
	jsr printf
	.byte 125,155,'RespeQt Client        Version 0.1  ',155
	.byte         '                  for RespeQt 5.x ',155,155,155
	.byte 'A List Slots     I List Host Images',155
	.byte 'B Mount Disk     J Auto Commit On',155
	.byte 'C Create Disk    K Auto Commit Off',155,0
    jsr printf
    .byte 'D UnMount Disk   L Set Date',155
	.byte 'E Save Disk      M TD Line On',155	
	.byte 'F Swap Slot      N TD Line Off',155
	.byte 'G Boot Disk      O Exit to Dos',155
	.byte 'H Boot XEX/Exe   P Cold Reboot',155,0
	
Main	
    jsr printf
	.byte 155,155,'Enter Command or Return for Menu: ',0
	jsr Input1
	bcs Main	
	jsr ToUpper
	
	cmp #'A'
    jeq SlotName    
    cmp #'B'
    jeq Mount
    cmp #'C'
    jeq CreateAndMount	
	cmp #'D'
    jeq UnMount
	cmp #'E'
    jeq Save    
    cmp #'F'
    jeq Swap
    cmp #'G'
    jeq BootATR
    cmp #'H'
    jeq BootXEX
    cmp #'I'    
    jeq ListDir   
    cmp #'J'    
    jeq CommitOn
    cmp #'K'    
    jeq CommitOff  
    cmp #'L'
    jeq GetTD
    cmp #'M'
    jeq GetTDOn
    cmp #'N'
    jeq GetTDOff
	cmp #'O'
    jeq Exit
	cmp #'P'
    jeq Reboot

    
	jmp Start

//	
//  get disk in slot
//    
.proc SlotName
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*] : ',0
	
    jsr input1
    jcs main
    jsr toUpper   
    jsr GetDrvWC
    jcs main
    
    sta drive
	cmp #$FA
	bne OneSlot
MultiSlot
    ldx #$01
    stx lp
LOOPa
    txa  
    sta drive
    jsr getSlotFileName
    adb lp #01
    ldx lp
    cpx #$0A
    bne LOOPa

    ldx #$1A
    stx lp
LOOPb
    txa  
    sta drive
    jsr getSlotFileName
    adb lp #01
    ldx lp
    cpx #$20
    bne LOOPb
    jmp Main       
OneSlot
    jsr getSlotFileName
    jmp Main
.endp



.proc getSlotFileName
   lda #DCB.GetSL
   jsr SetUpDCB
   mva drive DAUX1
   jsr SIOV
   bpl OKa
   jsr Printf
   .byte 155,'No server response!',0
   sec
   rts
OKa
    adb drive #$30
jsr Printf
	.byte 155,' Slot %c: %s',0
	.word Drive, Path
	clc
	rts	
.endp

//
//	Mount disk and boot!
//
.proc BootATR
	lda #0
	sta ArgFlag
	clc
	jmp MountAndBoot
.endp

.proc BootXEX
	lda #1
	sta ArgFlag
	clc
.endp

.proc  MountAndBoot
    jsr printf
	.byte 155,'Enter [FILENAME.EXT] : ',0
    jsr input
    jcs main
    cpy #03
    jmi main    
    ldx #0
loopB   
    lda InputBuf,x
	cmp #155
    beq AllDoneB
StoreB        
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne LoopB
AllDoneB
	lda #0
	sta IOBuf,x       	
	lda #DCB.MountAndBoot	
	mvx ArgFlag DAUX2
	jsr SetUpDCB
	mva ArgFlag DAUX2
	jsr SIOV
	bpl OKB
	jsr Printf
	.byte 155,'Error mounting image!',155,0
	sec
	jmp main
OKB			; image mounted

	jsr Printf
	.byte 155,'Ready to reboot - press return'
	.byte 155,'You may need to press [Option]:  ',0	
	jsr Input1
	jmp $E477
.endp


//------------------------------------------------------
//
//	Mount disk
//
.proc Mount
	lda #0
	sta CreateFlag
	clc
	jmp doMount
.endp
//
//	Create and mount
//
.proc CreateAndMount
    jsr printf
    .byte 155,'  [1] SSSD  [2] SSED  [3] SSDD'
	.byte 155,'  [4] DSDD  [5] DDHD  [6] QDHD'
	.byte 155,155,'Enter Disk Type : ',0
	jsr input1
    jcs main
    tay
    cmp #'6'+1
	jcs main
    sbc #'0'-1
	jcc main
	jeq main	
    sty CreateFlag 	
.endp

.proc DoMount
    ldy #0
    sty  ArgFlag
noDot1
    jsr printf
	.byte 155,'Enter [FILENAME.EXT] : ',0
    jsr input
    jcs main
    cpy #03
    jmi main    
    ldx #0
loop1    
    cpx #13
    beq noDot1
    lda InputBuf,x
	cmp #155
    beq FlFin1
    cmp #'.'
    bne Store1
    ldy #01
    sty ArgFlag
Store1        
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne Loop1
FlFin1
    ldy ArgFlag
    cpy #0
    beq noDot1
    ldy CreateFlag
    cpy #0
    beq AllDone1
    lda #'.' 
    sta IOBuf,x
    inx
    tya
    sta IOBuf,x
    inx
AllDone1
	lda #0
	sta IOBuf,x       	
	lda #DCB.Mount
	
	cpy #0
	beq goMount
	lda #DCB.CreateAndMount
goMount
	jsr SetUpDCB
	jsr SIOV
	bpl OK3
	jsr Printf
	.byte 155,'Error mounting image!',155,0
	sec
	jmp main

OK3			; image mounted, so now get drive number
	lda #DCB.GetDrvNum
	jsr SetUpDCB
	jsr SIOV
	bpl OK4
	
	jsr Printf
	.byte 'Error obtaining drive number!',155,0
	sec
	jmp main
OK4
	lda IOBuf
	clc
	adc #1	; bump drive number since server returns drive-1
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 'Mounted on drive %c',155,0
	.word DriveID1
	clc
	jmp main
.endp


//
//	Unmount disk
//
.proc UnMount
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*] : ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrvWC
	jcs main	
	sta Drive
	lda #DCB.Unmount
	jsr SetUpDCB
	mva Drive DAUX2
	jsr SIOV
	bpl OK6	
	jsr Printf
	.byte 155,'Error unmounting volume!',155,0
	sec
	jmp main
OK6
	lda Drive
	cmp #-6		 
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 155,'Drive %c unmounted',155,0
	.word DriveID1
	clc
	jmp main
AllDrives
	jsr Printf
	.byte 155,'All drives unmounted',155,0
	clc
	jmp main
.endp

	
//
//	Save disks
//
.proc Save
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*] : ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrvWC
	jcs main	
	sta Drive
	lda #DCB.Save
	jsr SetUpDCB
	mva Drive DAUX2
	jsr SIOV
	bpl OK7	
	jsr Printf
	.byte 155,'Nothing can be saved!',155,0
	sec
	jmp main
OK7
	jsr Printf
	.byte 155,'Disk(s) saved',155,0
	clc	
	jmp main
.endp
	
	
	
//
//	Disk Swap
//
.proc Swap
    jsr printf
    .byte 155,'Enter Slot 1  [1-9] [J-O] : ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrv
	jcs main 
	sta Slot
	jsr MakeDriveID	; make some drive IDs for later
	sta DriveID1

    jsr printf
    .byte 155,'Enter Slot 2  [1-9] [J-O] : ',0
    jsr input1
    jcs main
    jsr toUpper
	jsr GetDrv
	jcs main
	sta Drive
	jsr MakeDriveID
	sta DriveID2

	lda #DCB.Swap
	jsr SetUpDCB
	mva Slot DAUX1
	mva Drive DAUX2
	jsr SIOV
	bpl OK1
	jsr Printf
	.byte 155,'Error swapping disks!',155,0
	sec
	jmp Main
OK1
	jsr Printf
	.byte 155,'Swapped disk %c with %c',155,0
	.word DriveID1,DriveID2
	clc	
	jmp Main
.endp

//
//  Toggle Auto Commit
//
.proc CommitOn
      LDY #01
      jmp ToggleCommit
.endp      
.proc Commitoff
      LDY #00
.endp      
.proc ToggleCommit
    sty ArgFlag
    jsr printf
	.byte 155,'Enter Slot [1-9] [J-O] [*] : ',0
    jsr input1
    jcs main
    jsr toUpper 
	jsr GetDrvWC
	jcs main
	sta Drive
	lda #DCB.AutoToggle
	jsr SetUpDCB
	mva Drive DAUX1 
	mva ArgFlag DAUX2
	jsr SIOV
	bpl OK2
	jsr Printf
	.byte 155,'Error toggling auto-commit!',155,0
	sec
	jmp Main
OK2
	lda Drive
	cmp #-6
	beq AllDrives
	jsr MakeDriveID
	sta DriveID1
	jsr Printf
	.byte 155,'Auto-commit toggled on drive %c',155,0
	.word DriveID1
	clc
	jmp main
	
AllDrives
	jsr Printf
	.byte 155,'Auto-commit toggled on all drives',155,0
	clc
	jmp main
	.endp


//
//  List pth folder 
//
// 
.proc ListDir
    ldx #$00
    stx lp
    
//--------------------

    jsr printf
	.byte 155,'Filter [*file spec*]: ',0
    jsr input
    jcs main
    ldx #0
loop2    
    lda InputBuf,x
	cmp #155
    beq FlFin2
    jsr ToUpper	
    sta IOBuf,x
    inx
    bne Loop2
FlFin2
	lda #0
	sta IOBuf,x   
    
    lda #DCB.PutDR
    jsr SetUpDCB
    jsr SIOV
    bpl list2
    jsr Printf
    .byte 155,'No server response!',155,0
    sec
    jmp Main
list2    
    lda #DCB.GetDR
    jsr SetUpDCB
    mva lp   DAUX1
    mva #$01 DAUX2
    jsr SIOV
    bpl OK2a
    jsr Printf
    .byte 155,'No server response!',155,0
    sec
    jmp Main
OK2a
    jsr Printf
   .byte 155,'%s',155,0
   .word IOBuf
	ldx lp
	cpx #00
	beq allDone2a
	jsr Printf
	.byte ' * more..(q=quit)',0
	jsr getkey
	jsr ToUpper
	cmp #'Q'
	beq allDone2a
	jmp list2
allDone2a	
	clc
	jmp Main	
.endp


//
//	Time/Date Set
//
.proc GetTD
    jsr isSparta
    jcs main
	jsr SetTD	
	jmp Main
.endp

//
//  Time set, enable TD line
//
.proc GetTDOn
    jsr isSparta
    jcs main
	jsr SetTD
	bcs Abort
	ldy #1
	jsr TDLineOnOff
Abort
	jmp Main
.endp


//
//	Time set, disable TD line
//
.proc GetTDOff
    jsr isSparta
    jcs main
	jsr SetTD
	bcs Abort
	ldy #0
	jsr TDLineOnOff
Abort	
    jmp Main
 .endp 


//
//	Read date and time from the server and set the clock
//
.proc SetTD
	lda #DCB.GetTD
	jsr SetUpDCB
	jsr SIOV
	bpl OK5
	jsr Printf
	.byte 155,'No server response!',155,0
	sec
	rts
	
OK5	; got date and time from server, so attempt to set Sparta clock
	jsr Printf
	.byte 155,'Date received from server',155,0
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
	.byte 155,'Error setting time/date!',155,0
	sec
	rts
	
TDSetOK
	jsr Printf
	.byte 155,'Time/date set',155,0
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
	.byte 155,'Error enabling TD Line!',155,0
	sec
	rts
.endp


.proc isSparta 
	lda $0700
	cmp #'S'
	beq ok
	cmp #'R'
	beq ok
	jsr Printf
	.byte 'Not Sparta or Real Dos',155,0
	sec
	rts
ok 
    clc
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

.proc GetDriveID
	ror Temp1	; wildcard flag
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
	
 	
		
.proc Exit
	jsr Printf
	.byte 155,'Press a key to quit',155,0
	jmp GetKey
.endp

	
.proc Crunch
	jmp $FFFF
.endp

.proc Reboot
	jsr Printf
	.byte 155,'Press Y to reboot'
	.byte 155,'You may need to press [Option]:  ',0	
	jsr Input1
	cmp #'Y'
	jne main
	jmp $E477
.endp

	
.proc GetKey
	lda $E425
	pha
	lda $E424
	pha
	rts
.endp
	
.proc MakeDriveID
	clc
	adc #'0'	; everything gets $30 added
	rts	
.endp
		
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
	.byte 9,19,29,39,49,59,69,79,89,99,109,119
.endp
	
	icl 'printf.asm'

DCBTable
DCBPutDR
	.byte Cmd.GetDR
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $20	
	.byte $00,$00
DCBGetDR
	.byte Cmd.GetDR	  
	.byte $40		   
	.word IOBuf        
	.byte $08,$00	   
	.word $FF		  
	.byte $00,$00	  
DCBGetSL
	.byte Cmd.GetSL	  
	.byte $40		  
	.word Path        
	.byte $06,$00	  
	.word $16		  
	.byte $00,$00	  
DCBGetTD
	.byte Cmd.GetTD	
	.byte $40		
	.word IOBuf		
	.byte $06,$00	
	.word $06		
	.byte $00,$00	
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
DCBSave
	.byte Cmd.Save
	.byte $00
	.word IOBuf
	.byte $06,$00
	.word 0
	.byte $00,$00	
DCBMountAndBoot
	.byte Cmd.MountAndBoot
	.byte $80
	.word IOBuf
	.byte $06,$00
	.word $0C
	.byte $00,$00	

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
Path 
	.ds 22
Filename
	.ds 16		
IOBuf
	.ds 254
lp
    .ds   1	
	
	run Start
	