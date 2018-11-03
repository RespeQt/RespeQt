//
// Riot6532 class
// license:BSD-3-Clause
// copyright-holders:Aaron Giles
// adapted by Eric BACHER
//

#include "Riot6532.hpp"
#include "Emulator.h"

enum
{
	TIMER_IDLE,
	TIMER_COUNTING,
	TIMER_FINISHING
};

Riot6532::Riot6532(Cpu6502 *cpu)
{
	m_cpu = cpu;
	Reset();
}

void Riot6532::Reset(void)
{
	/* reset I/O states */
	m_portIn[0] = 0;
	m_portOut[0] = 0;
	m_portDir[0] = 0;
	m_portIn[1] = 0;
	m_portOut[1] = 0;
	m_portDir[1] = 0;

	/* reset IRQ states */
	m_irqEnable = 0;
	m_irqState = 0;
	m_irq = CLEAR_LINE;

	/* reset PA7 states */
	m_pa7Dir = 0;
	m_pa7Prev = 0;

	/* reset timer states */
	m_timerShift = 10;
	m_timerState = TIMER_COUNTING;
	m_timerRemaining = 256 << m_timerShift;
}

unsigned char Riot6532::GetTimer()
{
	/* if idle, return 0 */
	if (m_timerState == TIMER_IDLE) {
		return 0;
	}

	/* if counting, return the number of ticks remaining */
	else if (m_timerState == TIMER_COUNTING) {
		return (unsigned char) (m_timerRemaining >> m_timerShift);
	}

	/* if finishing, return the number of ticks without the shift */
	else {
		return (unsigned char) (m_timerRemaining & 0xFF);
	}
}

void Riot6532::Step(int nbTicks)
{
	/* if we are still counting. decrement remaining time and check if it expires */
	if (m_timerState == TIMER_COUNTING) {
		m_timerRemaining -= nbTicks;
		if (m_timerRemaining <= 0) {
			m_timerState = TIMER_FINISHING;

			/* signal timer IRQ as well */
			m_irqState |= TIMER_FLAG;
			UpdateIrqState();
		}
	}

	/* if we finished counting, keep spinning */
	else if (m_timerState == TIMER_FINISHING) {
        if (m_timerRemaining <= 0) {
			m_timerRemaining -= nbTicks;
            if (m_timerRemaining <= -256) {
                m_timerRemaining += 256;

                /* signal timer IRQ as well */
                m_irqState |= TIMER_FLAG;
                UpdateIrqState();
            }
		}
	}
}

void Riot6532::UpdateIrqState()
{
	int irq = (m_irqState & m_irqEnable) ? ASSERT_LINE : CLEAR_LINE;

	if (m_irq != irq) {
		TriggerIrq(irq);
		m_irq = irq;
	}
}

unsigned char Riot6532::ApplyDir(int portNum)
{
	return (m_portOut[portNum] & m_portDir[portNum]) | (m_portIn[portNum] & ~m_portDir[portNum]);
}

void Riot6532::UpdatePa7State()
{
	unsigned char data = ApplyDir(0) & 0x80;

	/* if the state changed in the correct direction, set the PA7 flag and update IRQs */
	if ((m_pa7Prev ^ data) && (m_pa7Dir ^ data) == 0) {
		m_irqState |= PA7_FLAG;
		UpdateIrqState();
	}
	m_pa7Prev = data;
}

unsigned char Riot6532::ReadRegister(unsigned short addr)
{
	unsigned char val;

	/* if A2 == 1 and A0 == 1, we are reading interrupt flags */
	if ((addr & 0x05) == 0x05) {
		val = m_irqState;

		/* implicitly clears the PA7 flag */
		m_irqState &= ~PA7_FLAG;
		UpdateIrqState();
	}

	/* if A2 == 1 and A0 == 0, we are reading the timer */
	else if ((addr & 0x05) == 0x04) {
		val = GetTimer();

		/* A3 contains the timer IRQ enable */
		if (addr & 8) {
			m_irqEnable |= TIMER_FLAG;
		}
		else {
			m_irqEnable &= ~TIMER_FLAG;
		}

		/* implicitly clears the timer flag */
		if (m_timerState != TIMER_FINISHING || val != 0xFF) {
			m_irqState &= ~TIMER_FLAG;
		}
		UpdateIrqState();
	}

	/* if A2 == 0 and A0 == anything, we are reading from ports */
	else {
		/* A1 selects the port */
		int portNum = (addr & 0x02) >> 1;

		/* if A0 == 1, we are reading the port's DDR */
		if (addr & 1) {
			val = m_portDir[portNum];
		}

		/* if A0 == 0, we are reading the port as an input */
		else {
			/* call the input callback if it exists */
			m_portIn[portNum] = ReadPort(portNum);

			/* changes to port A need to update the PA7 state */
			if ((addr & 0x02) == 0) {
				UpdatePa7State();
			}

			/* apply the DDR to the result */
			val = ApplyDir((addr & 0x02) >> 1);
		}
	}
	return val;
}

void Riot6532::WriteRegister(unsigned short addr, unsigned char val)
{
	/* if A4 == 1 and A2 == 1, we are writing to the timer */
	if ((addr & 0x14) == 0x14) {
		/* A0-A1 contain the timer divisor */
		static const unsigned char timershift[4] = { 0, 3, 6, 10 };
		m_timerShift = timershift[addr & 3];

		/* A3 contains the timer IRQ enable */
		if (addr & 8) {
			m_irqEnable |= TIMER_FLAG;
		}
		else {
			m_irqEnable &= ~TIMER_FLAG;
		}

		/* writes here clear the timer flag */
		if (m_timerState != TIMER_FINISHING || GetTimer() != 0xFF) {
			m_irqState &= ~TIMER_FLAG;
		}
		UpdateIrqState();

		/* update the timer */
		m_timerState = TIMER_COUNTING;
		m_timerRemaining = 1 + (val << m_timerShift);
	}

	/* if A4 == 0 and A2 == 1, we are writing to the edge detect control */
	else if ((addr & 0x14) == 0x04) {
		/* A1 contains the A7 IRQ enable */
		if (addr & 0x02) {
			m_irqEnable |= PA7_FLAG;
		}
		else {
			m_irqEnable &= ~PA7_FLAG;
		}
        UpdateIrqState();

		/* A0 specifies the edge detect direction: 0=negative, 1=positive */
		m_pa7Dir = (addr & 1) << 7;
	}

	/* if A4 == anything and A2 == 0, we are writing to the I/O section */
	else {
		/* A1 selects the port */
		int portNum = (addr & 0x02) >> 1;

		/* if A0 == 1, we are writing to the port's DDR */
		if (addr & 1) {
			m_portDir[portNum] = val;
			ChangePortDir(portNum, val);
			WritePort(portNum, m_portOut[portNum], m_portDir[portNum]);
		}

		/* if A0 == 0, we are writing to the port's output */
		else {
			m_portOut[portNum] = val;
			WritePort(portNum, val, m_portDir[portNum]);
		}

		/* writes to port A need to update the PA7 state */
		if ((addr & 0x02) == 0) {
			UpdatePa7State();
		}
	}
}
