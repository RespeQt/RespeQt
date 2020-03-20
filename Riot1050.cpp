//
// RiotDevices class
// (c) 2016 Eric BACHER
//

#include <cstdio>
#include <cstring>
#include "Riot1050.hpp"
#include "Emulator.h"

Riot1050::Riot1050(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc)
	: RiotDevices(cpu, driveNumber, motor, sio, fdc)
{
}

unsigned char Riot1050::ReadPort(int portNum)
{
	if (portNum == 0) {
		static unsigned char switchMap[4] = { SWITCH1 | SWITCH2, SWITCH2, 0, SWITCH1 };
		unsigned char switches = switchMap[(m_driveNumber - 1) & (sizeof(switchMap) - 1)];
		unsigned char motor = m_motor->GetMotorOn() ? 0 : DRVMOTOR;
		unsigned char bank = GetBankNumber() ? BANKSW : 0;
        unsigned char sdensity = m_fdc->IsDoubleDensity() ? 0 : SDENSITY;
        unsigned char irq = m_fdc->HasIndexPulse() ? 0 : IRQ2793;
		unsigned char drq = m_fdc->HasDrq() ? DRQ2793 : 0;
        return switches | motor | bank | sdensity | irq | drq;
	}
	else {
		m_sioWaitingForMoreData = false;
		if (m_sio->IsWaitingForComplete()) {
			if (++m_portBRead >= NUMBER_OF_PORTB_POLLING_THRESHOLD) {
				m_sioWaitingForMoreData = true;
			}
		}
		unsigned char dataOut = m_sio->GetDataLineOut() ? DATAOUT : 0;
		unsigned char vcc = m_sio->GetVccLine() ? 0 : VCCRDY;
		unsigned char phases = m_motor->GetPhase() << 2;
		unsigned char command = m_sio->GetCommandLine() ? COMMAND : 0;
		unsigned char dataIn = m_sio->GetDataLineIn() ? DATAIN : 0;
		return dataIn | command | phases | vcc | dataOut;
	}
}

void Riot1050::WritePort(int portNum, unsigned char val, unsigned char dir)
{
	if (portNum == 0) {
		if (dir & DRVMOTOR) {
			m_motor->SetMotorOn((val & DRVMOTOR) ? false : true);
		}
        if (dir & SDENSITY) {
            m_fdc->SetDoubleDensity(val & SDENSITY ? false : true);
        }
        if (dir & IRQ2793) {
            TriggerIrq(val & IRQ2793 ? false : true);
        }
        // for compatibility with ARCHIVER 1050 bank switching
        if (dir & BANKSW) {
			SetBankNumber((val & BANKSW) ? 1 : 0);
		}
	}
	else {
		if (dir & DATAOUT) {
			m_sio->SetDataLineOut((val & DATAOUT) ? true : false);
		}
		if (dir & (MOTOR_ALL_PHASES << 2)) {
			m_motor->Rotate((unsigned char)((val >> 2) & 0x0F));
		}
        // for compatibility with ARCHIVER 1050 speed change
        if (dir & VCCRDY) {
            m_motor->SetSpeed((val & VCCRDY) ? true : false);
        }
	}
}

void Riot1050::ChangePortDir(int portNum, unsigned char dir)
{
	if (portNum == 1) {
		m_sio->EnableDataLineOut((dir & DATAOUT) ? true : false);
	}
}
