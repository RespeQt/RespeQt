//
// Atari1050 class
// (c) 2016 Eric BACHER
//

#include <cstdio>
#include "Emulator.h"
#include "Atari1050.hpp"
#include "Riot1050.hpp"

Atari1050::Atari1050(Rom *rom, eHARDWARE eHardware, int rpm)
    : AtariDrive(CPU_6502, rom, eHardware, rpm, A1050_FREQUENCY, A1050_TICKS_BETWEEN_BITS, A1050_TICKS_AFTER_START_BITS, false, FDC_2793)
{
    m_riot = new Riot1050(this, 1, m_motor, m_sio, m_fdc);
    if (m_hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
        m_ramArchiver = new Ram(this, 2048);
        m_ramUnderRomArchiver = new RamUnderRomArchiver(m_rom, m_ramArchiver, m_riot);
    }
    m_unmapped = new Ram(this, 1);
    m_unmapped->WriteRegister(0, 0xFF);
}

Atari1050::~Atari1050()
{
    delete m_unmapped;
    delete m_riot;
    if (m_hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
        delete m_ramUnderRomArchiver;
        delete m_ramArchiver;
    }
}

int Atari1050::Step(void)
{
	return AtariDrive::Step();
}

Chip *Atari1050::FindChip(unsigned short addr, unsigned short *chipAddr)
{
	addr &= 0x1FFF;
	if (addr & 0x1000) {
        *chipAddr = addr & 0x0FFF;
        if (m_hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
            return m_ramUnderRomArchiver;
		}
		else {
			return m_rom;
		}
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

void Atari1050::WriteClock(int bitNumber, bool bit)
{
    if ((m_hardware == HARDWARE_1050_WITH_THE_ARCHIVER) && (m_riot->GetBankNumber() == 0)) {
        bit = false;
    }
    m_track->WriteClockOnTrack(bitNumber, bit);
}

void Atari1050::SetSpeed(bool normalSpeed)
{
    m_track->SetRPM(normalSpeed ? DISK_NORMAL_RPM : DISK_SLOW_RPM);
}

char *Atari1050::GetAddressLabel(unsigned short addr)
{
	if (addr & 0x1000) {
        if (m_hardware != HARDWARE_1050_WITH_THE_ARCHIVER) {
			addr = (addr & 0x0FFF) | 0xF000;
		}
	}
	return m_rom->GetAddressLabel(addr);
}

char *Atari1050::GetAddressLabelAllBanks(unsigned short addr)
{
    if (addr & 0x1000) {
        unsigned short fixedAddr = addr;
        if (m_hardware != HARDWARE_1050_WITH_THE_ARCHIVER) {
            fixedAddr = (addr & 0x0FFF) | 0xF000;
        }
        char *label = m_rom->GetAddressLabel(fixedAddr);
        if (label) {
            return label;
        }
    }
    return m_rom->GetAddressLabel(addr);
}

bool Atari1050::IsAddressSkipped(unsigned short addr)
{
    if (addr & 0x1000) {
        if (m_hardware != HARDWARE_1050_WITH_THE_ARCHIVER) {
            addr = (addr & 0x0FFF) | 0xF000;
        }
    }
    return m_rom->IsAddressSkipped(addr);
}
