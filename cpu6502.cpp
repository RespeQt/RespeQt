//
// Cpu6502 class
// (c) 1996 Eric BACHER
//
// This class simulates a 6502 CPU.
// All undocumented op-codes are implemented
//

#include <cstdio>
#include <cstring>
#include "cpu6502.h"

// test if there is a page change
#define CROSS_PAGE(a, r)      (((a - r) ^ a) & 0xFF00)

// mask for Xas, Sya and other instructions
#define UNDOC_MASK            0xDB // not sure

static OPCODE tabOpcode02[256] =
{
	/* 00 */{ "BRK", false, MODE_IMPLIED },
	/* 01 */{ "ORA", false, MODE_INDEXED_INDIRECT },
	/* 02 */{ "kil", true,  MODE_IMPLIED },
    /* 03 */{ "slo", true,  MODE_INDEXED_INDIRECT },
	/* 04 */{ "dop", true,  MODE_ZERO_PAGE },
	/* 05 */{ "ORA", false, MODE_ZERO_PAGE },
	/* 06 */{ "ASL", false, MODE_ZERO_PAGE },
	/* 07 */{ "slo", true,  MODE_ZERO_PAGE },
	/* 08 */{ "PHP", false, MODE_IMPLIED },
	/* 09 */{ "ORA", false, MODE_IMMEDIATE },
	/* 0A */{ "ASL", false, MODE_ACCUMULATOR },
	/* 0B */{ "aac", true,  MODE_IMMEDIATE },
	/* 0C */{ "top", true,  MODE_ABSOLUTE },
	/* 0D */{ "ORA", false, MODE_ABSOLUTE },
	/* 0E */{ "ASL", false, MODE_ABSOLUTE },
	/* 0F */{ "slo", true,  MODE_ABSOLUTE },
	/* 10 */{ "BPL", false, MODE_RELATIVE },
	/* 11 */{ "ORA", false, MODE_INDIRECT_INDEXED },
	/* 12 */{ "kil", true,  MODE_IMPLIED },
    /* 13 */{ "slo", true,  MODE_INDIRECT_INDEXED },
	/* 14 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* 15 */{ "ORA", false, MODE_ZERO_PAGE_X },
	/* 16 */{ "ASL", false, MODE_ZERO_PAGE_X },
	/* 17 */{ "slo", true,  MODE_ZERO_PAGE_X },
	/* 18 */{ "CLC", false, MODE_IMPLIED },
	/* 19 */{ "ORA", false, MODE_ABSOLUTE_Y },
	/* 1A */{ "nop", true,  MODE_IMPLIED },
	/* 1B */{ "slo", true,  MODE_ABSOLUTE_Y },
	/* 1C */{ "top", true,  MODE_ABSOLUTE_X },
	/* 1D */{ "ORA", false, MODE_ABSOLUTE_X },
	/* 1E */{ "ASL", false, MODE_ABSOLUTE_X },
	/* 1F */{ "slo", true,  MODE_ABSOLUTE_X },
	/* 20 */{ "JSR", false, MODE_ABSOLUTE },
	/* 21 */{ "AND", false, MODE_INDEXED_INDIRECT },
	/* 22 */{ "kil", true,  MODE_IMPLIED },
	/* 23 */{ "rla", true,  MODE_INDEXED_INDIRECT },
	/* 24 */{ "BIT", false, MODE_ZERO_PAGE },
	/* 25 */{ "AND", false, MODE_ZERO_PAGE },
	/* 26 */{ "ROL", false, MODE_ZERO_PAGE },
	/* 27 */{ "rla", true,  MODE_ZERO_PAGE },
	/* 28 */{ "PLP", false, MODE_IMPLIED },
	/* 29 */{ "AND", false, MODE_IMMEDIATE },
	/* 2A */{ "ROL", false, MODE_ACCUMULATOR },
	/* 2B */{ "aac", true,  MODE_IMMEDIATE },
	/* 2C */{ "BIT", false, MODE_ABSOLUTE },
	/* 2D */{ "AND", false, MODE_ABSOLUTE },
	/* 2E */{ "ROL", false, MODE_ABSOLUTE },
	/* 2F */{ "rla", true,  MODE_ABSOLUTE },
	/* 30 */{ "BMI", false, MODE_RELATIVE },
	/* 31 */{ "AND", false, MODE_INDIRECT_INDEXED },
	/* 32 */{ "kil", true,  MODE_IMPLIED },
	/* 33 */{ "rla", true,  MODE_INDIRECT_INDEXED },
	/* 34 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* 35 */{ "AND", false, MODE_ZERO_PAGE_X },
	/* 36 */{ "ROL", false, MODE_ZERO_PAGE_X },
	/* 37 */{ "rla", true,  MODE_ZERO_PAGE_X },
	/* 38 */{ "SEC", false, MODE_IMPLIED },
	/* 39 */{ "AND", false, MODE_ABSOLUTE_Y },
	/* 3A */{ "nop", true,  MODE_IMPLIED },
	/* 3B */{ "rla", true,  MODE_ABSOLUTE_Y },
	/* 3C */{ "top", true,  MODE_ABSOLUTE_X },
	/* 3D */{ "AND", false, MODE_ABSOLUTE_X },
	/* 3E */{ "ROL", false, MODE_ABSOLUTE_X },
	/* 3F */{ "rla", true,  MODE_ABSOLUTE_X },
	/* 40 */{ "RTI", false, MODE_IMPLIED },
	/* 41 */{ "EOR", false, MODE_INDEXED_INDIRECT },
	/* 42 */{ "kil", true,  MODE_IMPLIED },
	/* 43 */{ "sre", true,  MODE_INDEXED_INDIRECT },
	/* 44 */{ "dop", true,  MODE_ZERO_PAGE },
	/* 45 */{ "EOR", false, MODE_ZERO_PAGE },
	/* 46 */{ "LSR", false, MODE_ZERO_PAGE },
	/* 47 */{ "sre", true,  MODE_ZERO_PAGE },
	/* 48 */{ "PHA", false, MODE_IMPLIED },
	/* 49 */{ "EOR", false, MODE_IMMEDIATE },
	/* 4A */{ "LSR", false, MODE_ACCUMULATOR },
	/* 4B */{ "asr", true,  MODE_IMMEDIATE },
	/* 4C */{ "JMP", false, MODE_ABSOLUTE },
	/* 4D */{ "EOR", false, MODE_ABSOLUTE },
	/* 4E */{ "LSR", false, MODE_ABSOLUTE },
	/* 4F */{ "sre", true,  MODE_ABSOLUTE },
	/* 50 */{ "BVC", false, MODE_RELATIVE },
	/* 51 */{ "EOR", false, MODE_INDIRECT_INDEXED },
	/* 52 */{ "kil", true,  MODE_IMPLIED },
	/* 53 */{ "sre", true,  MODE_INDIRECT_INDEXED },
	/* 54 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* 55 */{ "EOR", false, MODE_ZERO_PAGE_X },
	/* 56 */{ "LSR", false, MODE_ZERO_PAGE_X },
	/* 57 */{ "sre", true,  MODE_ZERO_PAGE_X },
	/* 58 */{ "CLI", false, MODE_IMPLIED },
	/* 59 */{ "EOR", false, MODE_ABSOLUTE_Y },
	/* 5A */{ "nop", true,  MODE_IMPLIED },
	/* 5B */{ "sre", true,  MODE_ABSOLUTE_Y },
	/* 5C */{ "top", true,  MODE_ABSOLUTE_X },
	/* 5D */{ "EOR", false, MODE_ABSOLUTE_X },
	/* 5E */{ "LSR", false, MODE_ABSOLUTE_X },
	/* 5F */{ "sre", true,  MODE_ABSOLUTE_X },
	/* 60 */{ "RTS", false, MODE_IMPLIED },
	/* 61 */{ "ADC", false, MODE_INDEXED_INDIRECT },
	/* 62 */{ "kil", true,  MODE_IMPLIED },
	/* 63 */{ "rra", true,  MODE_INDEXED_INDIRECT },
	/* 64 */{ "dop", true,  MODE_ZERO_PAGE },
	/* 65 */{ "ADC", false, MODE_ZERO_PAGE },
	/* 66 */{ "ROR", false, MODE_ZERO_PAGE },
	/* 67 */{ "rra", true,  MODE_ZERO_PAGE },
	/* 68 */{ "PLA", false, MODE_IMPLIED },
	/* 69 */{ "ADC", false, MODE_IMMEDIATE },
	/* 6A */{ "ROR", false, MODE_ACCUMULATOR },
	/* 6B */{ "arr", true,  MODE_IMMEDIATE },
	/* 6C */{ "JMP", false, MODE_INDIRECT },
	/* 6D */{ "ADC", false, MODE_ABSOLUTE },
	/* 6E */{ "ROR", false, MODE_ABSOLUTE },
	/* 6F */{ "rra", true,  MODE_ABSOLUTE },
	/* 70 */{ "BVS", false, MODE_RELATIVE },
	/* 71 */{ "ADC", false, MODE_INDIRECT_INDEXED },
	/* 72 */{ "kil", true,  MODE_IMPLIED },
	/* 73 */{ "rra", true,  MODE_INDIRECT_INDEXED },
	/* 74 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* 75 */{ "ADC", false, MODE_ZERO_PAGE_X },
	/* 76 */{ "ROR", false, MODE_ZERO_PAGE_X },
	/* 77 */{ "rra", true,  MODE_ZERO_PAGE_X },
	/* 78 */{ "SEI", false, MODE_IMPLIED },
	/* 79 */{ "ADC", false, MODE_ABSOLUTE_Y },
	/* 7A */{ "nop", true,  MODE_IMPLIED },
	/* 7B */{ "rra", true,  MODE_ABSOLUTE_Y },
	/* 7C */{ "top", true,  MODE_ABSOLUTE_X },
	/* 7D */{ "ADC", false, MODE_ABSOLUTE_X },
	/* 7E */{ "ROR", false, MODE_ABSOLUTE_X },
	/* 7F */{ "rra", true,  MODE_ABSOLUTE_X },
	/* 80 */{ "dop", true,  MODE_IMMEDIATE },
	/* 81 */{ "STA", false, MODE_INDEXED_INDIRECT },
	/* 82 */{ "dop", true,  MODE_IMMEDIATE },
	/* 83 */{ "aax", true,  MODE_INDEXED_INDIRECT },
	/* 84 */{ "STY", false, MODE_ZERO_PAGE },
	/* 85 */{ "STA", false, MODE_ZERO_PAGE },
	/* 86 */{ "STX", false, MODE_ZERO_PAGE },
	/* 87 */{ "aax", true,  MODE_ZERO_PAGE },
	/* 88 */{ "DEY", false, MODE_IMPLIED },
	/* 89 */{ "dop", true,  MODE_IMMEDIATE },
	/* 8A */{ "TXA", false, MODE_IMPLIED },
	/* 8B */{ "xaa", true,  MODE_IMMEDIATE },
	/* 8C */{ "STY", false, MODE_ABSOLUTE },
	/* 8D */{ "STA", false, MODE_ABSOLUTE },
	/* 8E */{ "STX", false, MODE_ABSOLUTE },
	/* 8F */{ "aax", true,  MODE_ABSOLUTE },
	/* 90 */{ "BCC", false, MODE_RELATIVE },
	/* 91 */{ "STA", false, MODE_INDIRECT_INDEXED },
	/* 92 */{ "kil", true,  MODE_IMPLIED },
	/* 93 */{ "axa", true,  MODE_INDIRECT_INDEXED },
	/* 94 */{ "STY", false, MODE_ZERO_PAGE_X },
	/* 95 */{ "STA", false, MODE_ZERO_PAGE_X },
	/* 96 */{ "STX", false, MODE_ZERO_PAGE_Y },
	/* 97 */{ "aax", true,  MODE_ZERO_PAGE_Y },
	/* 98 */{ "TYA", false, MODE_IMPLIED },
	/* 99 */{ "STA", false, MODE_ABSOLUTE_Y },
	/* 9A */{ "TXS", false, MODE_IMPLIED },
	/* 9B */{ "xas", true,  MODE_ABSOLUTE_Y },
	/* 9C */{ "sya", true,  MODE_ABSOLUTE_X },
	/* 9D */{ "STA", false, MODE_ABSOLUTE_X },
	/* 9E */{ "sxa", true,  MODE_ABSOLUTE_Y },
	/* 9F */{ "axa", true,  MODE_ABSOLUTE_Y },
	/* A0 */{ "LDY", false, MODE_IMMEDIATE },
	/* A1 */{ "LDA", false, MODE_INDEXED_INDIRECT },
	/* A2 */{ "LDX", false, MODE_IMMEDIATE },
	/* A3 */{ "lax", true,  MODE_INDEXED_INDIRECT },
	/* A4 */{ "LDY", false, MODE_ZERO_PAGE },
	/* A5 */{ "LDA", false, MODE_ZERO_PAGE },
	/* A6 */{ "LDX", false, MODE_ZERO_PAGE },
	/* A7 */{ "lax", true,  MODE_ZERO_PAGE },
	/* A8 */{ "TAY", false, MODE_IMPLIED },
	/* A9 */{ "LDA", false, MODE_IMMEDIATE },
	/* AA */{ "TAX", false, MODE_IMPLIED },
	/* AB */{ "atx", true,  MODE_IMPLIED },
	/* AC */{ "LDY", false, MODE_ABSOLUTE },
	/* AD */{ "LDA", false, MODE_ABSOLUTE },
	/* AE */{ "LDX", false, MODE_ABSOLUTE },
	/* AF */{ "lax", true,  MODE_ABSOLUTE },
	/* B0 */{ "BCS", false, MODE_RELATIVE },
	/* B1 */{ "LDA", false, MODE_INDIRECT_INDEXED },
	/* B2 */{ "kil", true,  MODE_IMPLIED },
	/* B3 */{ "lax", true,  MODE_INDIRECT_INDEXED },
	/* B4 */{ "LDY", false, MODE_ZERO_PAGE_X },
	/* B5 */{ "LDA", false, MODE_ZERO_PAGE_X },
	/* B6 */{ "LDX", false, MODE_ZERO_PAGE_Y },
	/* B7 */{ "lax", true,  MODE_ZERO_PAGE_Y },
	/* B8 */{ "CLV", false, MODE_IMPLIED },
	/* B9 */{ "LDA", false, MODE_ABSOLUTE_Y },
	/* BA */{ "TSX", false, MODE_IMPLIED },
	/* BB */{ "lar", true,  MODE_ABSOLUTE_Y },
	/* BC */{ "LDY", false, MODE_ABSOLUTE_X },
	/* BD */{ "LDA", false, MODE_ABSOLUTE_X },
	/* BE */{ "LDX", false, MODE_ABSOLUTE_Y },
	/* BF */{ "lax", true,  MODE_ABSOLUTE_Y },
	/* C0 */{ "CPY", false, MODE_IMMEDIATE },
	/* C1 */{ "CMP", false, MODE_INDEXED_INDIRECT },
	/* C2 */{ "dop", true,  MODE_IMMEDIATE },
	/* C3 */{ "dcp", true,  MODE_INDEXED_INDIRECT },
	/* C4 */{ "CPY", false, MODE_ZERO_PAGE },
	/* C5 */{ "CMP", false, MODE_ZERO_PAGE },
	/* C6 */{ "DEC", false, MODE_ZERO_PAGE },
	/* C7 */{ "dcp", true,  MODE_ZERO_PAGE },
	/* C8 */{ "INY", false, MODE_IMPLIED },
	/* C9 */{ "CMP", false, MODE_IMMEDIATE },
	/* CA */{ "DEX", false, MODE_IMPLIED },
	/* CB */{ "axs", true,  MODE_IMMEDIATE },
	/* CC */{ "CPY", false, MODE_ABSOLUTE },
	/* CD */{ "CMP", false, MODE_ABSOLUTE },
	/* CE */{ "DEC", false, MODE_ABSOLUTE },
	/* CF */{ "dcp", true,  MODE_ABSOLUTE },
	/* D0 */{ "BNE", false, MODE_RELATIVE },
	/* D1 */{ "CMP", false, MODE_INDIRECT_INDEXED },
	/* D2 */{ "kil", true,  MODE_IMPLIED },
	/* D3 */{ "dcp", true,  MODE_INDIRECT_INDEXED },
	/* D4 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* D5 */{ "CMP", false, MODE_ZERO_PAGE_X },
	/* D6 */{ "DEC", false, MODE_ZERO_PAGE_X },
	/* D7 */{ "dcp", true,  MODE_ZERO_PAGE_X },
	/* D8 */{ "CLD", false, MODE_IMPLIED },
	/* D9 */{ "CMP", false, MODE_ABSOLUTE_Y },
	/* DA */{ "nop", true,  MODE_IMPLIED },
	/* DB */{ "dcp", true,  MODE_ABSOLUTE_Y },
	/* DC */{ "top", true,  MODE_ABSOLUTE_X },
	/* DD */{ "CMP", false, MODE_ABSOLUTE_X },
	/* DE */{ "DEC", false, MODE_ABSOLUTE_X },
	/* DF */{ "dcp", true,  MODE_ABSOLUTE_X },
	/* E0 */{ "CPX", false, MODE_IMMEDIATE },
	/* E1 */{ "SBC", false, MODE_INDEXED_INDIRECT },
	/* E2 */{ "dop", true,  MODE_IMMEDIATE },
	/* E3 */{ "isc", true,  MODE_INDEXED_INDIRECT },
	/* E4 */{ "CPX", false, MODE_ZERO_PAGE },
	/* E5 */{ "SBC", false, MODE_ZERO_PAGE },
	/* E6 */{ "INC", false, MODE_ZERO_PAGE },
	/* E7 */{ "isc", true,  MODE_ZERO_PAGE },
	/* E8 */{ "INX", false, MODE_IMPLIED },
	/* E9 */{ "SBC", false, MODE_IMMEDIATE },
	/* EA */{ "NOP", false, MODE_IMPLIED },
	/* EB */{ "sbc", true,  MODE_IMMEDIATE },
	/* EC */{ "CPX", false, MODE_ABSOLUTE },
	/* ED */{ "SBC", false, MODE_ABSOLUTE },
	/* EE */{ "INC", false, MODE_ABSOLUTE },
	/* EF */{ "isc", true,  MODE_ABSOLUTE },
	/* F0 */{ "BEQ", false, MODE_RELATIVE },
	/* F1 */{ "SBC", false, MODE_INDIRECT_INDEXED },
	/* F2 */{ "kil", true,  MODE_IMPLIED },
	/* F3 */{ "isc", true,  MODE_INDIRECT_INDEXED },
	/* F4 */{ "dop", true,  MODE_ZERO_PAGE_X },
	/* F5 */{ "SBC", false, MODE_ZERO_PAGE_X },
	/* F6 */{ "INC", false, MODE_ZERO_PAGE_X },
	/* F7 */{ "isc", true,  MODE_ZERO_PAGE_X },
	/* F8 */{ "SED", false, MODE_IMPLIED },
	/* F9 */{ "SBC", false, MODE_ABSOLUTE_Y },
	/* FA */{ "nop", true,  MODE_IMPLIED },
	/* FB */{ "isc", true,  MODE_ABSOLUTE_Y },
	/* FC */{ "top", true,  MODE_ABSOLUTE_X },
	/* FD */{ "SBC", false, MODE_ABSOLUTE_X },
	/* FE */{ "INC", false, MODE_ABSOLUTE_X },
	/* FF */{ "isc", true,  MODE_ABSOLUTE_X }
};

static OPCODE tabOpcodeC02[256] =
{
    /* 00 */{ "BRK", false, MODE_IMPLIED },
    /* 01 */{ "ORA", false, MODE_INDEXED_INDIRECT },
    /* 02 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 03 */{ "NOP", true,  MODE_IMPLIED },
    /* 04 */{ "TSB", true,  MODE_ZERO_PAGE },
    /* 05 */{ "ORA", false, MODE_ZERO_PAGE },
    /* 06 */{ "ASL", false, MODE_ZERO_PAGE },
    /* 07 */{ "NOP", true,  MODE_IMPLIED },
    /* 08 */{ "PHP", false, MODE_IMPLIED },
    /* 09 */{ "ORA", false, MODE_IMMEDIATE },
    /* 0A */{ "ASL", false, MODE_ACCUMULATOR },
    /* 0B */{ "NOP", true,  MODE_IMPLIED },
    /* 0C */{ "TSB", true,  MODE_ABSOLUTE },
    /* 0D */{ "ORA", false, MODE_ABSOLUTE },
    /* 0E */{ "ASL", false, MODE_ABSOLUTE },
    /* 0F */{ "NOP", true,  MODE_IMPLIED },
    /* 10 */{ "BPL", false, MODE_RELATIVE },
    /* 11 */{ "ORA", false, MODE_INDIRECT_INDEXED },
    /* 12 */{ "ORA", true,  MODE_ZERO_PAGE_INDIRECT },
    /* 13 */{ "NOP", true,  MODE_IMPLIED },
    /* 14 */{ "TRB", true,  MODE_ZERO_PAGE },
    /* 15 */{ "ORA", false, MODE_ZERO_PAGE_X },
    /* 16 */{ "ASL", false, MODE_ZERO_PAGE_X },
    /* 17 */{ "NOP", true,  MODE_IMPLIED },
    /* 18 */{ "CLC", false, MODE_IMPLIED },
    /* 19 */{ "ORA", false, MODE_ABSOLUTE_Y },
    /* 1A */{ "INA", true,  MODE_ACCUMULATOR },
    /* 1B */{ "NOP", true,  MODE_IMPLIED },
    /* 1C */{ "TRB", true,  MODE_ABSOLUTE },
    /* 1D */{ "ORA", false, MODE_ABSOLUTE_X },
    /* 1E */{ "ASL", false, MODE_ABSOLUTE_X },
    /* 1F */{ "NOP", true,  MODE_IMPLIED },
    /* 20 */{ "JSR", false, MODE_ABSOLUTE },
    /* 21 */{ "AND", false, MODE_INDEXED_INDIRECT },
    /* 22 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 23 */{ "NOP", true,  MODE_IMPLIED },
    /* 24 */{ "BIT", false, MODE_ZERO_PAGE },
    /* 25 */{ "AND", false, MODE_ZERO_PAGE },
    /* 26 */{ "ROL", false, MODE_ZERO_PAGE },
    /* 27 */{ "NOP", true,  MODE_IMPLIED },
    /* 28 */{ "PLP", false, MODE_IMPLIED },
    /* 29 */{ "AND", false, MODE_IMMEDIATE },
    /* 2A */{ "ROL", false, MODE_ACCUMULATOR },
    /* 2B */{ "NOP", true,  MODE_IMPLIED },
    /* 2C */{ "BIT", false, MODE_ABSOLUTE },
    /* 2D */{ "AND", false, MODE_ABSOLUTE },
    /* 2E */{ "ROL", false, MODE_ABSOLUTE },
    /* 2F */{ "NOP", true,  MODE_IMPLIED },
    /* 30 */{ "BMI", false, MODE_RELATIVE },
    /* 31 */{ "AND", false, MODE_INDIRECT_INDEXED },
    /* 32 */{ "AND", true,  MODE_ZERO_PAGE_INDIRECT },
    /* 33 */{ "NOP", true,  MODE_IMPLIED },
    /* 34 */{ "BIT", true,  MODE_ZERO_PAGE_X },
    /* 35 */{ "AND", false, MODE_ZERO_PAGE_X },
    /* 36 */{ "ROL", false, MODE_ZERO_PAGE_X },
    /* 37 */{ "NOP", true,  MODE_IMPLIED },
    /* 38 */{ "SEC", false, MODE_IMPLIED },
    /* 39 */{ "AND", false, MODE_ABSOLUTE_Y },
    /* 3A */{ "DEA", true,  MODE_ACCUMULATOR },
    /* 3B */{ "NOP", true,  MODE_IMPLIED },
    /* 3C */{ "BIT", true,  MODE_ABSOLUTE_X },
    /* 3D */{ "AND", false, MODE_ABSOLUTE_X },
    /* 3E */{ "ROL", false, MODE_ABSOLUTE_X },
    /* 3F */{ "NOP", true,  MODE_IMPLIED },
    /* 40 */{ "RTI", false, MODE_IMPLIED },
    /* 41 */{ "EOR", false, MODE_INDEXED_INDIRECT },
    /* 42 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 43 */{ "NOP", true,  MODE_IMPLIED },
    /* 44 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 45 */{ "EOR", false, MODE_ZERO_PAGE },
    /* 46 */{ "LSR", false, MODE_ZERO_PAGE },
    /* 47 */{ "NOP", true,  MODE_IMPLIED },
    /* 48 */{ "PHA", false, MODE_IMPLIED },
    /* 49 */{ "EOR", false, MODE_IMMEDIATE },
    /* 4A */{ "LSR", false, MODE_ACCUMULATOR },
    /* 4B */{ "NOP", true,  MODE_IMPLIED },
    /* 4C */{ "JMP", false, MODE_ABSOLUTE },
    /* 4D */{ "EOR", false, MODE_ABSOLUTE },
    /* 4E */{ "LSR", false, MODE_ABSOLUTE },
    /* 4F */{ "NOP", true,  MODE_IMPLIED },
    /* 50 */{ "BVC", false, MODE_RELATIVE },
    /* 51 */{ "EOR", false, MODE_INDIRECT_INDEXED },
    /* 52 */{ "EOR", true,  MODE_ZERO_PAGE_INDIRECT },
    /* 53 */{ "NOP", true,  MODE_IMPLIED },
    /* 54 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 55 */{ "EOR", false, MODE_ZERO_PAGE_X },
    /* 56 */{ "LSR", false, MODE_ZERO_PAGE_X },
    /* 57 */{ "NOP", true,  MODE_IMPLIED },
    /* 58 */{ "CLI", false, MODE_IMPLIED },
    /* 59 */{ "EOR", false, MODE_ABSOLUTE_Y },
    /* 5A */{ "PHY", true,  MODE_IMPLIED },
    /* 5B */{ "NOP", true,  MODE_IMPLIED },
    /* 5C */{ "NOP", true,  MODE_IMMEDIATE_WORD },
    /* 5D */{ "EOR", false, MODE_ABSOLUTE_X },
    /* 5E */{ "LSR", false, MODE_ABSOLUTE_X },
    /* 5F */{ "NOP", true,  MODE_IMPLIED },
    /* 60 */{ "RTS", false, MODE_IMPLIED },
    /* 61 */{ "ADC", false, MODE_INDEXED_INDIRECT },
    /* 62 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 63 */{ "NOP", true,  MODE_IMPLIED },
    /* 64 */{ "STZ", true,  MODE_ZERO_PAGE },
    /* 65 */{ "ADC", false, MODE_ZERO_PAGE },
    /* 66 */{ "ROR", false, MODE_ZERO_PAGE },
    /* 67 */{ "NOP", true,  MODE_IMPLIED },
    /* 68 */{ "PLA", false, MODE_IMPLIED },
    /* 69 */{ "ADC", false, MODE_IMMEDIATE },
    /* 6A */{ "ROR", false, MODE_ACCUMULATOR },
    /* 6B */{ "NOP", true,  MODE_IMPLIED },
    /* 6C */{ "JMP", false, MODE_INDIRECT },
    /* 6D */{ "ADC", false, MODE_ABSOLUTE },
    /* 6E */{ "ROR", false, MODE_ABSOLUTE },
    /* 6F */{ "NOP", true,  MODE_IMPLIED },
    /* 70 */{ "BVS", false, MODE_RELATIVE },
    /* 71 */{ "ADC", false, MODE_INDIRECT_INDEXED },
    /* 72 */{ "ADC", true,  MODE_ZERO_PAGE_INDIRECT },
    /* 73 */{ "NOP", true,  MODE_IMPLIED },
    /* 74 */{ "STZ", true,  MODE_ZERO_PAGE_X },
    /* 75 */{ "ADC", false, MODE_ZERO_PAGE_X },
    /* 76 */{ "ROR", false, MODE_ZERO_PAGE_X },
    /* 77 */{ "NOP", true,  MODE_IMPLIED },
    /* 78 */{ "SEI", false, MODE_IMPLIED },
    /* 79 */{ "ADC", false, MODE_ABSOLUTE_Y },
    /* 7A */{ "PLY", true,  MODE_IMPLIED },
    /* 7B */{ "NOP", true,  MODE_IMPLIED },
    /* 7C */{ "JMP", true,  MODE_ABSOLUTE_X },
    /* 7D */{ "ADC", false, MODE_ABSOLUTE_X },
    /* 7E */{ "ROR", false, MODE_ABSOLUTE_X },
    /* 7F */{ "NOP", true,  MODE_IMPLIED },
    /* 80 */{ "BRA", true,  MODE_RELATIVE },
    /* 81 */{ "STA", false, MODE_INDEXED_INDIRECT },
    /* 82 */{ "NOP", true,  MODE_IMMEDIATE },
    /* 83 */{ "NOP", true,  MODE_IMPLIED },
    /* 84 */{ "STY", false, MODE_ZERO_PAGE },
    /* 85 */{ "STA", false, MODE_ZERO_PAGE },
    /* 86 */{ "STX", false, MODE_ZERO_PAGE },
    /* 87 */{ "NOP", true,  MODE_IMPLIED },
    /* 88 */{ "DEY", false, MODE_IMPLIED },
    /* 89 */{ "BIT", true,  MODE_IMMEDIATE },
    /* 8A */{ "TXA", false, MODE_IMPLIED },
    /* 8B */{ "NOP", true,  MODE_IMPLIED },
    /* 8C */{ "STY", false, MODE_ABSOLUTE },
    /* 8D */{ "STA", false, MODE_ABSOLUTE },
    /* 8E */{ "STX", false, MODE_ABSOLUTE },
    /* 8F */{ "NOP", true,  MODE_IMPLIED },
    /* 90 */{ "BCC", false, MODE_RELATIVE },
    /* 91 */{ "STA", false, MODE_INDIRECT_INDEXED },
    /* 92 */{ "STA", true,  MODE_ZERO_PAGE_INDIRECT },
    /* 93 */{ "NOP", true,  MODE_IMPLIED },
    /* 94 */{ "STY", false, MODE_ZERO_PAGE_X },
    /* 95 */{ "STA", false, MODE_ZERO_PAGE_X },
    /* 96 */{ "STX", false, MODE_ZERO_PAGE_Y },
    /* 97 */{ "NOP", true,  MODE_IMPLIED },
    /* 98 */{ "TYA", false, MODE_IMPLIED },
    /* 99 */{ "STA", false, MODE_ABSOLUTE_Y },
    /* 9A */{ "TXS", false, MODE_IMPLIED },
    /* 9B */{ "NOP", true,  MODE_IMPLIED },
    /* 9C */{ "STZ", true,  MODE_ABSOLUTE },
    /* 9D */{ "STA", false, MODE_ABSOLUTE_X },
    /* 9E */{ "STZ", true,  MODE_ABSOLUTE_X },
    /* 9F */{ "NOP", true,  MODE_IMPLIED },
    /* A0 */{ "LDY", false, MODE_IMMEDIATE },
    /* A1 */{ "LDA", false, MODE_INDEXED_INDIRECT },
    /* A2 */{ "LDX", false, MODE_IMMEDIATE },
    /* A3 */{ "NOP", true,  MODE_IMPLIED },
    /* A4 */{ "LDY", false, MODE_ZERO_PAGE },
    /* A5 */{ "LDA", false, MODE_ZERO_PAGE },
    /* A6 */{ "LDX", false, MODE_ZERO_PAGE },
    /* A7 */{ "NOP", true,  MODE_IMPLIED },
    /* A8 */{ "TAY", false, MODE_IMPLIED },
    /* A9 */{ "LDA", false, MODE_IMMEDIATE },
    /* AA */{ "TAX", false, MODE_IMPLIED },
    /* AB */{ "NOP", true,  MODE_IMPLIED },
    /* AC */{ "LDY", false, MODE_ABSOLUTE },
    /* AD */{ "LDA", false, MODE_ABSOLUTE },
    /* AE */{ "LDX", false, MODE_ABSOLUTE },
    /* AF */{ "NOP", true,  MODE_IMPLIED },
    /* B0 */{ "BCS", false, MODE_RELATIVE },
    /* B1 */{ "LDA", false, MODE_INDIRECT_INDEXED },
    /* B2 */{ "LDA", true,  MODE_ZERO_PAGE_INDIRECT },
    /* B3 */{ "NOP", true,  MODE_IMPLIED },
    /* B4 */{ "LDY", false, MODE_ZERO_PAGE_X },
    /* B5 */{ "LDA", false, MODE_ZERO_PAGE_X },
    /* B6 */{ "LDX", false, MODE_ZERO_PAGE_Y },
    /* B7 */{ "NOP", true,  MODE_IMPLIED },
    /* B8 */{ "CLV", false, MODE_IMPLIED },
    /* B9 */{ "LDA", false, MODE_ABSOLUTE_Y },
    /* BA */{ "TSX", false, MODE_IMPLIED },
    /* BB */{ "NOP", true,  MODE_IMPLIED },
    /* BC */{ "LDY", false, MODE_ABSOLUTE_X },
    /* BD */{ "LDA", false, MODE_ABSOLUTE_X },
    /* BE */{ "LDX", false, MODE_ABSOLUTE_Y },
    /* BF */{ "NOP", true,  MODE_IMPLIED },
    /* C0 */{ "CPY", false, MODE_IMMEDIATE },
    /* C1 */{ "CMP", false, MODE_INDEXED_INDIRECT },
    /* C2 */{ "NOP", true,  MODE_IMMEDIATE },
    /* C3 */{ "NOP", true,  MODE_IMPLIED },
    /* C4 */{ "CPY", false, MODE_ZERO_PAGE },
    /* C5 */{ "CMP", false, MODE_ZERO_PAGE },
    /* C6 */{ "DEC", false, MODE_ZERO_PAGE },
    /* C7 */{ "NOP", true,  MODE_IMPLIED },
    /* C8 */{ "INY", false, MODE_IMPLIED },
    /* C9 */{ "CMP", false, MODE_IMMEDIATE },
    /* CA */{ "DEX", false, MODE_IMPLIED },
    /* CB */{ "NOP", true,  MODE_IMPLIED },
    /* CC */{ "CPY", false, MODE_ABSOLUTE },
    /* CD */{ "CMP", false, MODE_ABSOLUTE },
    /* CE */{ "DEC", false, MODE_ABSOLUTE },
    /* CF */{ "NOP", true,  MODE_IMPLIED },
    /* D0 */{ "BNE", false, MODE_RELATIVE },
    /* D1 */{ "CMP", false, MODE_INDIRECT_INDEXED },
    /* D2 */{ "CMP", true,  MODE_ZERO_PAGE_INDIRECT },
    /* D3 */{ "NOP", true,  MODE_IMPLIED },
    /* D4 */{ "NOP", true,  MODE_IMMEDIATE },
    /* D5 */{ "CMP", false, MODE_ZERO_PAGE_X },
    /* D6 */{ "DEC", false, MODE_ZERO_PAGE_X },
    /* D7 */{ "NOP", true,  MODE_IMPLIED },
    /* D8 */{ "CLD", false, MODE_IMPLIED },
    /* D9 */{ "CMP", false, MODE_ABSOLUTE_Y },
    /* DA */{ "PHX", true,  MODE_IMPLIED },
    /* DB */{ "NOP", true,  MODE_IMPLIED },
    /* DC */{ "NOP", true,  MODE_IMMEDIATE_WORD },
    /* DD */{ "CMP", false, MODE_ABSOLUTE_X },
    /* DE */{ "DEC", false, MODE_ABSOLUTE_X },
    /* DF */{ "NOP", true,  MODE_IMPLIED },
    /* E0 */{ "CPX", false, MODE_IMMEDIATE },
    /* E1 */{ "SBC", false, MODE_INDEXED_INDIRECT },
    /* E2 */{ "NOP", true,  MODE_IMMEDIATE },
    /* E3 */{ "NOP", true,  MODE_IMPLIED },
    /* E4 */{ "CPX", false, MODE_ZERO_PAGE },
    /* E5 */{ "SBC", false, MODE_ZERO_PAGE },
    /* E6 */{ "INC", false, MODE_ZERO_PAGE },
    /* E7 */{ "NOP", true,  MODE_IMPLIED },
    /* E8 */{ "INX", false, MODE_IMPLIED },
    /* E9 */{ "SBC", false, MODE_IMMEDIATE },
    /* EA */{ "NOP", false, MODE_IMPLIED },
    /* EB */{ "NOP", true,  MODE_IMPLIED },
    /* EC */{ "CPX", false, MODE_ABSOLUTE },
    /* ED */{ "SBC", false, MODE_ABSOLUTE },
    /* EE */{ "INC", false, MODE_ABSOLUTE },
    /* EF */{ "NOP", true,  MODE_IMPLIED },
    /* F0 */{ "BEQ", false, MODE_RELATIVE },
    /* F1 */{ "SBC", false, MODE_INDIRECT_INDEXED },
    /* F2 */{ "SBC", true,  MODE_ZERO_PAGE_INDIRECT },
    /* F3 */{ "NOP", true,  MODE_IMPLIED },
    /* F4 */{ "NOP", true,  MODE_IMMEDIATE },
    /* F5 */{ "SBC", false, MODE_ZERO_PAGE_X },
    /* F6 */{ "INC", false, MODE_ZERO_PAGE_X },
    /* F7 */{ "NOP", true,  MODE_IMPLIED },
    /* F8 */{ "SED", false, MODE_IMPLIED },
    /* F9 */{ "SBC", false, MODE_ABSOLUTE_Y },
    /* FA */{ "PLX", true,  MODE_IMPLIED },
    /* FB */{ "NOP", true,  MODE_IMPLIED },
    /* FC */{ "NOP", true,  MODE_IMMEDIATE_WORD },
    /* FD */{ "SBC", false, MODE_ABSOLUTE_X },
    /* FE */{ "INC", false, MODE_ABSOLUTE_X },
    /* FF */{ "NOP", true,  MODE_IMPLIED }
};

Cpu6502::Cpu6502(CPU_ENUM cpuType)
{
    m_cpuType = cpuType;
	m_PC = 0;
	m_SR = CPU6502_FLAG_U;
	m_SP = 0xFF;
	m_A = m_X = m_Y = 0;
    m_traceOn = 0;
	m_instructionsSkipped = 0;

	// Compute the BCD lookup table
	for (unsigned int t = 0; t < 256; t++) {
		m_BCDTable[0][t] = ((t >> 4) * 10) + (t & 0x0F);
		m_BCDTable[1][t] = (((t % 100) / 10) << 4) | (t % 10);
		m_BCDTable[2][t] = 0;
		if ((t & 0x0F) >= 0x0A) {
			m_BCDTable[2][t] = 1;
		}
		if (t >= 0xA0) {
			m_BCDTable[2][t] = 1;
		}
	}
}

int Cpu6502::Branch(unsigned char val)
{
	unsigned short OldPC = m_PC;
	m_PC += (char)val;
	if ((OldPC ^ m_PC) & 0xFF00) {
		return 4; // Different page
	}
	else {
		return 3; // Same page
	}
}

void Cpu6502::Aac(unsigned char val)
{
	And(val);
	SetFlagC(m_SR & CPU6502_FLAG_N);
}

void Cpu6502::Aax(unsigned short addr)
{
	WriteByte(addr, m_A & m_X);
}

void Cpu6502::Adc(unsigned char val)
{
	unsigned char oldA = m_A;
	unsigned short sum = ((unsigned short)val + (unsigned short)m_A) + (m_SR & CPU6502_FLAG_C);
    auto lowSum = (unsigned char)sum;
	SetFlagN(lowSum);
	SetFlagV(((lowSum ^ oldA) & 0x80) && ((lowSum ^ val) & 0x80));

	if (m_SR & CPU6502_FLAG_D) {

		// BCD mode
		if (m_BCDTable[2][m_A] || m_BCDTable[2][val]) {

			// bad BCD values
			unsigned short sumBCD, sumLow, sumHigh, lowCarry;
			unsigned char lowA = m_A & 0x0F;
			unsigned char lowVal = val & 0x0F;
			unsigned short highA = (unsigned short)m_A & 0xF0;
			unsigned short highVal = (unsigned short)val & 0xF0;

			// fix low part of register A and val
			if (lowA >= 0x0A) {
				lowA += 0x06;
			}
			else {

				// val is fixed only if register A is a good BCD value
				if (lowVal >= 0x0A) {
					lowVal += 0x06;
				}
			}

			// make the sum of low part
			sumLow = (lowA + lowVal) + (m_SR & CPU6502_FLAG_C);

			// fix BCD value if both parts are valid BCD values
			if ((sumLow >= 0x0A) && (lowA < 0x0A) && (lowVal < 0x0A)) {
				sumLow += 0x06;
			}

			// set carry for low part
			lowCarry = ((sumLow & 0xF0) ? 0x10 : 0);
			sumLow &= 0x0F;

			// fix high part of register A and val
			if (highA >= 0xA0) {
				highA += 0x60;
			}
			else {

				// val is fixed only if register A is a good BCD value
				if (highVal >= 0xA0) {
					highVal += 0x60;
				}
			}

			// make the sum of high part
			sumHigh = highA + highVal + lowCarry;

			// fix BCD value if both parts are valid BCD values
			if ((sumHigh >= 0xA0) && (highA < 0xA0) && (highVal < 0xA0)) {
				sumHigh += 0x60;
			}

			// make the sum and set carry flag.
			sumBCD = sumHigh + sumLow;
			SetFlagC(sumBCD >> 8);

			// fix A for overflow setting.
			m_A = (unsigned char)sumBCD;

		}
		else {

			// good BCD values
			short sumBCD = m_BCDTable[0][m_A] + m_BCDTable[0][val] + (m_SR & CPU6502_FLAG_C);
			SetFlagC(sumBCD > 99);
			m_A = m_BCDTable[1][sumBCD & 0xFF];
		}
	}
	else {

		// binary mode
		SetFlagC(sum >> 8);
		m_A = (unsigned char)sum;
	}
	SetFlagZ(m_A);
}

void Cpu6502::And(unsigned char val)
{
	m_A &= val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Ane(unsigned char val)
{
    // m_A = (((m_A & val) | 0xEE) & (m_A | val)) & m_X;
    m_A &= m_X & val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Arr(unsigned char val)
{
	unsigned char tmpA = m_A & val;
	unsigned char highA = (tmpA >> 4) & 0x0F;

	m_A = Ror(tmpA);

	// Set overflow and carry flag
	SetFlagV((tmpA ^ m_A) & CPU6502_FLAG_V);
	SetFlagC((highA + (highA & 1) > 5));

	// BCD fixup if we are in decimal mode
	if (m_SR & CPU6502_FLAG_D) {
		unsigned char lowA = tmpA & 0x0F;

		// BCD fixup for low part.
		if (lowA + (lowA & 1) > 5) {
			m_A = (m_A & 0xF0) | ((m_A + 0x06) & 0xF);
		}

		// BCD fixup for high part.
		if (m_SR & CPU6502_FLAG_C) {
			m_A += 0x60;
		}
	}
}

unsigned char Cpu6502::Asl(unsigned char val)
{
	SetFlagC(val & 0x80);
	val <<= 1;
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}

void Cpu6502::Asr(unsigned char val)
{
	And(val);
	m_A = Lsr(m_A);
}

void Cpu6502::Atx(unsigned char val)
{
	m_A = m_X = (m_A & val); // not sure
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Axa(unsigned short addr)
{
	WriteByte(addr, m_A & m_X & UNDOC_MASK); // not sure
}

void Cpu6502::Axs(unsigned char val)
{
	unsigned short reg = (unsigned short)(m_A & m_X) - val;

	m_X = (unsigned char)reg;
	SetFlagC(reg < 0x100);
	SetFlagN(m_X);
	SetFlagZ(m_X);
}

int Cpu6502::Bcc(unsigned char val)
{
	if ((m_SR & CPU6502_FLAG_C) == 0) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Bcs(unsigned char val)
{
	if (m_SR & CPU6502_FLAG_C) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Beq(unsigned char val)
{
	if (m_SR & CPU6502_FLAG_Z) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

void Cpu6502::Bit(unsigned char val)
{
	SetFlagN(val);
	SetFlagV(val & CPU6502_FLAG_V);
	SetFlagZ(val & m_A);
}

void Cpu6502::BitImm(unsigned char val)
{
    SetFlagZ(val & m_A);
}

int Cpu6502::Bmi(unsigned char val)
{
	if (m_SR & CPU6502_FLAG_N) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Bne(unsigned char val)
{
	if ((m_SR & CPU6502_FLAG_Z) == 0) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Bpl(unsigned char val)
{
	if ((m_SR & CPU6502_FLAG_N) == 0) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Bvc(unsigned char val)
{
	if ((m_SR & CPU6502_FLAG_V) == 0) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

int Cpu6502::Bvs(unsigned char val)
{
	if (m_SR & CPU6502_FLAG_V) {
		return Branch(val);
	}
	else {
		return 2;
	}
}

void Cpu6502::Brk(void)
{
	SetFlagB(1);
	PushWord(m_PC);
	PushByte(m_SR);
	SetFlagI(1);
	m_PC = ReadWord(CPU6502_VEC_IRQ);
}

void Cpu6502::Clc(void)
{
	SetFlagC(0);
}

void Cpu6502::Cld(void)
{
	SetFlagD(0);
}

void Cpu6502::Cli(void)
{
	SetFlagI(0);
}

void Cpu6502::Clv(void)
{
	SetFlagV(0);
}

void Cpu6502::Cmp(unsigned char val)
{
	unsigned short cmp = (unsigned short)m_A - val;

	SetFlagC(cmp < 0x100);
	SetFlagN((unsigned char)cmp);
	SetFlagZ(cmp & 0xFF);
}

void Cpu6502::Cpx(unsigned char val)
{
	unsigned short cmp = (unsigned short)m_X - val;

	SetFlagC(cmp < 0x100);
	SetFlagN((unsigned char)cmp);
	SetFlagZ(cmp & 0xFF);
}

void Cpu6502::Cpy(unsigned char val)
{
	unsigned short cmp = (unsigned short)m_Y - val;

	SetFlagC(cmp < 0x100);
	SetFlagN((unsigned char)cmp);
	SetFlagZ(cmp & 0xFF);
}

void Cpu6502::Dcp(unsigned short addr, unsigned char val)
{
	unsigned char reg = Dec(val);

	WriteByte(addr, reg);
	Cmp(reg);
}

void Cpu6502::Dea(void)
{
    m_A--;
    SetFlagN(m_A);
    SetFlagZ(m_A);
}

unsigned char Cpu6502::Dec(unsigned char val)
{
	val--;
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}

void Cpu6502::Dex(void)
{
	m_X--;
	SetFlagN(m_X);
	SetFlagZ(m_X);
}

void Cpu6502::Dey(void)
{
	m_Y--;
	SetFlagN(m_Y);
	SetFlagZ(m_Y);
}

void Cpu6502::Eor(unsigned char val)
{
	m_A ^= val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

unsigned char Cpu6502::Inc(unsigned char val)
{
	val++;
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}

void Cpu6502::Ina(void)
{
    m_A++;
    SetFlagN(m_A);
    SetFlagZ(m_A);
}

void Cpu6502::Inx(void)
{
	m_X++;
	SetFlagN(m_X);
	SetFlagZ(m_X);
}

void Cpu6502::Iny(void)
{
	m_Y++;
	SetFlagN(m_Y);
	SetFlagZ(m_Y);
}

void Cpu6502::Isc(unsigned short addr, unsigned char val)
{
	unsigned char reg = Inc(val);

	WriteByte(addr, reg);
	Sbc(reg);
}

void Cpu6502::Jmp(unsigned short addr)
{
	m_PC = addr;
}

void Cpu6502::Jsr(unsigned short addr)
{
	m_PC--; // This really is a 6502 bug
	PushWord(m_PC);
	m_PC = addr;
}

void Cpu6502::Lar(unsigned char val)
{
	m_A = m_X = m_SP = (m_SP & val);
	SetFlagN(m_A);
}

void Cpu6502::Lax(unsigned char val)
{
	m_A = m_X = val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Lda(unsigned char val)
{
	m_A = val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Ldx(unsigned char val)
{
	m_X = val;
	SetFlagN(m_X);
	SetFlagZ(m_X);
}

void Cpu6502::Ldy(unsigned char val)
{
	m_Y = val;
	SetFlagN(m_Y);
	SetFlagZ(m_Y);
}

unsigned char Cpu6502::Lsr(unsigned char val)
{
	SetFlagC(val & 0x01);
	val >>= 1;
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}

void Cpu6502::Ora(unsigned char val)
{
	m_A |= val;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Pha(void)
{
	PushByte(m_A);
}

void Cpu6502::Php(void)
{
	PushByte(m_SR | CPU6502_FLAG_B); // This really is another 6502 bug
}

void Cpu6502::Phx(void)
{
    PushByte(m_X);
}

void Cpu6502::Phy(void)
{
    PushByte(m_Y);
}

void Cpu6502::Pla(void)
{
	m_A = PopByte();
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Plp(void)
{
	m_SR = PopByte();
	SetFlagB(0);
}

void Cpu6502::Plx(void)
{
    m_X = PopByte();
    SetFlagN(m_X);
    SetFlagZ(m_X);
}

void Cpu6502::Ply(void)
{
    m_Y = PopByte();
    SetFlagN(m_Y);
    SetFlagZ(m_Y);
}

void Cpu6502::Rla(unsigned short addr, unsigned char val)
{
	unsigned char reg = Rol(val);

	WriteByte(addr, reg);
	And(reg);
}
               
unsigned char Cpu6502::Rol(unsigned char val)
{
	if (val & 0x80) {
		val <<= 1;
		if (m_SR & CPU6502_FLAG_C) {
			val |= 0x01;
		}
		SetFlagC(1);
	}
	else {
		val <<= 1;
		if (m_SR & CPU6502_FLAG_C) {
			val |= 0x01;
		}
		SetFlagC(0);
	}
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}
               
unsigned char Cpu6502::Ror(unsigned char val)
{
	if (m_SR & CPU6502_FLAG_C) {
		SetFlagC(val & 0x01);
		val >>= 1;
		val |= 0x80;
	}
	else {
		SetFlagC(val & 0x01);
		val >>= 1;
	}
	SetFlagN(val);
	SetFlagZ(val);
	return val;
}

void Cpu6502::Rra(unsigned short addr, unsigned char val)
{
	unsigned char reg = Ror(val);

	WriteByte(addr, reg);
	Adc(reg);
}

void Cpu6502::Rti(void)
{
	m_SR = PopByte();
	m_PC = PopWord();
}

void Cpu6502::Rts(void)
{
	m_PC = PopWord();
	m_PC++;
}

void Cpu6502::Sbc(unsigned char val)
{
	unsigned short dif;
	unsigned char lowDif;
	unsigned char oldA = m_A;

	dif = ((unsigned short)m_A - (unsigned short)val) - ((m_SR & CPU6502_FLAG_C) ? 0 : 1);
	lowDif = (unsigned char)dif;
	SetFlagN(lowDif);
	SetFlagV(((lowDif ^ oldA) & 0x80) && ((oldA ^ val) & 0x80));

	if (m_SR & CPU6502_FLAG_D) {

		// BCD mode
		if (m_BCDTable[2][m_A] || m_BCDTable[2][val]) {

			// bad BCD values
			unsigned short difBCD, difLow, difHigh, lowCarry;
			unsigned char lowA = m_A & 0x0F;
			unsigned char lowVal = val & 0x0F;
			unsigned short highA = (unsigned short)m_A & 0xF0;
			unsigned short highVal = (unsigned short)val & 0xF0;

			// make the dif of low part
			difLow = (lowA - lowVal) - ((m_SR & CPU6502_FLAG_C) ? 0 : 1);

			// fix BCD value
			if (difLow & 0x10) {
				difLow -= 0x06;
			}

			// set carry for low part
			lowCarry = ((difLow & 0xF0) ? 0x10 : 0);
			difLow &= 0x0F;

			// make the dif of high part
			difHigh = highA - highVal - lowCarry;

			// fix BCD value if both parts are valid BCD values
			if (difHigh & 0x100) {
				difHigh -= 0x60;
			}

			// make the dif and set carry flag.
			difBCD = difHigh | difLow;
			SetFlagC((difBCD >> 8) == 0);

			// fix A for overflow setting.
			m_A = (unsigned char)difBCD;
			SetFlagZ(difBCD != 0);
		}
		else {

			// good BCD values
			short difBCD = m_BCDTable[0][m_A] - m_BCDTable[0][val] - ((m_SR & CPU6502_FLAG_C) ? 0 : 1);
			if (difBCD < 0)
				difBCD += 100;
			SetFlagC(m_A >= (val + ((m_SR & CPU6502_FLAG_C) ? 0 : 1)));
			m_A = m_BCDTable[1][difBCD & 0xFF];
			SetFlagZ(m_A);
		}
	}
	else {

		// binary mode
		SetFlagC((dif >> 8) == 0);
		m_A = (unsigned char)dif;
		SetFlagZ(m_A);
	}
}

void Cpu6502::Sec(void)
{
	SetFlagC(1);
}

void Cpu6502::Sed(void)
{
	SetFlagD(1);
}

void Cpu6502::Sei(void)
{
	SetFlagI(1);
}

void Cpu6502::Slo(unsigned short addr, unsigned char val)
{
	unsigned char reg = Asl(val);

	WriteByte(addr, reg);
	Ora(reg);
}

void Cpu6502::Sre(unsigned short addr, unsigned char val)
{
	unsigned char reg = Lsr(val);

	WriteByte(addr, reg);
	Eor(reg);
}

void Cpu6502::Sta(unsigned short addr)
{
	WriteByte(addr, m_A);
}

void Cpu6502::Stx(unsigned short addr)
{
	WriteByte(addr, m_X);
}

void Cpu6502::Sty(unsigned short addr)
{
	WriteByte(addr, m_Y);
}

void Cpu6502::Stz(unsigned short addr)
{
    WriteByte(addr, 0x00);
}

void Cpu6502::Sxa(unsigned short addr)
{
	WriteByte(addr, m_X & UNDOC_MASK); // not sure
}

void Cpu6502::Sya(unsigned short addr)
{
	WriteByte(addr, m_Y & UNDOC_MASK); // not sure
}

void Cpu6502::Tax(void)
{
	m_X = m_A;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Tay(void)
{
	m_Y = m_A;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

unsigned char Cpu6502::Trb(unsigned char val)
{
    SetFlagZ(val & m_A);
    return (m_A ^ 0xFF) & val;
}

unsigned char Cpu6502::Tsb(unsigned char val)
{
    SetFlagZ(val & m_A);
    return m_A | val;
}

void Cpu6502::Tsx(void)
{
	m_X = m_SP;
	SetFlagN(m_X);
	SetFlagZ(m_X);
}

void Cpu6502::Txa(void)
{
	m_A = m_X;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Txs(void)
{
	m_SP = m_X;
}

void Cpu6502::Tya(void)
{
	m_A = m_Y;
	SetFlagN(m_A);
	SetFlagZ(m_A);
}

void Cpu6502::Xas(unsigned short addr)
{
	m_SP = (m_A & m_X);
	WriteByte(addr, (m_SP & UNDOC_MASK)); // not sure
}

int Cpu6502::Step(void)
{
	int nClockCount;
    unsigned char val;
    unsigned short addr;
	char buf[256];

    BuildTrace(buf);
    if (buf[0]) {
        Trace(0, true, "%s", buf);
    }

	switch (ReadByte(m_PC++)) {

	case 0x00: // BRK
		m_PC++;
		nClockCount = 7;
		Brk();
		break;

	case 0x01: // ORA (Zpg,X)
        val = ReadXIndirect(m_PC);
        m_PC++;
        nClockCount = 6;
        Ora(val);
        break;

    case 0x02: // kil or NOP
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            m_PC++;
            nClockCount = 2;
        }
        break;

    case 0x03: // slo (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x04: // dop Zpg or TSB Zpg
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 3;
        }
        else {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            WriteByte(addr, Tsb(val));
        }
        break;

	case 0x05: // ORA Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Ora(val);
		break;

	case 0x06: // ASL Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Asl(val));
		break;

    case 0x07: // slo Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x08: // PHP
		nClockCount = 3;
		Php();
		break;

	case 0x09: // ORA #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Ora(val);
		break;

	case 0x0A: // ASL A
		nClockCount = 2;
		m_A = Asl(m_A);
		break;

    case 0x0B: // aac #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Aac(val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x0C: // top Abs or TSB Abs
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 4;
        }
        else {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            WriteByte(addr, Tsb(val));
        }
        break;

	case 0x0D: // ORA Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Ora(val);
		break;

	case 0x0E: // ASL Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Asl(val));
		break;

    case 0x0F: // slo Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x10: // BPL Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bpl(val);
		break;

	case 0x11: // ORA (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Ora(val);
		break;

    case 0x12: // kil or ORA (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            Ora(val);
        }
        break;

    case 0x13: // slo (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x14: // dop Zpg,X or TRB Zpg
        if (m_cpuType == CPU_6502) {
            val = ReadZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            WriteByte(addr, Trb(val));
        }
        break;

	case 0x15: // ORA Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Ora(val);
		break;

	case 0x16: // ASL Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Asl(val));
		break;

    case 0x17: // slo Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x18: // CLC
		nClockCount = 2;
		Clc();
		break;

	case 0x19: // ORA Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Ora(val);
		break;

    case 0x1A: // nop or INA
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 2;
            Ina();
        }
        break;

    case 0x1B: // slo Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x1C: // top Abs,X or TRB Abs
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 4;
        }
        else {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            WriteByte(addr, Trb(val));
        }
        break;

	case 0x1D: // ORA Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Ora(val);
		break;

	case 0x1E: // ASL Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
        if (m_cpuType == CPU_6502) {
            nClockCount = 7;
        }
        else {
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 7 : 6);
        }
        WriteByte(addr, Asl(val));
		break;

    case 0x1F: // slo Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Slo(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x20: // JSR Abs
		addr = FetchAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 6;
		Jsr(addr);
		break;

	case 0x21: // AND (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		And(val);
		break;

    case 0x22: // kil or NOP
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            m_PC++;
            nClockCount = 2;
        }
        break;

    case 0x23: // rla (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x24: // BIT Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Bit(val);
		break;

	case 0x25: // AND Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		And(val);
		break;

	case 0x26: // ROL Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Rol(val));
		break;

    case 0x27: // rla Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x28: // PLP
		nClockCount = 4;
		Plp();
		break;

	case 0x29: // AND #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		And(val);
		break;

	case 0x2A: // ROL A
		nClockCount = 2;
		m_A = Rol(m_A);
		break;

    case 0x2B: // aac #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Aac(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x2C: // BIT Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Bit(val);
		break;

	case 0x2D: // AND Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		And(val);
		break;

	case 0x2E: // ROL Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Rol(val));
		break;

    case 0x2F: // rla Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x30: // BMI Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bmi(val);
		break;

	case 0x31: // AND (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		And(val);
		break;

    case 0x32: // kil or AND (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            And(val);
        }
        break;

    case 0x33: // rla (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x34: // dop Zpg,X or BIT Zpg,X
        if (m_cpuType == CPU_6502) {
            val = ReadZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            val = ReadZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
            Bit(val);
        }
        break;

	case 0x35: // AND Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		And(val);
		break;

	case 0x36: // ROL Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Rol(val));
		break;

    case 0x37: // rla Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x38: // SEC
		nClockCount = 2;
		Sec();
		break;

	case 0x39: // AND Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		And(val);
		break;

    case 0x3A: // nop or DEA
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 2;
            Dea();
        }
        break;

    case 0x3B: // rla Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x3C: // top Abs,X or BIT Abs,X
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
        }
        else {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
            Bit(val);
        }
        break;

	case 0x3D: // AND Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		And(val);
		break;

	case 0x3E: // ROL Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
        if (m_cpuType == CPU_6502) {
            nClockCount = 7;
        }
        else {
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 7 : 6);
        }
        WriteByte(addr, Rol(val));
		break;

    case 0x3F: // rla Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Rla(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x40: // RTI
		nClockCount = 6;
		Rti();
		break;

	case 0x41: // EOR (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Eor(val);
		break;

    case 0x42: // kil or NOP
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            m_PC++;
            nClockCount = 2;
        }
        break;

    case 0x43: // sre (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x44: // dop Zpg or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPage(m_PC);
            m_PC++;
            nClockCount = 3;
        }
        else {
            m_PC++;
            nClockCount = 3;
        }
        break;

	case 0x45: // EOR Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Eor(val);
		break;

	case 0x46: // LSR Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Lsr(val));
		break;

    case 0x47: // sre Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x48: // PHA
		nClockCount = 3;
		Pha();
		break;

	case 0x49: // EOR #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Eor(val);
		break;

	case 0x4A: // LSR A
		nClockCount = 2;
		m_A = Lsr(m_A);
		break;

    case 0x4B: // asr #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Asr(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x4C: // JMP Abs
		addr = FetchAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 3;
		Jmp(addr);
		break;

	case 0x4D: // EOR Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Eor(val);
		break;

	case 0x4E: // LSR Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Lsr(val));
		break;

    case 0x4F: // sre Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x50: // BVC Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bvc(val);
		break;

	case 0x51: // EOR (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Eor(val);
		break;

    case 0x52: // kil or EOR (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            Eor(val);
        }
        break;

    case 0x53: // sre (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x54: // dop Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            m_PC++;
            nClockCount = 4;
        }
        break;

	case 0x55: // EOR Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Eor(val);
		break;

	case 0x56: // LSR Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Lsr(val));
		break;

    case 0x57: // sre Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x58: // CLI
		nClockCount = 2;
		Cli();
		break;

	case 0x59: // EOR Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Eor(val);
		break;

    case 0x5A: // nop or PHY
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 3;
            Phy();
        }
        break;

    case 0x5B: // sre Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x5C: // top Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 4;
        }
        else {
            m_PC += 2;
            nClockCount = 8;
        }
        break;

	case 0x5D: // EOR Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Eor(val);
		break;

	case 0x5E: // LSR Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
        if (m_cpuType == CPU_6502) {
            nClockCount = 7;
        }
        else {
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 7 : 6);
        }
        WriteByte(addr, Lsr(val));
		break;

    case 0x5F: // sre Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Sre(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x60: // RTS
		Rts();
		nClockCount = 6;
		break;

	case 0x61: // ADC (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Adc(val);
		break;

    case 0x62: // kil or NOP
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            m_PC++;
            nClockCount = 2;
        }
        break;

    case 0x63: // rra (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x64: // dop Zpg or STZ Zpg
        if (m_cpuType == CPU_6502) {
            m_PC++;
            nClockCount = 3;
        }
        else {
            addr = FetchZPage(m_PC);
            m_PC++;
            nClockCount = 3;
            Stz(addr);
        }
        break;

	case 0x65: // ADC Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Adc(val);
		break;

	case 0x66: // ROR Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Ror(val));
		break;

    case 0x67: // rra Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x68: // PLA
		nClockCount = 4;
		Pla();
		break;

	case 0x69: // ADC #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Adc(val);
		break;

	case 0x6A: // ROR A
		nClockCount = 2;
		m_A = Ror(m_A);
		break;

    case 0x6B: // arr #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Arr(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x6C: // JMP Ind
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectBug(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Jmp(addr);
        }
        else {
            addr = FetchIndirect(m_PC);
            m_PC += 2;
            nClockCount = 6;
            Jmp(addr);
        }
        break;

	case 0x6D: // ADC Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Adc(val);
		break;

	case 0x6E: // ROR Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Ror(val));
		break;

    case 0x6F: // rra Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x70: // BVS Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bvs(val);
		break;

	case 0x71: // ADC (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Adc(val);
		break;

    case 0x72: // kil or ADC (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = (m_SR & CPU6502_FLAG_D) ? 6 : 5;
            Adc(val);
        }
        break;

    case 0x73: // rra (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x74: // dop Zpg,X or STZ Zpg,X
        if (m_cpuType == CPU_6502) {
            val = ReadZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            addr = FetchZPageX(m_PC);
            m_PC++;
            nClockCount = 4;
            Stz(addr);
        }
		break;

	case 0x75: // ADC Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Adc(val);
		break;

	case 0x76: // ROR Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Ror(val));
		break;

    case 0x77: // rra Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x78: // SEI
		nClockCount = 2;
		Sei();
		break;

	case 0x79: // ADC Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Adc(val);
		break;

    case 0x7A: // nop or PLY
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 4;
            Ply();
        }
        break;

    case 0x7B: // rra Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x7C: // top Abs,X or JMP (Abs,X)
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 4;
        }
        else {
            addr = FetchAbsoluteXIndirect(m_PC);
            m_PC += 2;
            nClockCount = 6;
            Jmp(addr);
        }
        break;

	case 0x7D: // ADC Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Adc(val);
		break;

	case 0x7E: // ROR Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
        if (m_cpuType == CPU_6502) {
            nClockCount = 7;
        }
        else {
            nClockCount = ((CROSS_PAGE(addr, m_X)) ? 7 : 6);
        }
        WriteByte(addr, Ror(val));
		break;

    case 0x7F: // rra Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Rra(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x80: // dop #Imm or BRA Rel
        if (m_cpuType == CPU_6502) {
            m_PC++;
            nClockCount = 2;
        }
        else {
            val = ReadByte(m_PC);
            m_PC++;
            nClockCount = Branch(val);
        }
        break;

	case 0x81: // STA (Zpg,X)
		addr = FetchXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Sta(addr);
		break;

    case 0x82: // dop #Imm or NOP
        m_PC++;
        nClockCount = 2;
        break;

    case 0x83: // aax (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            m_PC++;
            nClockCount = 6;
            Aax(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x84: // STY Zpg
		addr = FetchZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Sty(addr);
		break;

	case 0x85: // STA Zpg
		addr = FetchZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Sta(addr);
		break;

	case 0x86: // STX Zpg
		addr = FetchZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Stx(addr);
		break;

    case 0x87: // aax Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            m_PC++;
            nClockCount = 3;
            Aax(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x88: // DEY
		nClockCount = 2;
		Dey();
		break;

    case 0x89: // dop #Imm or BIT #Imm
        if (m_cpuType == CPU_6502) {
            m_PC++;
            nClockCount = 2;
        }
        else {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            BitImm(val); // do not update V and N flags
        }
        break;

	case 0x8A: // TXA
		nClockCount = 2;
		Txa();
		break;

    case 0x8B: // ane #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Ane(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x8C: // STY Abs
		addr = FetchAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Sty(addr);
		break;

	case 0x8D: // STA Abs
		addr = FetchAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Sta(addr);
		break;

	case 0x8E: // STX Abs
		addr = FetchAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Stx(addr);
		break;

    case 0x8F: // aax Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            m_PC += 2;
            nClockCount = 4;
            Aax(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x90: // BCC Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bcc(val);
		break;

	case 0x91: // STA (Zpg),Y
		addr = FetchIndirectY(m_PC);
		m_PC++;
		nClockCount = 6;
		Sta(addr);
		break;

    case 0x92: // kil or STA (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            addr = FetchZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            Sta(addr);
        }
        break;

    case 0x93: // axa (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Axa(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x94: // STY Zpg,X
		addr = FetchZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Sty(addr);
		break;

	case 0x95: // STA Zpg,X
		addr = FetchZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Sta(addr);
		break;

	case 0x96: // STX Zpg,Y
		addr = FetchZPageY(m_PC);
		m_PC++;
		nClockCount = 4;
		Stx(addr);
		break;

    case 0x97: // aax Zpg,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageY(m_PC);
            m_PC++;
            nClockCount = 4;
            Aax(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0x98: // TYA
		nClockCount = 2;
		Tya();
		break;

	case 0x99: // STA Abs,Y
		addr = FetchAbsoluteY(m_PC);
		m_PC += 2;
		nClockCount = 5;
		Sta(addr);
		break;

	case 0x9A: // TXS
		nClockCount = 2;
		Txs();
		break;

    case 0x9B: // xas Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Xas(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0x9C: // sya Abs,X or STZ Abs
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Sya(addr);
        }
        else {
            addr = FetchAbsolute(m_PC);
            m_PC += 2;
            nClockCount = 4;
            Stz(addr);
        }
        break;

	case 0x9D: // STA Abs,X
		addr = FetchAbsoluteX(m_PC);
		m_PC += 2;
		nClockCount = 5;
		Sta(addr);
		break;

    case 0x9E: // sxa Abs,Y or STZ Abs,X
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Sxa(addr);
        }
        else {
            addr = FetchAbsoluteX(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Stz(addr);
        }
        break;

    case 0x9F: // axa Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            m_PC += 2;
            nClockCount = 5;
            Axa(addr);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xA0: // LDY #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Ldy(val);
		break;

	case 0xA1: // LDA (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Lda(val);
		break;

	case 0xA2: // LDX #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Ldx(val);
		break;

    case 0xA3: // lax (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadXIndirect(m_PC);
            m_PC++;
            nClockCount = 6;
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xA4: // LDY Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Ldy(val);
		break;

	case 0xA5: // LDA Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Lda(val);
		break;

	case 0xA6: // LDX Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Ldx(val);
		break;

    case 0xA7: // lax Zpg or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPage(m_PC);
            m_PC++;
            nClockCount = 3;
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xA8: // TAY
		nClockCount = 2;
		Tay();
		break;

	case 0xA9: // LDA #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Lda(val);
		break;

	case 0xAA: // TAX
		nClockCount = 2;
		Tax();
		break;

    case 0xAB: // atx #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Atx(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xAC: // LDY Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Ldy(val);
		break;

	case 0xAD: // LDA Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Lda(val);
		break;

	case 0xAE: // LDX Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Ldx(val);
		break;

    case 0xAF: // lax Abs or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadAbsolute(m_PC);
            m_PC += 2;
            nClockCount = 4;
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xB0: // BCS Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bcs(val);
		break;

	case 0xB1: // LDA (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Lda(val);
		break;

    case 0xB2: // kil or LDA (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            Lda(val);
        }
        break;

    case 0xB3: // lax (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xB4: // LDY Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Ldy(val);
		break;

	case 0xB5: // LDA Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Lda(val);
		break;

	case 0xB6: // LDX Zpg,Y
		val = ReadZPageY(m_PC);
		m_PC++;
		nClockCount = 4;
		Ldx(val);
		break;

    case 0xB7: // lax Zpg,Y or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPageY(m_PC);
            m_PC++;
            nClockCount = 4;
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xB8: // CLV
		nClockCount = 2;
		Clv();
		break;

	case 0xB9: // LDA Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Lda(val);
		break;

	case 0xBA: // TSX
		nClockCount = 2;
		Tsx();
		break;

    case 0xBB: // lar Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
            Lar(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xBC: // LDY Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Ldy(val);
		break;

	case 0xBD: // LDA Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Lda(val);
		break;

	case 0xBE: // LDX Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Ldx(val);
		break;

    case 0xBF: // lax Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
            Lax(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xC0: // CPY #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Cpy(val);
		break;

	case 0xC1: // CMP (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Cmp(val);
		break;

    case 0xC2: // dop #Imm or NOP
        m_PC++;
        nClockCount = 2;
        break;

    case 0xC3: // dcp (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xC4: // CPY Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Cpy(val);
		break;

    case 0xC5: // CMP Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Cmp(val);
		break;

	case 0xC6: // DEC Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Dec(val));
		break;

    case 0xC7: // dcp Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xC8: // INY
		nClockCount = 2;
		Iny();
		break;

	case 0xC9: // CMP #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Cmp(val);
		break;

	case 0xCA: // DEX
		nClockCount = 2;
		Dex();
		break;

    case 0xCB: // axs #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Axs(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xCC: // CPY Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Cpy(val);
		break;

	case 0xCD: // CMP Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Cmp(val);
		break;

	case 0xCE: // DEC Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Dec(val));
		break;

    case 0xCF: // dcp Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xD0: // BNE Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Bne(val);
		break;

	case 0xD1: // CMP (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Cmp(val);
		break;

    case 0xD2: // kil or CMP (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = 5;
            Cmp(val);
        }
        break;

    case 0xD3: // dcp (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0xD4: // dop Zpg,Y or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPageY(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            m_PC++;
            nClockCount = 4;
        }
		break;

	case 0xD5: // CMP Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Cmp(val);
		break;

	case 0xD6: // DEC Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Dec(val));
		break;

    case 0xD7: // dcp Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xD8: // CLD
		nClockCount = 2;
		Cld();
		break;

	case 0xD9: // CMP Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Cmp(val);
		break;

    case 0xDA: // nop or PHX
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 3;
            Phx();
        }
        break;

    case 0xDB: // dcp Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0xDC: // top Abs,Y or NOP
        m_PC += 2;
        nClockCount = 4;
        break;

	case 0xDD: // CMP Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Cmp(val);
		break;

	case 0xDE: // DEC Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 7;
		WriteByte(addr, Dec(val));
		break;

    case 0xDF: // dcp Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Dcp(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xE0: // CPX #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Cpx(val);
		break;

	case 0xE1: // SBC (Zpg,X)
		val = ReadXIndirect(m_PC);
		m_PC++;
		nClockCount = 6;
		Sbc(val);
		break;

    case 0xE2: // dop #Imm or NOP
		m_PC++;
		nClockCount = 2;
		break;

    case 0xE3: // isc (Zpg,X) or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchXIndirect(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xE4: // CPX Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Cpx(val);
		break;

	case 0xE5: // SBC Zpg
		val = ReadZPage(m_PC);
		m_PC++;
		nClockCount = 3;
		Sbc(val);
		break;

	case 0xE6: // INC Zpg
		addr = FetchZPage(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 5;
		WriteByte(addr, Inc(val));
		break;

    case 0xE7: // isc Zpg or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPage(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 5;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xE8: // INX
		nClockCount = 2;
		Inx();
		break;

	case 0xE9: // SBC #Imm
		val = ReadImm(m_PC);
		m_PC++;
		nClockCount = 2;
		Sbc(val);
		break;

	case 0xEA: // NOP
		nClockCount = 2;
		break;

    case 0xEB: // sbc #Imm or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadImm(m_PC);
            m_PC++;
            nClockCount = 2;
            Sbc(val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xEC: // CPX Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Cpx(val);
		break;

	case 0xED: // SBC Abs
		val = ReadAbsolute(m_PC);
		m_PC += 2;
		nClockCount = 4;
		Sbc(val);
		break;

	case 0xEE: // INC Abs
		addr = FetchAbsolute(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 6;
		WriteByte(addr, Inc(val));
		break;

    case 0xEF: // isc Abs or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsolute(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 6;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xF0: // BEQ Rel
		val = ReadByte(m_PC);
		m_PC++;
		nClockCount = Beq(val);
		break;

	case 0xF1: // SBC (Zpg),Y
		addr = FetchIndirectY(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 6 : 5);
		Sbc(val);
		break;

    case 0xF2: // kil or SBC (Zpg)
        if (m_cpuType == CPU_6502) {
            nClockCount = 0;
        }
        else {
            val = ReadZPageIndirect(m_PC);
            m_PC++;
            nClockCount = (m_SR & CPU6502_FLAG_D) ? 6 : 5;
            Sbc(val);
        }
        break;

    case 0xF3: // isc (Zpg),Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchIndirectY(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 8;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0xF4: // dop Zpg,Y or NOP
        if (m_cpuType == CPU_6502) {
            val = ReadZPageY(m_PC);
            m_PC++;
            nClockCount = 4;
        }
        else {
            m_PC++;
            nClockCount = 4;
        }
		break;

	case 0xF5: // SBC Zpg,X
		val = ReadZPageX(m_PC);
		m_PC++;
		nClockCount = 4;
		Sbc(val);
		break;

	case 0xF6: // INC Zpg,X
		addr = FetchZPageX(m_PC);
		val = ReadByte(addr);
		m_PC++;
		nClockCount = 6;
		WriteByte(addr, Inc(val));
		break;

    case 0xF7: // isc Zpg,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchZPageX(m_PC);
            val = ReadByte(addr);
            m_PC++;
            nClockCount = 6;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

	case 0xF8: // SED
		nClockCount = 2;
		Sed();
		break;

	case 0xF9: // SBC Abs,Y
		addr = FetchAbsoluteY(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_Y)) ? 5 : 4);
		Sbc(val);
		break;

    case 0xFA: // nop or PLX
        if (m_cpuType == CPU_6502) {
            nClockCount = 2;
        }
        else {
            nClockCount = 4;
            Plx();
        }
        break;

    case 0xFB: // isc Abs,Y or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteY(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;

    case 0xFC: // top Abs,Y or NOP
		m_PC += 2;
		nClockCount = 4;
		break;

	case 0xFD: // SBC Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = ((CROSS_PAGE(addr, m_X)) ? 5 : 4);
		Sbc(val);
		break;

	case 0xFE: // INC Abs,X
		addr = FetchAbsoluteX(m_PC);
		val = ReadByte(addr);
		m_PC += 2;
		nClockCount = 7;
		WriteByte(addr, Inc(val));
		break;

    case 0xFF: // isc Abs,X or NOP
        if (m_cpuType == CPU_6502) {
            addr = FetchAbsoluteX(m_PC);
            val = ReadByte(addr);
            m_PC += 2;
            nClockCount = 7;
            Isc(addr, val);
        }
        else {
            nClockCount = 1;
        }
        break;
	}
	return nClockCount;
}

int Cpu6502::GetOpCodeLength(unsigned char opCode)
{
    MODE_ENUM wMode = m_cpuType == CPU_6502 ? tabOpcode02[opCode].wMode : tabOpcodeC02[opCode].wMode;
    switch (wMode) {
	case MODE_IMMEDIATE:
	case MODE_ZERO_PAGE:
	case MODE_INDEXED_INDIRECT:
	case MODE_INDIRECT_INDEXED:
	case MODE_ZERO_PAGE_X:
	case MODE_ZERO_PAGE_Y:
	case MODE_RELATIVE:
		return 2;
	case MODE_ABSOLUTE:
	case MODE_ABSOLUTE_X:
	case MODE_ABSOLUTE_Y:
	case MODE_INDIRECT:
		return 3;
	default:
		return 1;
	}
}

char *Cpu6502::GetAddressLabel(unsigned short addr)
{
	static char buffer[6];
	sprintf(buffer, "$%04X", ((int)addr) & 0xFFFF);
	return buffer;
}

char *Cpu6502::GetAddressOrLabel(unsigned short addr)
{
	char *label = GetAddressLabel(addr);
    if (label != nullptr) {
		return label;
	}
	else {
		return Cpu6502::GetAddressLabel(addr);
	}
}

char *Cpu6502::GetAddressLabelAllBanks(unsigned short addr)
{
    // default implementation when no bank system is available
    return GetAddressOrLabel(addr);
}

char *Cpu6502::GetAddressOrLabelAllBanks(unsigned short addr)
{
    char *label = GetAddressLabelAllBanks(addr);
    if (label != nullptr) {
        return label;
    }
    else {
        return Cpu6502::GetAddressLabelAllBanks(addr);
    }
}

unsigned short Cpu6502::BuildTrace(char *buffer)
{
    char *p = buffer;
    *p = 0;
	if (! m_traceOn) {
		m_instructionsSkipped = 0;
		return 0xFFFF;
	}
    if (IsAddressSkipped(m_PC)) {
		m_instructionsSkipped++;
		return 0xFFFF;
	}
    unsigned char opCode = ReadByte(m_PC);
	int lenOpCode = GetOpCodeLength(opCode);
	sprintf(p, "A=%02X X=%02X Y=%02X P=%02X SP=%02X  ", ((int)m_A) & 0xFF, ((int)m_X) & 0xFF, ((int)m_Y) & 0xFF, ((int)m_SR) & 0xFF, ((int)m_SP) & 0xFF);
	p += strlen(p);
	sprintf(p, "%04X:", ((int)m_PC) & 0xFFFF);
	p += strlen(p);
	unsigned char opCodes[3];
	for (int i = 0; i < lenOpCode; i++) {
		opCodes[i] = ReadByte(m_PC + i);
		sprintf(p, "%02X ", ((int)opCodes[i]) & 0xFF);
		p += 3;
	}
	for (int i = lenOpCode; i < 3; i++) {
        strcpy(p, "-- ");
		p += 3;
	}
	char *label = GetAddressLabel(m_PC);
	int lenLabel = 0;
    if (label != nullptr) {
		strcpy(p, label);
		lenLabel = strlen(label);
		p += lenLabel;
        *p++ = ':';
        lenLabel++;
	}
	for (int i = lenLabel; i < 17; i++) {
		*p++ = ' ';
	}
    const char *opCodeName = m_cpuType == CPU_6502 ? tabOpcode02[opCode].szName : tabOpcodeC02[opCode].szName;
	strcpy(p, opCodeName);
	p += 3;
	*p++ = ' ';
	unsigned short addr = 0xFFFF;
    char *secondLabel = nullptr;
    char *thirdLabel = nullptr;
    MODE_ENUM wMode = m_cpuType == CPU_6502 ? tabOpcode02[opCode].wMode : tabOpcodeC02[opCode].wMode;
    switch (wMode) {
	case MODE_IMMEDIATE:
		sprintf(p, "#$%02X ", ((int)opCodes[1]) & 0xFF);
		p += strlen(p);
		break;
	case MODE_ZERO_PAGE:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_INDEXED_INDIRECT:
		secondLabel = GetAddressOrLabel(addr);
		*p++ = '(';
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x)");
		p += strlen(p);
		addr = ReadWord(addr + m_X);
		break;
	case MODE_INDIRECT_INDEXED:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
		secondLabel = GetAddressOrLabel(addr);
		*p++ = '(';
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, "),y");
		p += strlen(p);
		addr = ReadWord(addr) + m_Y;
		break;
	case MODE_ZERO_PAGE_X:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x");
		p += strlen(p);
		addr += m_X;
		break;
	case MODE_ZERO_PAGE_Y:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",y");
		p += strlen(p);
		addr += m_Y;
		break;
	case MODE_RELATIVE:
		secondLabel = GetAddressOrLabel(m_PC + 2 + (char)opCodes[1]);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_ABSOLUTE:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_ABSOLUTE_X:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x");
		p += strlen(p);
		addr += m_X;
		break;
	case MODE_ABSOLUTE_Y:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
		secondLabel = GetAddressOrLabel(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",y");
		p += strlen(p);
		addr += m_Y;
		break;
	case MODE_INDIRECT:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
		*p++ = '(';
		secondLabel = GetAddressOrLabel(addr);
        if (secondLabel != nullptr) {
			strcpy(p, secondLabel);
		}
		else {
			sprintf(p, "$%04X", ((int)addr) & 0xFFFF);
		}
		p += strlen(p);
		*p++ = ')';
		break;
    case MODE_ZERO_PAGE_INDIRECT:
        addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabel(addr);
        *p++ = '(';
        strcpy(p, secondLabel);
        p += strlen(p);
        strcpy(p, ")");
        p += strlen(p);
        addr = ReadWord(addr);
        break;
    case MODE_ZERO_PAGE_RELATIVE:
        addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabel(addr);
        strcpy(p, secondLabel);
        p += strlen(p);
        strcpy(p, ",");
        p += strlen(p);
        thirdLabel = GetAddressOrLabel(m_PC + 2 + (char)opCodes[2]);
        strcpy(p, thirdLabel);
        p += strlen(p);
        break;
    case MODE_IMMEDIATE_WORD:
        sprintf(p, "#$%04X ", (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00));
        p += strlen(p);
        break;
    default:
		break;
	}
	*p = 0;
	if (m_instructionsSkipped) {
        sprintf(p, " ; %d instructions skipped", m_instructionsSkipped);
	}
    // The call to ReadByte is commented because it would be a second access for display purpose only and
    // this access may trigger something if address is a register (Port B for example)
    /*
	if (addr != 0xFFFF) {
		unsigned char val = ReadByte(addr);
        sprintf(p, " ; $%04X=$%02X", ((int)addr) & 0xFFFF, ((int)val) & 0xFF);
	}
    */
	m_instructionsSkipped = 0;
    return addr;
}

int Cpu6502::BuildInstruction(char *buffer, unsigned char *data, int lenData, unsigned short address)
{
    char *p = buffer;
    *p = 0;
    if (lenData < 1) {
    	return -1;
    }
    unsigned char opCode = data[0];
	int lenOpCode = GetOpCodeLength(opCode);
	if (lenData < lenOpCode) {
    	return -1;
	}
    sprintf(p, "$%04X: ", ((int)address) & 0xFFFF);
	p += strlen(p);
	unsigned char opCodes[3];
	for (int i = 0; i < lenOpCode; i++) {
		opCodes[i] = data[i];
		sprintf(p, "%02X ", ((int)opCodes[i]) & 0xFF);
		p += 3;
	}
	for (int i = lenOpCode; i < 3; i++) {
        strcpy(p, "-- ");
		p += 3;
	}
	char *label = GetAddressLabel(address);
	int lenLabel = 0;
    if (label != nullptr) {
		strcpy(p, label);
		lenLabel = strlen(label);
		p += lenLabel;
        *p++ = ':';
        lenLabel++;
    }
	for (int i = lenLabel; i < 17; i++) {
		*p++ = ' ';
	}
    const char *opCodeName = m_cpuType == CPU_6502 ? tabOpcode02[opCode].szName : tabOpcodeC02[opCode].szName;
	strcpy(p, opCodeName);
	p += 3;
	*p++ = ' ';
	unsigned short addr = 0xFFFF;
    char *secondLabel = nullptr;
    char *thirdLabel = nullptr;
    MODE_ENUM wMode = m_cpuType == CPU_6502 ? tabOpcode02[opCode].wMode : tabOpcodeC02[opCode].wMode;
    switch (wMode) {
	case MODE_IMMEDIATE:
		sprintf(p, "#$%02X ", ((int)opCodes[1]) & 0xFF);
		p += strlen(p);
		break;
	case MODE_ZERO_PAGE:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_INDEXED_INDIRECT:
        secondLabel = GetAddressOrLabelAllBanks(addr);
		*p++ = '(';
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x)");
		p += strlen(p);
		break;
	case MODE_INDIRECT_INDEXED:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
		*p++ = '(';
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, "),y");
		p += strlen(p);
		break;
	case MODE_ZERO_PAGE_X:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x");
		p += strlen(p);
		break;
	case MODE_ZERO_PAGE_Y:
		addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",y");
		p += strlen(p);
		break;
	case MODE_RELATIVE:
        secondLabel = GetAddressOrLabelAllBanks(address + 2 + (char)opCodes[1]);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_ABSOLUTE:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		break;
	case MODE_ABSOLUTE_X:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",x");
		p += strlen(p);
		break;
	case MODE_ABSOLUTE_Y:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
        secondLabel = GetAddressOrLabelAllBanks(addr);
		strcpy(p, secondLabel);
		p += strlen(p);
		strcpy(p, ",y");
		p += strlen(p);
		break;
	case MODE_INDIRECT:
		addr = (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00);
		*p++ = '(';
        secondLabel = GetAddressOrLabelAllBanks(addr);
        if (secondLabel != nullptr) {
			strcpy(p, secondLabel);
		}
		else {
			sprintf(p, "$%04X", ((int)addr) & 0xFFFF);
		}
		p += strlen(p);
		*p++ = ')';
		break;
    case MODE_ZERO_PAGE_INDIRECT:
        addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
        *p++ = '(';
        strcpy(p, secondLabel);
        p += strlen(p);
        strcpy(p, ")");
        p += strlen(p);
        break;
    case MODE_ZERO_PAGE_RELATIVE:
        addr = ((unsigned short)opCodes[1]) & 0x00FF;
        secondLabel = GetAddressOrLabelAllBanks(addr);
        strcpy(p, secondLabel);
        p += strlen(p);
        strcpy(p, ",");
        p += strlen(p);
        thirdLabel = GetAddressOrLabelAllBanks(address + 2 + (char)opCodes[2]);
        strcpy(p, thirdLabel);
        p += strlen(p);
        break;
    case MODE_IMMEDIATE_WORD:
        sprintf(p, "#$%04X ", (((unsigned short)opCodes[1]) & 0x00FF) | ((((unsigned short)opCodes[2]) << 8) & 0xFF00));
        p += strlen(p);
        break;
    default:
		break;
	}
	*p = 0;
    return lenOpCode;
}

int Cpu6502::Reset(void)
{
	SetFlagB(0);
	SetFlagI(1);
	SetFlagD(0);
	m_PC = ReadWord(CPU6502_VEC_RESET);
	return 7;
}

int Cpu6502::Nmi(void)
{
	PushWord(m_PC);
	PushByte(m_SR);
	SetFlagI(1);
	m_PC = ReadWord(CPU6502_VEC_NMI);
	return 7;
}

int Cpu6502::Irq(void)
{
	int nClockCount;

	if (m_SR & CPU6502_FLAG_I) {
		nClockCount = 0;
	}
	else {
		SetFlagB(0);
		PushWord(m_PC);
		PushByte(m_SR);
		SetFlagI(1);
		m_PC = ReadWord(CPU6502_VEC_IRQ);
		nClockCount = 7;
	}
	return nClockCount;
}
