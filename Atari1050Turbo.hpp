//
// Atari1050Turbo class
// (c) 2018 Eric BACHER
//

#ifndef ATARI1050Turbo_HPP
#define ATARI1050Turbo_HPP 1

#include "AtariDrive.hpp"
#include "RomBankSwitchTurbo.hpp"

#define A1050_FREQUENCY 1000000L

#define A1050_TICKS_BETWEEN_BITS 52
#define A1050_TICKS_AFTER_START_BITS 20

// This class defines a common interface for all the computer models.
class Atari1050Turbo : public AtariDrive {

protected:

    // 8192 bytes on Turbo board
    Ram                     *m_ramTurbo;
    Ram                     *m_unmapped;
    RomBankSwitchTurbo      *m_romBankSwitch;

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual void			Dump(char *folder, int sequenceNumber);

public:

	// constructors and destructor
                            Atari1050Turbo(Rom *rom, eHARDWARE eHardware, int rpm);
    virtual					~Atari1050Turbo();
};

#endif
