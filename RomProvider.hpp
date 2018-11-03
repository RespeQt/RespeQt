//
// RomProvider class
// (c) 2016 Eric BACHER
//

#ifndef ROMPROVIDER_HPP
#define ROMPROVIDER_HPP 1

#include "Rom.hpp"

// This class is used to load a ROM file and the corresponding symbol file if any.
class RomProvider {

private:

	Rom						*m_roms;

protected:

    Rom 					*GetRom(QString romName);

public:

     // constructor and destructor
							RomProvider();
	virtual					~RomProvider();

	// load binary and symbols
    virtual Rom				*RegisterRom(QString romName, QString filename);
};

#endif
