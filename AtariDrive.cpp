//
// AtariDrive class
// (c) 2016 Eric BACHER
//

#include <stdio.h>
#include <stdarg.h>
#include "Emulator.h"
#include "AtariDrive.hpp"

AtariDrive::AtariDrive(CPU_ENUM cpuType, Rom *rom, eHARDWARE eHardware, int rpm, long frequency, int ticksBetweenBits, int ticksAfterStartBit, bool invertedBus, eFDC fdcType)
    : Cpu6502(cpuType)
{
	m_hardware = eHardware;
	m_frequency = frequency;
	m_cycles = 0L;
	m_rom = rom;
	m_ram6810 = new Ram(this);
	m_ram6532 = new Ram(this);
    eTRACK trackType = (fdcType == FDC_1771) ? TRACK_FULL : TRACK_HALF;
    m_track = new Track(this, rpm, frequency, trackType);
    eMOTOR motorType = (fdcType == FDC_1771) ? MOTOR_TWO_PHASES : MOTOR_ONE_PHASE;
    m_motor = new Motor(this, m_track, frequency, motorType);
	m_sio = new Sio(this, frequency, ticksBetweenBits, ticksAfterStartBit);
    m_fdc = new Fdc(this, m_track, frequency, invertedBus, fdcType);
	m_on = false;
}

AtariDrive::~AtariDrive()
{
    delete m_track;
	delete m_motor;
	delete m_sio;
	delete m_fdc;
	delete m_ram6810;
	delete m_ram6532;
}

void AtariDrive::PowerOn()
{
	TurnOn(false);
	TurnOn(true);
}

void AtariDrive::TurnOn(bool bOn)
{
    if (bOn && (!m_on)) {
		// reset CPU and hardware
		m_riot->Reset();
		int nbTicks = Reset();
		m_cycles = (long)nbTicks;
		m_riot->Step(nbTicks);
		m_fdc->Step(nbTicks);
        // waiting 8000000 cycles is enough for all firmwares to initialize, seek a specific track, read a sector
		// and wait a delay before turning the motor off
        while (m_cycles < 8000000) {
			Step();
		}
	}
    m_on = bOn;
}

void AtariDrive::SetReady(bool bReady)
{
    m_fdc->SetReady(bReady);
    m_motor->SetReady(bReady);
}

int AtariDrive::Step(void)
{
	int nbTicks = Cpu6502::Step();
	m_cycles += (long)nbTicks;
	m_riot->Step(nbTicks);
	m_fdc->Step(nbTicks);
	return nbTicks;
}

void AtariDrive::RunForCycles(long nbMicroSeconds)
{
    // compute number of cycles
    long nbCycles = (long)((long)((m_frequency / 1000) * nbMicroSeconds) / 1000L);
    while (nbCycles > 0) {
        nbCycles -= Step();
    };
}

void AtariDrive::RunUntilPosition(long nbMicroSeconds)
{
    int nbDataBits = nbMicroSeconds / 8;
    int targetDataBits = nbDataBits * m_track->GetReadDataBitsPerRotation() / m_track->GetNormalDataBitsPerRotation();
    if (targetDataBits >= m_track->GetReadDataBitsPerRotation()) {
        targetDataBits -= m_track->GetReadDataBitsPerRotation();
    }
    int currentDataBits = m_track->GetRotationDataBit();
    int divisor = (int)(m_frequency / 125000L);
    long nbCycles = 0;
    if (currentDataBits < targetDataBits) {
        nbCycles = (targetDataBits - currentDataBits) * divisor;
    }
    else {
        nbCycles = (m_track->GetReadDataBitsPerRotation() + targetDataBits - currentDataBits) * divisor;
    }
    while (nbCycles > 0) {
        nbCycles -= Step();
    };
}

unsigned char AtariDrive::ReceiveSioCommandFrame(unsigned char *commandFrame)
{
    // check if SIO automaton is idle (is computer ready to send another command ?)
	if (!m_riot->IsSioIdle()) {
        Trace(SIO_MODULE, true, "***** ERROR: a command is sent to the firmware while another command was in progress");
		PowerOn();
	}
    m_riot->ReceiveSioCommandFrame(commandFrame);
	unsigned char answer = NO_STATUS;
	while (answer == NO_STATUS) {
		if (m_riot->IsSioIdle()) {
			break;
		}
		Step();
		answer = m_riot->GetSioCommandAnswer();
	}
    // command has been received. Return the last status byte sent by firmware to the computer.
    return answer;
}

unsigned char AtariDrive::ExecuteSioCommandFrame()
{
    // check if SIO automaton is idle (is computer ready to send another command ?)
    if (!m_riot->IsSioIdle()) {
        Trace(SIO_MODULE, true, "***** ERROR: a command is sent to the firmware while another command was in progress");
        return NO_STATUS;
    }
    m_riot->ExecuteSioCommandFrame();
    unsigned char answer = NO_STATUS;
    while (answer == NO_STATUS) {
        if (m_riot->IsSioWaitingForMoreData()) {
            // need more data from computer to execute the command
            return WAIT_FOR_DATA;
        }
        if (m_riot->IsSioIdle()) {
            break;
        }
        Step();
        answer = m_riot->GetSioCommandAnswer();
    }
    return answer;
}

int AtariDrive::CopySioDataFrame(unsigned char *buf)
{
	return m_riot->CopySioDataFrame(buf);
}

unsigned char AtariDrive::ReceiveSioDataFrame(unsigned char *buf, int len)
{
    m_riot->ReceiveSioDataFrame(buf, len);
	unsigned char answer = 0;
	while (answer == 0) {
		if (m_riot->IsSioIdle()) {
            break;
		}
		Step();
		answer = m_riot->GetSioCommandAnswer();
	}
    return answer;
}

void AtariDrive::FlushTrack(void)
{
    m_track->FlushTrack();
}

unsigned char AtariDrive::ReadByte(unsigned short addr)
{
	unsigned short chipAddr = addr;
	Chip *chip = FindChip(addr, &chipAddr);
	if (chip) {
		return chip->ReadRegister(chipAddr);
	}
	return 0;
}

void AtariDrive::WriteByte(unsigned short addr, unsigned char val)
{
	unsigned short chipAddr = addr;
	Chip *chip = FindChip(addr, &chipAddr);
	if (chip) {
		chip->WriteRegister(chipAddr, val);
	}
}

void AtariDrive::WriteClock(int bitNumber, bool bit)
{
    m_track->WriteClockOnTrack(bitNumber, bit);
}

void AtariDrive::SetSpeed(bool)
{
}

int AtariDrive::GetTransmitSpeedInBitsPerSeconds(void)
{
    int ticks = m_sio->GetTicksBetweenPortB();
    return m_frequency / ticks;
}

unsigned long AtariDrive::GetExecutionTimeInMicroseconds(void)
{
    long ticks = m_sio->GetTicksForExecution();
    return (unsigned long)((1000000L / m_frequency) * ticks);
}

unsigned long AtariDrive::GetDataTransmitTimeInMicroseconds(void)
{
    long ticks = m_sio->GetTicksForSendingData();
    return (unsigned long)((1000000L / m_frequency) * ticks);
}

bool AtariDrive::IsAddressSkipped(unsigned short addr)
{
	return m_rom->IsAddressSkipped(addr);
}

void AtariDrive::Trace(int module, bool debug, const char *msg, ...)
{
    if ((module == CPU_MODULE) && (HasTrace())) {
        static char buf[512];
        va_list args;
        va_start(args, msg);
        vsprintf(buf, msg, args);
        m_fdc->DumpCPUInstructions(buf); // Ugly way back to FirmwareDiskImage via several objects. Need to change that !
    }
    if ((module == FDC_MODULE) && (! debug)) {
        static char buf[512];
        va_list args;
        va_start(args, msg);
        vsprintf(buf, msg, args);
        qDebug() << "!u" << QString::fromLocal8Bit(buf);
    }
    if ((module == SIO_MODULE) && (! debug)) {
        static char buf[512];
        va_list args;
        va_start(args, msg);
        vsprintf(buf, msg, args);
        qDebug() << "!u" << QString::fromLocal8Bit(buf);
    }
    if ((module == TRK_MODULE) && (! debug)) {
        static char buf[512];
        va_list args;
        va_start(args, msg);
        vsprintf(buf, msg, args);
        qDebug() << "!u" << QString::fromLocal8Bit(buf);
    }
}

void AtariDrive::Dump(char *folder, int sequenceNumber)
{
    static char buf[512];
    sprintf(buf, "%sram6810_%d.bin", folder, sequenceNumber);
    m_ram6810->Dump(buf);
    sprintf(buf, "%sram6532_%d.bin", folder, sequenceNumber);
    m_ram6532->Dump(buf);
}
