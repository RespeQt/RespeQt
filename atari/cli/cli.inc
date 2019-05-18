STATUS	equ $0030
CHKSUM	equ $0031
BUFRLO	equ $0032
BUFRHI	equ $0033
BFENLO	equ $0034
BFENHI	equ $0035
RECVDN	equ $0039
XMTDON	equ $003A
CHKSNT	equ $003B
NOCKSM	equ $003C
BPTR	equ $003D
FTYPE	equ $003E

GINTLK	equ	$03FA
TRIG3	equ $D013

portb	equ $d301
pactl	equ $d302
pbctl	equ $d303
porta	equ $d300

Boot	equ $09
dosvec	equ $0A

RTCLOCK	equ $12
BRKKEY	equ $11
CRITIC	equ $42
ch		equ $2fc
BASICF	equ $3f8

DVSTAT	equ $02EA

DDEVIC  equ $0300
DUNIT   equ $0301
DCOMND	equ $0302
DSTATS	equ $0303
DBUFLO	equ $0304
DBUFHI	equ $0305
DTIMLO	equ $0306
DUNUSE	equ $0307
DBYTLO	equ $0308
DBYTHI	equ $0309
DAUX1	equ $030A
DAUX2	equ $030B
DAUXA	equ $030C ; xdcb
DAUXB	equ $030D ; xdcb
DBFX1	equ $030E ; xdcb
DBFX2	equ $030F ; xdcb
RTCLOK	equ $12
savmsc	equ $58

RAMSIZ	equ $02E4
MEMTOP	equ $02E5
RAMTOP	equ $6A

; IOCB stuff

FMSORG	equ	$0700
FMSZPG	equ	$43
IOCBORG	equ	$0340
DCBORG	equ	$0300
DHADR	equ	$E453
EOL		equ	$9B
DEVTAB	equ	$031A
ZICB	equ	$20
LMADR	equ	$02E7
STAK	equ	$0102	; STACK LOC FOR PUT BYTE
DSKTIM	equ	$0246	; ADDR OF OS WORST CASE DISK TIME OUT
TIMOUT	equ	15		; TIME OUT VALUE OF 15 SECS

Stick0	equ $0278
Stick1	equ $0279

Strig0	equ $0284
Strig1	equ $0285

Trig0	equ $D010
Trig1	equ $D011

DOSINI	equ $0C
MEMLO	equ $2E7

RunVec	equ $02E0
IniVec	equ $02E2


	.struct IOCBlock
ID		.byte
DevNum	.byte
Command	.byte
Status	.byte
Address	.word
Put		.word	; put byte address
Len		.word
Aux1	.byte 	
Aux2	.byte
Aux3	.byte
Aux4	.byte
Aux5	.byte
Aux6	.byte
	.ends
	
;	org $20
	
;ZIOCB	dta IOCBlock
	
	org $0340

IOCB	dta IOCBlock [8]

;	CIO commands

	.enum IOCommand
Open	= $03
GetText	= $05
Read	= $07
PutText	= $09
Write	= $0B
Close	= $0C
Status	= $0D
	.ende
	
;	CIO aux values

	.enum IOMode
Read	= $04
Dir		= $06
Create	= $08
Append	= $09
Update	= $0C
	.ende
	
	
;	CIO Error Codes

	.enum IOErr
AlreadyOpen	= 129
BadDevice	= 130
NotOpen		= 133
EOF			= 136
NAK			= 139
NoFunction	= 146
BadPath		= 150
BadName		= 165
NotFound	= 170
	.ende


; status values

	.enum Stat
OK			= $01
Trunc		= $02
EOF			= $03
Break		= $80
NotReady	= $81	; device not ready
NoDevice	= $82
DataError	= $83
BadCommand	= $84
NotOpen		= $85
BadIOCB		= $86
Locked		= $87
	.ende


KEYBDV	equ $E420
CIOV	equ $E456


SIOV    equ $E459

pdvmsk 	equ $247
shpdvs	equ $248
pdmsk	equ $249

HATABS	equ $031A



// Shadow registers

VDSLST	equ	$200
VKEYBD	equ	$208
VIMIRQ	equ $216
CDTMV1	equ $218
VVBLKI	equ	$222
VVBLKD	equ	$224
CDTMA1	equ $226
GPRIOR	equ	$26F
SDMCTL	equ $22F
SDLSTL	equ	$230


// POKEY

POKEY1	equ	$D200
POKEY2	equ	$D210
KBCODE	equ	$D209
RANDOM	equ $D20A
IRQEN	equ	$D20E
IRQST	equ	$D20E
SKSTAT	equ	$D20F
SKCTL	equ	$D20F

pokmsk	equ $10

sskctl	equ $232

audf1	equ $d200
audc1	equ $d201
audf2	equ $d202
audc2	equ $d203
audf3	equ $d204
audc3	equ $d205
audf4	equ $d206
audc4	equ $d207
audctl	equ $d208
SKREST	equ $D20A
SERIN	equ $D20D
SEROUT	equ $D20D

// ANTIC

DMACTL	equ	$D400
CHRCTL	equ	$D401
DLPTR	equ	$D402
HSCROL	equ	$D404
VSCROL	equ	$D405
PMBASE	equ	$D407
CHBASE	equ	$D409
WSYNC	equ	$D40A
VCOUNT	equ	$D40B
LPENH	equ	$D40C
LPENV	equ	$D40D
NMIEN	equ	$D40E
NMIST	equ	$D40F
NMIRES	equ	$D40F

Color2	equ	$2C6
Color3	equ	$2C7
Color4	equ	$2C8


// GTIA

HPosP0	equ $D000
HPosP1	equ $D001
HPosP2	equ $D002
HPosP3	equ $D003
HPosM0	equ $D004
HPosM1	equ $D005
HPosM2	equ $D006
HPosM3	equ $D007

SizeP0	equ $D008
SizeP1	equ $D009
SizeP2	equ $D00A
SizeP3	equ $D00B
SizeM	equ $D00C
GrafP0	equ $D00D
GrafP1	equ $D00E
GrafP2	equ $D00F
GrafP3	equ $D010

ColPM0	equ $D012
ColPM1	equ $D013
ColPM2	equ $D014
ColPM3	equ $D015
COLPF0	equ $D016
COLPF1	equ $D017
COLPF2	equ $D018
COLPF3	equ $D019
COLBAK	equ $D01A
Prior	equ $D01B
VDelay	equ $D01C
Gractl	equ $D01D
CONSOL	equ $D01F



iccom	equ	$342
icstat	equ	$343
icbadr	equ	$344
icblen	equ	$348
icaux1	equ	$34a
icaux2	equ	$34b

comtab	equ $0A

	org $80
	
ptr1	.ds 2
ptr2	.ds 2
ptr3	.ds 2
ptr4	.ds 2
tmp1	.ds 1
tmp2	.ds 1
tmp3	.ds 1
tmp4	.ds 1

xsave	.ds 1
ysave	.ds 1

ArgIndex 	.ds 1
LeadingZeroFlag	.ds 1
FieldWidth	.ds 1
StringIndex	.ds 1

// RespeQt client specific commands
rcldev		equ $46 // device id for RCl
	.enum Cmd
GetTD		= $93
Swap		= $94
Unmount		= $95
Mount		= $96
CreateAndMount	= $97
AutoToggle	= $98
	.ende
	
	.enum DCB
GetTD
Swap
Unmount
Mount
CreateAndMount
AutoToggle
GetDrvNum
	.ende

// RespeQt client data structure.
	.struct DateTimeData
day 	.byte 
month	.byte
year	.byte 
hour	.byte 
minute	.byte
second	.byte
	.ends
