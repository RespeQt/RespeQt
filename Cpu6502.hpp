//
// Cpu6502 class
// (c) 1996 Eric BACHER
//
// This class simulates a 6502 CPU.
// All undocumented op-codes are implemented
//

#ifndef CPU6502_HPP
#define CPU6502_HPP 1

// get Low/High byte of a word.
#define LO_BYTE(val)          ((unsigned char) ((val) & 0xff))
#define HI_BYTE(val)          ((unsigned char) (((val) >> 8) & 0xff))
#define MAKE_WORD(l, h)       ((unsigned short) ((l) | ((h) << 8)))

// vectors for 6502
const unsigned short CPU6502_VEC_NMI   = 0xFFFA;
const unsigned short CPU6502_VEC_RESET = 0xFFFC;
const unsigned short CPU6502_VEC_IRQ   = 0xFFFE;

// flags for status register
const unsigned char  CPU6502_FLAG_C    = 0x01;
const unsigned char  CPU6502_FLAG_Z    = 0x02;
const unsigned char  CPU6502_FLAG_I    = 0x04;
const unsigned char  CPU6502_FLAG_D    = 0x08;
const unsigned char  CPU6502_FLAG_B    = 0x10;
const unsigned char  CPU6502_FLAG_U    = 0x20;
const unsigned char  CPU6502_FLAG_V    = 0x40;
const unsigned char  CPU6502_FLAG_N    = 0x80;

typedef enum MODE_ENUM
{
    // 6502
	MODE_IMMEDIATE,
	MODE_ABSOLUTE,
	MODE_ZERO_PAGE,
	MODE_ACCUMULATOR,
	MODE_IMPLIED,
	MODE_INDEXED_INDIRECT,
	MODE_INDIRECT_INDEXED,
	MODE_ZERO_PAGE_X,
	MODE_ZERO_PAGE_Y,
	MODE_ABSOLUTE_X,
	MODE_ABSOLUTE_Y,
	MODE_RELATIVE,
    MODE_INDIRECT,
    // 65C02
    MODE_ZERO_PAGE_INDIRECT,
    MODE_ZERO_PAGE_RELATIVE,
    MODE_IMMEDIATE_WORD // for 3-byte NOP
} MODE_ENUM;

typedef struct OPCODE
{
    const char		szName[4];
	bool			bIllegal;
	MODE_ENUM		wMode;
} OPCODE;

typedef enum CPU_ENUM
{
    CPU_6502,
    CPU_65C02
} CPU_ENUM;

// This class implements 6502 CPU emulation.
// It must be derived because ReadByte and WriteByte are pure virtual functions.
class Cpu6502 {

private:

    // cpu type
    CPU_ENUM				m_cpuType;

	// registers
	unsigned short			m_PC;
	unsigned char			m_SR;
	unsigned char			m_SP;
	unsigned char			m_A;
	unsigned char			m_X;
	unsigned char			m_Y;

	// BCD loopkup table
	unsigned char			m_BCDTable[3][256];

    // disassemble instructions
    bool					m_traceOn;
    int						m_instructionsSkipped;

	// get address depending on addressing mode
	inline unsigned short	FetchZPage(unsigned short addr) { return (unsigned short)ReadByte(addr); }
	inline unsigned short	FetchAbsolute(unsigned short addr) { return ReadWord(addr); }
	inline unsigned short	FetchAbsoluteX(unsigned short addr) { return ReadWord(addr) + m_X; }
	inline unsigned short	FetchAbsoluteY(unsigned short addr) { return ReadWord(addr) + m_Y; }
    inline unsigned short	FetchZPageX(unsigned short addr) { return (unsigned short)(ReadByte(addr) + m_X); }
    inline unsigned short	FetchZPageY(unsigned short addr) { return (unsigned short)(ReadByte(addr) + m_Y); }
	inline unsigned short	FetchXIndirect(unsigned short addr) { return ReadWord((unsigned char)(ReadByte(addr) + m_X)); }
	inline unsigned short	FetchIndirectY(unsigned short addr) { return ReadWord((unsigned short)ReadByte(addr)) + m_Y; }
    inline unsigned short	FetchIndirectBug(unsigned short addr) { return ReadWordBug(ReadWord(addr)); }
    inline unsigned short	FetchIndirect(unsigned short addr) { return ReadWord(ReadWord(addr)); }
    inline unsigned short	FetchZPageIndirect(unsigned short addr) { return ReadWord((unsigned char)ReadByte(addr)); }
    inline unsigned short	FetchAbsoluteXIndirect(unsigned short addr) { return ReadWord((unsigned short)(ReadWord(addr) + m_X)); }

	// get value depending on addressing mode
	inline unsigned char	ReadImm(unsigned short addr) { return ReadByte(addr); }
	inline unsigned char	ReadZPage(unsigned short addr) { return ReadByte(FetchZPage(addr)); }
	inline unsigned char	ReadAbsolute(unsigned short addr) { return ReadByte(FetchAbsolute(addr)); }
	inline unsigned char	ReadAbsoluteX(unsigned short addr) { return ReadByte(FetchAbsoluteX(addr)); }
	inline unsigned char	ReadAbsoluteY(unsigned short addr) { return ReadByte(FetchAbsoluteY(addr)); }
	inline unsigned char	ReadZPageX(unsigned short addr) { return ReadByte(FetchZPageX(addr)); }
	inline unsigned char	ReadZPageY(unsigned short addr) { return ReadByte(FetchZPageY(addr)); }
	inline unsigned char	ReadXIndirect(unsigned short addr) { return ReadByte(FetchXIndirect(addr)); }
	inline unsigned char	ReadIndirectY(unsigned short addr) { return ReadByte(FetchIndirectY(addr)); }
    inline unsigned char	ReadZPageIndirect(unsigned short addr) { return ReadByte(FetchZPageIndirect(addr)); }

	// modify m_PC with relative branch
	inline int				Branch(unsigned char val);

	// opcode functions
	inline void				Aac(unsigned char val);
	inline void				Aax(unsigned short addr);
	inline void				Adc(unsigned char val);
	inline void				And(unsigned char val);
	inline void				Ane(unsigned char val);
	inline void				Arr(unsigned char val);
	inline unsigned char	Asl(unsigned char val);
	inline void				Asr(unsigned char val);
	inline void				Atx(unsigned char val);
	inline void				Axa(unsigned short addr);
	inline void				Axs(unsigned char val);
	inline int				Bcc(unsigned char val);
	inline int				Bcs(unsigned char val);
	inline int				Beq(unsigned char val);
	inline void				Bit(unsigned char val);
    inline void				BitImm(unsigned char val);
    inline int				Bmi(unsigned char val);
	inline int				Bne(unsigned char val);
	inline int				Bpl(unsigned char val);
	inline int				Bvc(unsigned char val);
	inline int				Bvs(unsigned char val);
	inline void				Brk(void);
	inline void				Clc(void);
	inline void				Cld(void);
	inline void				Cli(void);
	inline void				Clv(void);
	inline void				Cmp(unsigned char val);
	inline void				Cpx(unsigned char val);
	inline void				Cpy(unsigned char val);
	inline void				Dcp(unsigned short addr, unsigned char val);
	inline unsigned char	Dec(unsigned char val);
    inline void				Dea(void);
    inline void				Dex(void);
	inline void				Dey(void);
	inline void				Eor(unsigned char val);
	inline unsigned char	Inc(unsigned char val);
    inline void				Ina(void);
    inline void				Inx(void);
	inline void				Iny(void);
	inline void				Isc(unsigned short addr, unsigned char val);
	inline void				Jmp(unsigned short addr);
	inline void				Jsr(unsigned short addr);
	inline void				Lar(unsigned char val);
	inline void				Lax(unsigned char val);
	inline void				Lda(unsigned char val);
	inline void				Ldx(unsigned char val);
	inline void				Ldy(unsigned char val);
	inline unsigned char	Lsr(unsigned char val);
	inline void				Ora(unsigned char val);
	inline void				Pha(void);
	inline void				Php(void);
    inline void				Phx(void);
    inline void				Phy(void);
    inline void				Pla(void);
	inline void				Plp(void);
    inline void				Plx(void);
    inline void				Ply(void);
    inline void				Rla(unsigned short addr, unsigned char val);
	inline unsigned char	Rol(unsigned char val);
	inline unsigned char	Ror(unsigned char val);
	inline void				Rra(unsigned short addr, unsigned char val);
	inline void				Rti(void);
	inline void				Rts(void);
	inline void				Sbc(unsigned char val);
	inline void				Sec(void);
	inline void				Sed(void);
	inline void				Sei(void);
	inline void				Slo(unsigned short addr, unsigned char val);
	inline void				Sta(unsigned short addr);
	inline void				Stx(unsigned short addr);
	inline void				Sty(unsigned short addr);
    inline void				Stz(unsigned short addr);
    inline void				Sre(unsigned short addr, unsigned char val);
	inline void				Sxa(unsigned short addr);
	inline void				Sya(unsigned short addr);
	inline void				Tax(void);
	inline void				Tay(void);
    inline unsigned char	Trb(unsigned char val);
    inline unsigned char	Tsb(unsigned char val);
    inline void				Tsx(void);
	inline void				Txa(void);
	inline void				Txs(void);
	inline void				Tya(void);
	inline void				Xas(unsigned short addr);

	// find the length of an op-code
	inline int				GetOpCodeLength(unsigned char opCode);

	// get label for a given address
	inline char				*GetAddressOrLabel(unsigned short addr);
    inline char				*GetAddressOrLabelAllBanks(unsigned short addr);

public:

	// constructors and destructor
                            Cpu6502(CPU_ENUM cpuType);
    virtual					~Cpu6502() = default;

	// return register value
	inline unsigned short	GetPC(void) { return m_PC; }
	inline unsigned char	GetSR(void) { return m_SR; }
	inline unsigned char	GetSP(void) { return m_SP; }
	inline unsigned char	GetA(void) { return m_A; }
	inline unsigned char	GetX(void) { return m_X; }
	inline unsigned char	GetY(void) { return m_Y; }

	// set register value
	inline void				SetPC(unsigned short PC) { m_PC = PC; }
	inline void				SetSR(unsigned char SR) { m_SR = SR; }
	inline void				SetSP(unsigned char SP) { m_SP = SP; }
	inline void				SetA(unsigned char A) { m_A = A; }
	inline void				SetX(unsigned char X) { m_X = X; }
	inline void				SetY(unsigned char Y) { m_Y = Y; }

	// modify a flag in status register
	inline void				SetFlagB(unsigned char val) { if (val) m_SR |= CPU6502_FLAG_B; else m_SR &= ~CPU6502_FLAG_B; }
	inline void				SetFlagI(unsigned char val) { if (val) m_SR |= CPU6502_FLAG_I; else m_SR &= ~CPU6502_FLAG_I; }
	inline void				SetFlagZ(unsigned char val) { if (val == 0) m_SR |= CPU6502_FLAG_Z; else m_SR &= ~CPU6502_FLAG_Z; }
	inline void				SetFlagN(unsigned char val) { if (val & CPU6502_FLAG_N) m_SR |= CPU6502_FLAG_N; else m_SR &= ~CPU6502_FLAG_N; }
	inline void				SetFlagC(unsigned char val) { if (val) m_SR |= CPU6502_FLAG_C; else m_SR &= ~CPU6502_FLAG_C; }
	inline void				SetFlagV(unsigned char val) { if (val) m_SR |= CPU6502_FLAG_V; else m_SR &= ~CPU6502_FLAG_V; }
	inline void				SetFlagD(unsigned char val) { if (val) m_SR |= CPU6502_FLAG_D; else m_SR &= ~CPU6502_FLAG_D; }

	// read/write a word in memory
    inline unsigned short	ReadWordBug(unsigned short addr) { return MAKE_WORD(ReadByte(addr), ReadByte((addr & 0xFF00) | ((addr + 1) & 0x00FF))); }
    inline unsigned short	ReadWord(unsigned short addr) { return MAKE_WORD(ReadByte(addr), ReadByte(addr + 1)); }
	inline void				WriteWord(unsigned short addr, unsigned short val) { WriteByte(addr, LO_BYTE(val)); WriteByte(addr + 1, HI_BYTE(val)); }

	// push/pop a value on the stack.
	inline void				PushByte(unsigned char val) { WriteByte(0x100 + m_SP, val); m_SP--; }
	inline void				PushWord(unsigned short val) { PushByte(HI_BYTE(val)); PushByte(LO_BYTE(val)); }
	inline unsigned char	PopByte(void) { m_SP++; return ReadByte(0x100 + m_SP); }
	inline unsigned short	PopWord(void) { unsigned char uLow = PopByte(); return (((unsigned short)uLow) | (((unsigned short)PopByte()) << 8)); }

	// execute one instruction and returns the number of cycles.
	virtual int				Step(void);

	// build a trace of the next instruction. Buffer should be at least 128 bytes
	virtual unsigned short	BuildTrace(char *buffer);
	virtual int				BuildInstruction(char *buffer, unsigned char *data, int lenData, unsigned short address);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual char			*GetAddressLabelAllBanks(unsigned short addr);
    virtual void			Trace(int module, bool debug, const char *msg, ...) = 0;
	virtual bool			HasTrace(void) { return m_traceOn; }
	virtual void			SetTrace(bool traceOn) { m_traceOn = traceOn; m_instructionsSkipped = 0; }

	// trigger an interrupt line and returns the number of cycles.
	int						Reset(void);
	int						Nmi(void);
	int						Irq(void);

	// read/write a byte in memory
	virtual unsigned char	ReadByte(unsigned short addr) = 0;
	virtual void			WriteByte(unsigned short addr, unsigned char val) = 0;
	virtual bool			IsAddressSkipped(unsigned short addr) = 0;
};

#endif
