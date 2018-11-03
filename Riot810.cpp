//
// RiotDevices class
// (c) 2016 Eric BACHER
//

#include <stdio.h>
#include <string.h>
#include "Riot810.hpp"
#include "Emulator.h"

Riot810::Riot810(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc)
	: RiotDevices(cpu, driveNumber, motor, sio, fdc)
{
}

unsigned char Riot810::ReadPort(int portNum)
{
	if (portNum == 0) {
		static unsigned char switchMap[4] = { SWITCH1 | SWITCH2, SWITCH2, 0, SWITCH1 };
		unsigned char switches = switchMap[(m_driveNumber - 1) & (sizeof(switchMap) - 1)];
		unsigned char motor = m_motor->GetMotorOn() ? DRVMOTOR : 0;
		unsigned char bank = GetBankNumber() ? BANKSW : 0;
		unsigned char irq = m_fdc->HasIrq() ? IRQ1771 : 0;
		unsigned char drq = m_fdc->HasDrq() ? DRQ1771 : 0;
        return switches | motor | bank | irq | drq;
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

void Riot810::WritePort(int portNum, unsigned char val, unsigned char dir)
{
	if (portNum == 0) {
		if (dir & DRVMOTOR) {
			m_motor->SetMotorOn((val & DRVMOTOR) ? true : false);
		}
		// for compatibility with CHIP 810 bank switching
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
	}
}

void Riot810::ChangePortDir(int portNum, unsigned char dir)
{
	if (portNum == 1) {
		m_sio->EnableDataLineOut((dir & DATAOUT) ? true : false);
	}
}
