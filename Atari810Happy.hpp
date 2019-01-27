//
// Atari810Happy class
// (c) 2016 Eric BACHER
//

#ifndef ATARI810HAPPY_HPP
#define ATARI810HAPPY_HPP 1

#include "AtariDrive.hpp"

#define A810_FREQUENCY 500000L

#define A810_TICKS_BETWEEN_BITS 26
#define A810_TICKS_AFTER_START_BITS 12

// This class defines a common interface for all the computer models.
class Atari810Happy : public AtariDrive {

protected:

    // 4096 bytes on Happy board
    Ram                     *m_ramHappy;
    bool                    m_fullRomDump;
    bool                    m_bankSwitching;
    int                     m_bankNumber;

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual char			*GetAddressLabelAllBanks(unsigned short addr);
    virtual void			Dump(char *folder, int sequenceNumber);

public:

	// constructors and destructor
                            Atari810Happy(Rom *rom, eHARDWARE eHardware, int rpm);
    virtual					~Atari810Happy();
};

#endif
