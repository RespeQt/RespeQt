//
// RomBankSwitchTurbo class
// (c) 2018 Eric BACHER
//

#ifndef ROMBANKSWITCHTURBO_HPP
#define ROMBANKSWITCHTURBO_HPP 1

#include "Chip.hpp"
#include "Rom.hpp"
#include "RiotDevices.hpp"

// This class emulates a ROM chip with a GAL to handle the bank switching of 2Kb blocks for Turbo 1050.
class RomBankSwitchTurbo : public Chip {

private:

    int						m_bankNumber;
    Rom						*m_rom;

public:

	// constructor and destructor
                            RomBankSwitchTurbo(Rom *rom);
    virtual					~RomBankSwitchTurbo();

    // read/write Rom/Ram addresses
	virtual unsigned char	ReadRegister(unsigned short addr);
	virtual void			WriteRegister(unsigned short addr, unsigned char val);
    virtual void			Dump(char *filename);
};

#endif
