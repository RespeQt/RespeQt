//
// Chip class
// (c) 2016 Eric BACHER
//

#ifndef CHIP_HPP
#define CHIP_HPP 1

// This class defines an interface for a Chip.
class Chip {

public:

     // read/write Chip registers
     virtual unsigned char	ReadRegister(unsigned short addr) = 0;
     virtual void			WriteRegister(unsigned short addr, unsigned char val) = 0;
};

#endif
