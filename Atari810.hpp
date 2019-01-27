//
// Atari810 class
// (c) 2016 Eric BACHER
//

#ifndef ATARI810_HPP
#define ATARI810_HPP 1

#include "AtariDrive.hpp"

#define A810_FREQUENCY 500000L

#define A810_TICKS_BETWEEN_BITS 26
#define A810_TICKS_AFTER_START_BITS 12

// This class defines a common interface for all the computer models.
class Atari810 : public AtariDrive {

protected:

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual char			*GetAddressLabelAllBanks(unsigned short addr);
    virtual bool			IsAddressSkipped(unsigned short addr);

public:

	// constructors and destructor
                            Atari810(Rom *rom, eHARDWARE eHardware, int rpm);
	virtual					~Atari810();
};

#endif
