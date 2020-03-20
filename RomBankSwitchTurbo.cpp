//
// RamUnderRom class
// (c) 2018 Eric BACHER
//

#include "RomBankSwitchTurbo.hpp"

RomBankSwitchTurbo::RomBankSwitchTurbo(Rom *rom)
{
    m_bankNumber = 0x03;
    m_rom = rom;
}

RomBankSwitchTurbo::~RomBankSwitchTurbo()
{
    m_rom = nullptr;
}

unsigned char RomBankSwitchTurbo::ReadRegister(unsigned short addr)
{
    unsigned char val = m_rom->ReadRegister((addr & 0x07FF) | (m_bankNumber << 11));
    if (addr < 0x0800) {
        m_bankNumber = (val & 0x50 ? 1 : 0) | (val & 0x20 ? 0 : 2);
    }
    return val;
}

void RomBankSwitchTurbo::WriteRegister(unsigned short, unsigned char)
{
}

void RomBankSwitchTurbo::Dump(char *)
{
}
