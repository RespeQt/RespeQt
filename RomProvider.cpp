//
// RomProvider class
// (c) 2016 Eric BACHER
//

#include <cstring>
#include <cstdio>
#include "RomProvider.hpp"

RomProvider::RomProvider()
{
    m_roms = nullptr;
}

RomProvider::~RomProvider()
{
	Rom *head = m_roms;
	while (head) {
		Rom *next = head->GetNext();
		delete head;
		head = next;
	}
    m_roms = nullptr;
}

Rom *RomProvider::GetRom(QString romName)
{
	Rom *head = m_roms;
    while (head != nullptr) {
        if (head->GetRomName() == romName) {
			return head;
		}
		head = head->GetNext();
	}
    return nullptr;
}

Rom *RomProvider::RegisterRom(QString romName, QString filename)
{
	Rom *rom = GetRom(romName);
    if (rom == nullptr) {
		rom = new Rom(romName);
        if (!rom->LoadRomFile(filename)) {
			delete rom;
            rom = nullptr;
		}
	}
	return rom;
}
