//
// Atari1050Happy class
// (c) 2018 Eric BACHER
//

#include <stdio.h>
#include "Emulator.h"
#include "Atari1050Happy.hpp"
#include "Riot1050.hpp"

Atari1050Happy::Atari1050Happy(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_65C02, rom, eHardware, rpm, A1050_FREQUENCY, A1050_TICKS_BETWEEN_BITS, A1050_TICKS_AFTER_START_BITS, false, FDC_2793)
{
    m_riot = new Riot1050(this, 1, m_motor, m_sio, m_fdc);
    m_ramHappy = new Ram(this, 6144);
    m_unmapped = new Ram(this, 1);
    m_unmapped->WriteRegister(0, 0xFF);
    m_bankNumber = 1;
}

Atari1050Happy::~Atari1050Happy()
{
    delete m_unmapped;
    delete m_ramHappy;
	delete m_riot;
}

Chip *Atari1050Happy::FindChip(unsigned short addr, unsigned short *chipAddr)
{
    int bankNumber = m_bankNumber;
    if ((addr & 0x2000) && (addr & 0x0FFE) == 0x0FF8) {
        m_bankNumber = addr & 1;
    }
    if ((addr & 0x3000) == 0x3000) {
        addr &= 0xFFF;
        if (bankNumber == 1) {
            addr |= 0x1000;
        }
        *chipAddr = addr;
        return m_rom;
    }
    if ((addr >= 0x8000) && (addr <= 0x97FF)) {
        *chipAddr = addr - 0x8000;
        return m_ramHappy;
    }
    if ((addr >= 0xA000) && (addr <= 0xB7FF)) {
        *chipAddr = addr - 0xA000;
        return m_ramHappy;
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

char *Atari1050Happy::GetAddressLabel(unsigned short addr)
{
	return m_rom->GetAddressLabel(addr);
}

char *Atari1050Happy::GetAddressLabelAllBanks(unsigned short addr)
{
    if ((addr & 0x3000) == 0x3000) {
        unsigned short fixedAddr = addr | 0x1000;
        char *label = m_rom->GetAddressLabel(fixedAddr);
        if (label) {
            return label;
        }
    }
    return m_rom->GetAddressLabel(addr);
}

void Atari1050Happy::Dump(char *folder, int sequenceNumber)
{
    AtariDrive::Dump(folder, sequenceNumber);
    static char buf[512];
    sprintf(buf, "%sramHappy_%d.bin", folder, sequenceNumber);
    m_ramHappy->Dump(buf);
}
