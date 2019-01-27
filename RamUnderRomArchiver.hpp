//
// RamUnderRom class
// (c) 2018 Eric BACHER
//

#ifndef RAMUNDERROMARCHIVER_HPP
#define RAMUNDERROMARCHIVER_HPP 1

#include "Chip.hpp"
#include "Ram.hpp"
#include "Rom.hpp"
#include "RiotDevices.hpp"

// This class emulates a RAM chip and a ROM chip at the same address range for Super Archiver.
class RamUnderRomArchiver : public Chip {

private:

    Rom						*m_rom;
    Ram						*m_ram;
    RiotDevices				*m_riot;

public:

	// constructor and destructor
                            RamUnderRomArchiver(Rom *rom, Ram *ram, RiotDevices *m_riot);
    virtual					~RamUnderRomArchiver();

    // read/write Rom/Ram addresses
	virtual unsigned char	ReadRegister(unsigned short addr);
	virtual void			WriteRegister(unsigned short addr, unsigned char val);
    virtual void			Dump(char *filename);
};

#endif
