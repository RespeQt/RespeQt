; support macros for debbuging using an external display
; (c) 2005 Matthias Reichl

	HWDISP	= $D5F0

	.MACRO DBGCLR ; clear debug display
	.IF .DEF HWDEBUG
	JSR SDBGCLR
	.ENDIF
	.ENDM

	.MACRO DBGTXT ; write 8 ascii bytes to display
	.IF .DEF HWDEBUG
	LDX #0
	BEQ DBGTXT1
DBGTXTT	.BYTE %$1
DBGTXT1	LDA DBGTXTT,X
	STA HWDISP,X
	INX
	CPX #8
	BNE DBGTXT1
	.ENDIF
	.ENDM

	.MACRO DBGT4 ; pos, text - write 4 ascii bytes to display
	.IF .DEF HWDEBUG
	LDX #%1
	LDY #0
	BEQ DBGTXT1
DBGTXTT	.BYTE %$2
DBGTXT1	LDA DBGTXTT,Y
	STA HWDISP,X
	INX
	INY
	CPY #4
	BNE DBGTXT1
	.ENDIF
	.ENDM

	.MACRO DBGADR ; offset, adr - write 2 byte hex address
	.IF .DEF HWDEBUG
        LDX #%1
	LDA %2+1
	JSR SDBGHEX
	LDA %2
	JSR SDBGHEX
	.ENDIF
	.ENDM

	.MACRO DBGA ; write A (in hex) to position X,X+1
	.IF .DEF HWDEBUG
	JSR SDBGHEX
	.ENDIF
	.ENDM

	.MACRO DBGASC ; write A to specified position
	.IF .DEF HWDEBUG
	STA HWDISP+%1
	.ENDIF
	.ENDM
	

