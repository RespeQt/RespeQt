//
// Atari810 class
// (c) 2016 Eric BACHER
//

#include <cstdio>
#include "Emulator.h"
#include "Atari810.hpp"
#include "Riot810.hpp"

Atari810::Atari810(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_6502, rom, eHardware, rpm, A810_FREQUENCY, A810_TICKS_BETWEEN_BITS, A810_TICKS_AFTER_START_BITS, true, FDC_1771)
{
    m_riot = new Riot810(this, 1, m_motor, m_sio, m_fdc);
}

Atari810::~Atari810()
{
	delete m_riot;
}

Chip *Atari810::FindChip(unsigned short addr, unsigned short *chipAddr)
{
	addr &= 0x0FFF;
	if (addr & 0x0800) {
		if (m_hardware == HARDWARE_810_WITH_THE_CHIP) {
			*chipAddr = (addr & 0x07FF) | (m_riot->GetBankNumber() << 11);
		}
		else {
			*chipAddr = addr & 0x07FF;
		}
		return m_rom;
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

char *Atari810::GetAddressLabel(unsigned short addr)
{
	if (addr & 0x0800) {
		if (m_hardware == HARDWARE_810_WITH_THE_CHIP) {
			addr = (addr & 0x0FFF) | (m_riot->GetBankNumber() << 12);
		}
		else {
			addr = addr & 0x0FFF;
		}
	}
	return m_rom->GetAddressLabel(addr);
}

char *Atari810::GetAddressLabelAllBanks(unsigned short addr)
{
    if (m_hardware == HARDWARE_810_WITH_THE_CHIP) {
        char *label = m_rom->GetAddressLabel((addr & 0xFFF) | 0x1000);
        if (label) {
            return label;
        }
    }
    return m_rom->GetAddressLabel(addr & 0xFFF);
}

bool Atari810::IsAddressSkipped(unsigned short addr)
{
    if (addr & 0x0800) {
        if (m_hardware == HARDWARE_810_WITH_THE_CHIP) {
            addr = (addr & 0x0FFF) | (m_riot->GetBankNumber() << 12);
        }
        else {
            addr = addr & 0x0FFF;
        }
    }
    return m_rom->IsAddressSkipped(addr);
}
