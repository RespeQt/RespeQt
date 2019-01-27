//
// Ram class
// (c) 2016 Eric BACHER
//

#ifndef RAM_HPP
#define RAM_HPP 1

#include "Chip.hpp"
#include "Cpu6502.hpp"

// This class emulates a RAM chip.
class Ram : public Chip {

private:

	Cpu6502					*m_cpu;
	unsigned char			*m_ram;
	int						m_size;

public:

	// constructor and destructor
							Ram(Cpu6502 *cpu, int iSize = 128);
	virtual					~Ram();

	// read/write Ram addresses
	virtual unsigned char	ReadRegister(unsigned short addr);
	virtual void			WriteRegister(unsigned short addr, unsigned char val);
	virtual void			Dump(char *filename);
};

#endif
