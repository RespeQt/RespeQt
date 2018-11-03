//
// Atari1050Turbo class
// (c) 2018 Eric BACHER
//

#include <stdio.h>
#include "Emulator.h"
#include "Atari1050Turbo.hpp"
#include "Riot1050.hpp"

Atari1050Turbo::Atari1050Turbo(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_6502, rom, eHardware, rpm, A1050_FREQUENCY, A1050_TICKS_BETWEEN_BITS, A1050_TICKS_AFTER_START_BITS, false, FDC_2793)
{
    m_riot = new Riot1050(this, 1, m_motor, m_sio, m_fdc);
    m_romBankSwitch = new RomBankSwitchTurbo(m_rom);
    m_unmapped = new Ram(this, 1);
    m_unmapped->WriteRegister(0, 0xFF);
}

Atari1050Turbo::~Atari1050Turbo()
{
    delete m_romBankSwitch;
    delete m_unmapped;
	delete m_riot;
}

Chip *Atari1050Turbo::FindChip(unsigned short addr, unsigned short *chipAddr)
{
    addr &= 0x1FFF;
    if (addr & 0x1000) {
        *chipAddr = addr & 0x0FFF;
        return m_romBankSwitch;
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

char *Atari1050Turbo::GetAddressLabel(unsigned short addr)
{
	return m_rom->GetAddressLabel(addr);
}

void Atari1050Turbo::Dump(char *folder, int sequenceNumber)
{
    AtariDrive::Dump(folder, sequenceNumber);
    static char buf[512];
    sprintf(buf, "%sramTurbo_%d.bin", folder, sequenceNumber);
    m_ramTurbo->Dump(buf);
}
