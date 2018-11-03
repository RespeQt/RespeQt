//
// Atari1050 class
// (c) 2016 Eric BACHER
//

#ifndef ATARI1050_HPP
#define ATARI1050_HPP 1

#include "AtariDrive.hpp"
#include "RamUnderRomArchiver.hpp"

#define A1050_FREQUENCY 1000000L

#define A1050_TICKS_BETWEEN_BITS 52
#define A1050_TICKS_AFTER_START_BITS 20

// This class defines a common interface for all the computer models.
class Atari1050 : public AtariDrive {

private:

    // 2 KB embedded with the Archiver
    Ram						*m_ramArchiver;
    // But the RAM may be written through the ROM
    RamUnderRomArchiver		*m_ramUnderRomArchiver;
    // Some parts are not mapped
    Ram                     *m_unmapped;

protected:

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual char			*GetAddressLabelAllBanks(unsigned short addr);
    virtual bool			IsAddressSkipped(unsigned short addr);

    // write a clock mark on track (coming from FDC)
    virtual void			WriteClock(int bitNumber, bool bit);

public:

	// constructors and destructor
                            Atari1050(Rom *rom, eHARDWARE eHardware, int rpm);
	virtual					~Atari1050();
	virtual int				Step(void);
    virtual void            SetSpeed(bool normalSpeed);
};

#endif
