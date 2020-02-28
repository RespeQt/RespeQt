//
// RamUnderRom class
// (c) 2018 Eric BACHER
//

#include "RamUnderRomArchiver.hpp"

#include <QtDebug>

RamUnderRomArchiver::RamUnderRomArchiver(Rom *rom, Ram *ram, RiotDevices *riot)
{
    m_rom = rom;
    m_ram = ram;
    m_riot = riot;
}

RamUnderRomArchiver::~RamUnderRomArchiver()
{
    m_rom = nullptr;
    m_ram = nullptr;
    m_riot = nullptr;
}

unsigned char RamUnderRomArchiver::ReadRegister(unsigned short addr)
{
    if (addr & 0x0800) {
        return m_rom->ReadRegister((addr & 0x07FF) | (m_riot->GetBankNumber() << 11));
    }
    return m_ram->ReadRegister(addr & 0x07FF);
}

void RamUnderRomArchiver::WriteRegister(unsigned short addr, unsigned char val)
{
    m_ram->WriteRegister(addr & 0x07FF, val);
}

void RamUnderRomArchiver::Dump(char *filename)
{
    m_ram->Dump(filename);
}
