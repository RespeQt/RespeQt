//
// Ram class
// (c) 2016 Eric BACHER
//

#include <memory.h>
#ifndef __clang__
#include <malloc.h>
#endif
#include <cstdio>
#include "Ram.hpp"

#include <QtDebug>

Ram::Ram(Cpu6502 *cpu, int iSize)
{
	m_cpu = cpu;
    m_ram = (unsigned char *)malloc(iSize);
	m_size = iSize;
    memset(m_ram, 0, iSize);
}

Ram::~Ram()
{
    free(m_ram);
    m_ram = nullptr;
}

unsigned char Ram::ReadRegister(unsigned short addr)
{
     return m_ram[addr & (m_size - 1)];
}

void Ram::WriteRegister(unsigned short addr, unsigned char val)
{
     m_ram[addr & (m_size - 1)] = val;
}

void Ram::Dump(char *filename)
{
	FILE *fpDump = fopen(filename, "wb");
	fwrite(m_ram, m_size, 1, fpDump);
	fclose(fpDump);
}
