//
// AtariDrive class
// (c) 2016 Eric BACHER
//

#ifndef ATARIDRIVE_HPP
#define ATARIDRIVE_HPP 1

#include "Cpu6502.hpp"
#include "RiotDevices.hpp"
#include "Rom.hpp"
#include "Ram.hpp"

// This class defines a common interface for all the computer models.
class AtariDrive : public Cpu6502 {

protected:

	// cpu speed and number of cycles
	eHARDWARE				m_hardware;
	long					m_frequency;
	long					m_cycles;

	// other processors.
    Track					*m_track;
	Motor					*m_motor;
	Sio						*m_sio;
	Fdc						*m_fdc;
	RiotDevices				*m_riot;
	Rom						*m_rom;
	Ram						*m_ram6810;
	Ram						*m_ram6532;
    FirmwareDiskImage		*m_diskImager;

	// drive state
	bool					m_on;

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr) = 0;

public:

	// constructors and destructor
                            AtariDrive(CPU_ENUM cpuType, Rom *rom, eHARDWARE eHardware, int rpm, long frequency, int ticksBetweenBits, int ticksAfterStartBit, bool invertedBus, eFDC fdcType);
	virtual					~AtariDrive();
	virtual void			PowerOn(void);
	virtual void			TurnOn(bool bOn);
    virtual void			SetReady(bool bReady);
    virtual void            SetDiskImager(FirmwareDiskImage *diskImager) { m_diskImager = diskImager; m_motor->GetTrack()->SetDiskImager(diskImager); }
    virtual void            SetSpeed(bool normalSpeed);

	// getters
    virtual Fdc				*GetFdc(void) { return m_fdc; }
    virtual RiotDevices		*GetRiot(void) { return m_riot; }
    virtual Rom				*GetRom(void) { return m_rom; }
    virtual Ram				*GetRam6810(void) { return m_ram6810; }
    virtual Ram				*GetRam6532(void) { return m_ram6532; }
    virtual Track			*GetTrack(void) { return m_track; }
    virtual long			GetCycles(void) { return m_cycles; }
    virtual bool			IsOn(void) { return m_on; }
    virtual eHARDWARE		GetHardware(void) { return m_hardware; }
    virtual int             GetBankNumber(void) { return m_riot->GetBankNumber(); }

	// execute one instruction and returns the number of cycles.
	virtual int				Step(void);
    virtual void			RunForCycles(long nbMicroSeconds);
    virtual void			RunUntilPosition(long nbMicroSeconds);
    virtual unsigned char	ReceiveSioCommandFrame(unsigned char *commandFrame);
    virtual unsigned char	ExecuteSioCommandFrame(void);
	virtual int				CopySioDataFrame(unsigned char *buf);
	virtual unsigned char	ReceiveSioDataFrame(unsigned char *buf, int len);
    virtual int             GetTransmitSpeedInBitsPerSeconds(void);
    virtual unsigned long   GetExecutionTimeInMicroseconds(void);
    virtual unsigned long   GetDataTransmitTimeInMicroseconds(void);
    virtual void			FlushTrack(void);

	// read/write a byte in memory
	virtual unsigned char	ReadByte(unsigned short addr);
	virtual void			WriteByte(unsigned short addr, unsigned char val);

    // write a clock mark on track (coming from FDC)
    virtual void			WriteClock(int bitNumber, bool bit);

	// enable/disable traces
    virtual void			Trace(int module, bool debug, const char *msg, ...);
    virtual void			Dump(char *folder, int sequenceNumber);
    virtual bool			IsAddressSkipped(unsigned short addr);
};

#endif
