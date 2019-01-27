//
// Atari1050Duplicator class
// (c) 2018 Eric BACHER
//

#include <stdio.h>
#include "Emulator.h"
#include "Atari1050Duplicator.hpp"
#include "Riot1050.hpp"

Atari1050Duplicator::Atari1050Duplicator(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_65C02, rom, eHardware, rpm, A1050_FREQUENCY, A1050_TICKS_BETWEEN_BITS, A1050_TICKS_AFTER_START_BITS, false, FDC_2793)
{
    m_riot = new Riot1050(this, 1, m_motor, m_sio, m_fdc);
    m_ramDuplicator = new Ram(this, 8192);
    m_unmapped = new Ram(this, 1);
    m_unmapped->WriteRegister(0, 0xFF);
}

Atari1050Duplicator::~Atari1050Duplicator()
{
    delete m_unmapped;
    delete m_ramDuplicator;
	delete m_riot;
}

Chip *Atari1050Duplicator::FindChip(unsigned short addr, unsigned short *chipAddr)
{
    if (addr >= 0xE000) {
        *chipAddr = addr - 0xE000;
        return m_rom;
    }
    if ((addr >= 0x2000) && (addr <= 0x3FFF)) {
        *chipAddr = addr - 0x2000;
        return m_ramDuplicator;
	}
    if (addr >= 0x1000) {
        *chipAddr = 0;
        m_unmapped->WriteRegister(0, 0xFF);
        return m_unmapped;
    }
    addr &= 0x07FF;
    if (addr & 0x0400) {
        *chipAddr = addr & 0x0003;
        return m_fdc;
    }
    if (addr & 0x0080) {
        if (addr & 0x0200) {
            *chipAddr = addr & 0x001F;
            return m_riot;
        }
        *chipAddr = addr & 0x007F;
        return m_ram6532;
    }
    if (addr & 0x0200) {
        *chipAddr = 0;
        m_unmapped->WriteRegister(0, 0xFF);
        return m_unmapped;
    }
    *chipAddr = addr & 0x007F;
    return m_ram6810;
}

char *Atari1050Duplicator::GetAddressLabel(unsigned short addr)
{
	return m_rom->GetAddressLabel(addr);
}

void Atari1050Duplicator::Dump(char *folder, int sequenceNumber)
{
    AtariDrive::Dump(folder, sequenceNumber);
    static char buf[512];
    sprintf(buf, "%sramDuplicator_%d.bin", folder, sequenceNumber);
    m_ramDuplicator->Dump(buf);
}
