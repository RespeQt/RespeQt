;  hipatch - 6502 high speed SIO code OS patch program
;
;  Copyright (c) by Matthias Reichl <hias@horus.com>
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

; indicate we are building the OS ROM PATCH
HIPATCH = 1

.if .def ROMABLE
  RAMBUF = $0108	; leave the first 8 bytes at page 1 free, some programs use them
.else
  RAMBUF = $CC00
.endif

SIOSPEED = RAMBUF	; speed byte for current operation
SPEEDTB	= RAMBUF+1	; 8-byte table for drive speeds

.if .def PATCHKEY
  ; add a config byte to enable/disable highspeed SIO with keyboard
  HIENAB = RAMBUF+9
  HIBUF = RAMBUF+10	; .. RAMBUF+13 (4 bytes for get status)
.else
  HIBUF = RAMBUF+9	; .. RAMBUF+12
.endif

; address of reset handler
.if .def ROMABLE
  RESETAD = HIBUF+4	; after end of highspeed SIO data
.else
  RESETAD = $0108	; (almost) at beginning of page 1
.endif

PDEST	= $CC00		; starting address of code patch
PSTD	= $CC10		; original SIO code + JMP back
PPWRUP	= $CC18		; powerup-code, checking for SHIFT (=cold boot)

PSIO	= $CC30		; start of new highspeed code

; power-up code
PUPCODE	= $C2B3		; this contains "LDA $033D"

; PNMI	= $CF68		; new NMI handler

.if .def PATCHKEY
PKEY	= $CFB8		; new keyboard IRQ handler

KEYIRQ  = $FC20 	; this contains "LDA $D209"
.endif


PVER = $CFF0		; 16 bytes version string
