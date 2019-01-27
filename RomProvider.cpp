//
// RomProvider class
// (c) 2016 Eric BACHER
//

#include <string.h>
#include <stdio.h>
#include "RomProvider.hpp"

RomProvider::RomProvider()
{
	m_roms = NULL;
}

RomProvider::~RomProvider()
{
	Rom *head = m_roms;
	while (head) {
		Rom *next = head->GetNext();
		delete head;
		head = next;
	}
	m_roms = NULL;
}

Rom *RomProvider::GetRom(QString romName)
{
	Rom *head = m_roms;
	while (head != NULL) {
        if (head->GetRomName() == romName) {
			return head;
		}
		head = head->GetNext();
	}
	return NULL;
}

Rom *RomProvider::RegisterRom(QString romName, QString filename)
{
	Rom *rom = GetRom(romName);
	if (rom == NULL) {
		rom = new Rom(romName);
        if (!rom->LoadRomFile(filename)) {
			delete rom;
			rom = NULL;
		}
	}
	return rom;
}
