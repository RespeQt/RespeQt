//
// Fdc class
// (c) 2016 Eric BACHER
//

#ifndef FDC_HPP
#define FDC_HPP 1

#include "Chip.hpp"
#include "Cpu6502.hpp"
#include "Track.hpp"
#include "Crc16.hpp"

enum eFDC { FDC_1771, FDC_2793 };
enum eTYPE { FDC_TYPE1, FDC_TYPE2, FDC_TYPE3, FDC_TYPE4 };
enum eTASK { FDC_RESTORE, FDC_SEEK, FDC_STEP, FDC_STEPIN, FDC_STEPOUT, FDC_READ, FDC_WRITE, FDC_READADDR, FDC_READTRK, FDC_WRITETRK, FDC_FORCEINT };
enum eAUTOM {
	AUT_NONE, AUT_DELAY_R1R0, AUT_CHK_TRACK, AUT_RSEC_WAIT_HEAD, AUT_RSEC_FETCH_ID, AUT_RSEC_FETCH_HDR, AUT_RSEC_FETCH_DATA, AUT_RSEC_READ_DATA,
	AUT_RSEC_READ_CRC, AUT_WSEC_WAIT_HEAD, AUT_WSEC_FETCH_ID, AUT_WSEC_FETCH_HDR, AUT_WSEC_DELAY2, AUT_WSEC_DELAY8, AUT_WSEC_DELAY1, AUT_WSEC_DELAY11,
	AUT_WSEC_ZEROS, AUT_WSEC_WRITEGATE, AUT_WSEC_DAM, AUT_WSEC_DATA, AUT_WSEC_CRC, AUT_WSEC_ONE, AUT_RADDR_DELAY, AUT_RADDR_FETCH_ID, AUT_RADDR_FETCH_HDR,
	AUT_RTRK_DELAY, AUT_RTRK_WAIT, AUT_RTRK_PULSE, AUT_RTRK_READ, AUT_WTRK_DELAY, AUT_WTRK_WAIT, AUT_WTRK_PULSE, AUT_WTRK_WRITE, AUT_FINT_INDEX
};

#define FORCE_INTERRUPT_TO_READY      0x01
#define FORCE_INTERRUPT_TO_NOT_READY  0x02
#define FORCE_INTERRUPT_INDEX         0x04
#define FORCE_INTERRUPT_IMMEDIATE     0x08

// This class emulates the common behavior of FD1771, WD2793 and WD2797 chips.
class Fdc : public Chip {

private:

	Cpu6502					*m_cpu;
	Track					*m_track;
	long					m_frequency;
	bool					m_invertedBus;

    // type of FDC
    eFDC					m_fdcType;

	// FDC registers
	unsigned char			m_commandRegister;
	unsigned char			m_statusRegister;
    unsigned char			m_trackRegister;
	unsigned char			m_sectorRegister;
	unsigned char			m_dataRegister;

	// internal register
	unsigned char			m_dsr;
	unsigned char			m_direction;
	int						m_previousBitIndex;
	int						m_nextBitIndex;
	int						m_maxBitIndex;
	int						m_nextWriteBitIndex;
	int						m_nbHoles;
	bool					m_previousIndexPulse;
	unsigned char			m_header[7]; // ID Mark, track, side, sector, size, crc1, crc2
	int						m_headerIndex;
	int						m_maxRotation;
	int						m_nbBytes;
    int                     m_idBitIndex;
	Crc16					m_crc16;

	// command type and automaton
	eTASK					m_task; // kind of command
	eTYPE					m_type; // type I, II, III or IV
    eAUTOM					m_automaton;
	int						m_cycles;
	unsigned char			m_r1r0;
	unsigned char			m_a1a0;
	unsigned char			m_i3i2i1i0;
	bool					m_verify;
	bool					m_headLoad;
	bool					m_update;
	bool					m_hld;
	bool					m_multiple;
	bool					m_synchronize;
	bool					m_block;
    int						m_sideCompare;
    bool					m_sideCompareEnabled;
    bool					m_eDelay;

	// floppy interface
	bool					m_busy;
	bool					m_indexPulse;
	bool					m_ready;
    bool					m_doubleDensity;
	bool					m_crcError;
	bool					m_seekError;
	bool					m_headEngaged;
	bool					m_writeProtect;
	bool					m_drq;
	bool					m_irq;
	bool					m_lostData;
	bool					m_idNotFound;
	bool					m_recordNotFound;
	bool					m_writeFault;
	int						m_recordType;

	int						m_r1r0Cycles[4];
    const char				*m_commandName[11] = { "RESTORE", "SEEK", "STEP", "STEP-IN", "STEP-OUT", "READ SECTOR", "WRITE SECTOR", "READ ADDRESS", "READ TRACK", "WRITE TRACK", "FORCE INTERRUPT" };

protected:

	inline void				UpdateStatus(void);
	inline void				DecodeCommand(void);
	inline void				ResetFlags(void);
	inline void				StartCommand(void);
	inline void				EndCommand(void);
	virtual void			ExecuteCommand(unsigned char command);
    virtual void            DumpFDCCommand(void);

public:

	// constructor and destructor
                            Fdc(Cpu6502 *cpu, Track *track, long frequency, bool invertedBus, eFDC fdcType);
	virtual					~Fdc() { }
	virtual void			Reset(void);

	// continue the execution of the current command
	virtual void			Step(int nbTicks);

	// get Irq and Drq status
    virtual bool			HasDrq(void)           { return m_drq; }
    virtual bool			HasIrq(void)           { return m_irq; }
    virtual bool			HasIndexPulse(void)    { return m_indexPulse; }
    virtual bool			IsBusy(void)           { return m_busy; }
    virtual bool			IsDoubleDensity(void)  { return m_doubleDensity; }
    virtual eFDC			GetFdcType(void)       { return m_fdcType; }
    virtual bool			IsOnTrack00(void)      { return (m_fdcType == FDC_2793) && m_track->IsOnTrack00(); }

    // read/write Fdc1771/WD2793 registers
	virtual unsigned char	ReadRegister(unsigned short addr);
	virtual void			WriteRegister(unsigned short addr, unsigned char val);
	virtual void			SetIndexPulse(int assertLine);
    virtual void			SetReady(bool bReady) { m_ready = (m_fdcType == FDC_1771) || bReady; }
    virtual void			SetDoubleDensity(bool doubleDensity) { m_doubleDensity = (m_fdcType == FDC_2793) && doubleDensity; }
	virtual void			ForceInterrupt(void);
    virtual void            DumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int bitNumber);

    // called from Cpu6502
    virtual void            DumpCPUInstructions(char *line);
};

#endif
