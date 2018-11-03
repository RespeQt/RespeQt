//
// Atari1050Speedy class
// (c) 2018 Eric BACHER
//

#ifndef ATARI1050SPEEDY_HPP
#define ATARI1050SPEEDY_HPP 1

#include "AtariDrive.hpp"

#define A1050_FREQUENCY 1000000L

#define A1050_TICKS_BETWEEN_BITS 52
#define A1050_TICKS_AFTER_START_BITS 20

// This class defines a common interface for all the computer models.
class Atari1050Speedy : public AtariDrive {

protected:

    // 8192 bytes on Speedy board
    Ram                     *m_ramSpeedy;
    Ram                     *m_unmapped;
    int                     m_bankNumber;

	// find the chip targeted with a given address
	virtual Chip			*FindChip(unsigned short addr, unsigned short *chipAddr);
	virtual char			*GetAddressLabel(unsigned short addr);
    virtual void			Dump(char *folder, int sequenceNumber);

public:

	// constructors and destructor
                            Atari1050Speedy(Rom *rom, eHARDWARE eHardware, int rpm);
    virtual					~Atari1050Speedy();
};

#endif
