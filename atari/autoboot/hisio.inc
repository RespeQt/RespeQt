;  highsio.inc - definitions for highspeed SIO code
;
;  Copyright (c) 2003-2010 by Matthias Reichl <hias@horus.com>
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

	POKMSK = $10

	RTCLOK = $12

	STATUS = $30
	CHKSUM = $31
	BUFRLO = $32
	BUFRHI = $33
	BFENLO = $34
	BFENHI = $35

; $36/$37 are kept free to avoid troubles with TurboDOS

; re-using original SIO ZP locations $38 - $3C for my own code
	CRETRY = $38	; command frame retries
	DRETRY = $39	; data frame retries

; default number of command frame retries
	DEFCRETRY = $0E

; number of command frame retries when checking highspeed mode
	DETCRETRY = $05

; speed used by this SIO command (changed to standard for second retry)
	MYSPEED = $3A

; timer in ZP, used by fast VBI code
	MYTIM1 = $3B ; $3C

	STACKP = $318
;	STACKP = $3B

;	CRETRY = $29C
;	DRETRY = $2BD

	SOUNDR = $41
	CRITIC = $42

	CDTMV1 = $218
	VVBLKI = $222
	CDTMA1 = $226
	CDEVIC = $23A
	CCOMND = $23B
	CAUX1 = $23C
	CAUX2 = $23D

	DDEVIC = $300
	DUNIT = $301
	DCOMND = $302
	DSTATS = $303
	DBUFLO = $304
	DBUFHI = $305
	DTIMLO = $306
	DBYTLO = $308
	DBYTHI = $309
	DAUX1 = $30A
	DAUX2 = $30B

	AUDF3 = $D204
	AUDF4 = $D206
	AUDC4 = $D207
	AUDCTL = $D208
	SKREST = $D20A
	SERIN = $D20D
	SEROUT = $D20D
	IRQEN = $D20E
	IRQST = $D20E
	SKCTL = $D20F
	SKSTAT = $D20F

; IRQEN masks
	IMBRK = $80	; break key pressed
	IMKBD = $40	; key pressed
	IMRECV = $20	; byte received
	IMSEND = $10	; ready to send next byte
	IMSCPL = $08	; transmission complete

; and the same, inverted, to reset IRQ status
	RMBRK = $7F
	RMRECV = $DF
	RMSEND = $EF

; standard masks: break + send/receive
	MSKRECV = $A0
	MSKSEND = $90

; Pokey SKCTL value for transmitting data (sync. send)
	SKSEND = $23

; Pokey SKCTL value for receiving data (async. recv)
	SKRECV = $13

; error codes
	ERRBRK = $80	; break key pressed
	ERRTO = $8A	; timeout
	ERRNAK = $8B	; command NAK
	ERRFRM = $8C	; framing error
	ERROVR = $8E	; data overrun
	ERRCHK = $8F	; checksum error
	ERRDEV = $90	; device error

	PBCTL = $D303
	WSYNC = $D40A
	VCOUNT = $D40B

	SETVBV = $E45C
	XITVBV = $E462

.if .not .def USONLY
	TURSPD = 6
	XFSPD = 16
.endif
	STDSPD = $28

.if .not .def USONLY
	TURFLG = $80
	XFFLG = $40
	HAPFLG = $41
.endif

