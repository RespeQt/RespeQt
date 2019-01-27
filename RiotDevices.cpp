//
// RiotDevices class
// (c) 2016 Eric BACHER
//

#include <stdio.h>
#include <string.h>
#include "RiotDevices.hpp"
#include "Emulator.h"

RiotDevices::RiotDevices(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc) : Riot6532(cpu)
{
	m_driveNumber = driveNumber;
	m_bankNumber = 0;
	m_motor = motor;
	m_sio = sio;
	m_fdc = fdc;
}

void RiotDevices::Reset(void)
{
	Riot6532::Reset();
	m_sio->Reset();
	m_fdc->Reset();
	m_motor->Reset();
}

void RiotDevices::Step(int nbTicks)
{
    Riot6532::Step(nbTicks);
    m_motor->Step(nbTicks);
	m_sio->Step(nbTicks);
}

bool RiotDevices::IsSioWaitingForMoreData(void)
{
	if (m_sioWaitingForMoreData) {
		m_portBRead = 0;
	}
	return m_sioWaitingForMoreData;
}

void RiotDevices::TriggerIrq(int assertLine)
{
	m_fdc->SetIndexPulse(assertLine);
}

void RiotDevices::ReceiveSioCommandFrame(unsigned char *commandFrame)
{
    m_sio->ReceiveCommandFrame(commandFrame);
    m_portBRead = 0;
}

void RiotDevices::ExecuteSioCommandFrame(void)
{
    m_sio->ExecuteCommandFrame();
	m_sioWaitingForMoreData = false;
	m_portBRead = 0;
}

int RiotDevices::CopySioDataFrame(unsigned char *buf)
{
	int size = m_sio->GetDataOutCount();
	if (size > 0) {
		memcpy(buf, m_sio->GetDataOutBuffer(), size);
	}
	return size;
}

void RiotDevices::ReceiveSioDataFrame(unsigned char *buf, int len)
{
	m_sio->ReceiveDataFrame(buf, len);
}
