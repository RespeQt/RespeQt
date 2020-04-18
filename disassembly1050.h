//
// disassembly1050 class
// (c) 2018 Eric BACHER
//

#ifndef DISASSEMBLY1050_H
#define DISASSEMBLY1050_H

#include <cpu6502.h>

class disassembly1050 : public Cpu6502
{
public:
                            disassembly1050();
    virtual					~disassembly1050() { }

    // read/write a byte in memory (unused)
    virtual unsigned char	ReadByte(unsigned short) { return 0; }
    virtual void			WriteByte(unsigned short, unsigned char) { }

    // enable/disable traces (unused)
    virtual void			Trace(int, bool, const char *, ...) { }
    virtual void			Dump(char *, int) { }
    virtual bool			IsAddressSkipped(unsigned short) { return false; }

    virtual char			*GetAddressLabel(unsigned short addr);
};

#endif // DISASSEMBLY1050_H
