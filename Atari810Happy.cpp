//
// Atari810Happy class
// (c) 2016 Eric BACHER
//

#include <cstdio>
#include "Emulator.h"
#include "Atari810Happy.hpp"
#include "Riot810.hpp"

Atari810Happy::Atari810Happy(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_6502, rom, eHardware, rpm, A810_FREQUENCY, A810_TICKS_BETWEEN_BITS, A810_TICKS_AFTER_START_BITS, true, FDC_1771)
{
    m_riot = new Riot810(this, 1, m_motor, m_sio, m_fdc);
    m_ramHappy = new Ram(this, 4096);
    // ROM size for Happy 810 without bank switch is max 4096
    // ROM size for Happy 810 with bank switch is min 6144
    int romSize = rom->GetLength();
    if (romSize > 5000) {
        m_bankSwitching = true;
        romSize /= 2;
    }
    // some ROM dumps contains the first part (1Kb) with the copyright which is not visible in the address space
    m_fullRomDump = (romSize < 4000) ? false : true;
}

Atari810Happy::~Atari810Happy()
{
    delete m_ramHappy;
	delete m_riot;
}

Chip *Atari810Happy::FindChip(unsigned short addr, unsigned short *chipAddr)
{
    addr &= 0x1FFF;
    if (addr >= 0x1400) {
        unsigned short fixedAddr = addr & 0x0FFF;
        if (m_bankSwitching) {
            if (addr == 0x1FF8) {
                m_bankNumber = 0;
            }
            else if (addr == 0x1FF9) {
                m_bankNumber = 1;
            }
            if (m_bankNumber == 1) {
                fixedAddr |= 0x1000;
                if (! m_fullRomDump) {
                    fixedAddr -= 0x400;
                }
            }
        }
        if (! m_fullRomDump) {
            fixedAddr -= 0x400;
        }
        *chipAddr = fixedAddr;
        return m_rom;
    }
    if (addr >= 0x400) {
        *chipAddr = (addr & 0x0FFF) - 0x400;
        return m_ramHappy;
	}
	if (addr & 0x0100) {
		if (addr & 0x0200) {
			*chipAddr = addr & 0x001F;
			return m_riot;
		}
		*chipAddr = addr & 0x007F;
		return m_ram6532;
	}
	if (addr & 0x0080) {
		*chipAddr = addr & 0x007F;
		return m_ram6810;
	}
	*chipAddr = addr & 0x0003;
	return m_fdc;
}

char *Atari810Happy::GetAddressLabel(unsigned short addr)
{
	return m_rom->GetAddressLabel(addr);
}

char *Atari810Happy::GetAddressLabelAllBanks(unsigned short addr)
{
    if (m_bankSwitching) {
        char *label = m_rom->GetAddressLabel(addr | 0x1000);
        if (label) {
            return label;
        }
    }
    return m_rom->GetAddressLabel(addr);
}

void Atari810Happy::Dump(char *folder, int sequenceNumber)
{
    AtariDrive::Dump(folder, sequenceNumber);
    static char buf[512];
    sprintf(buf, "%sramHappy_%d.bin", folder, sequenceNumber);
    m_ramHappy->Dump(buf);
}
