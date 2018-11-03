//
// Riot6532 class
// license:BSD-3-Clause
// copyright-holders:Aaron Giles
// adapted by Eric BACHER
//

#ifndef RIOT6532_HPP
#define RIOT6532_HPP 1

#include "Cpu6502.hpp"
#include "Chip.hpp"

#define TIMER_FLAG      0x80
#define PA7_FLAG        0x40

#define CLEAR_LINE      0x00
#define ASSERT_LINE     0xFF

// This class emulates a RIOT chip.
class Riot6532 : public Chip {

private:

	// Port A & B
	unsigned char			m_portIn[2];
	unsigned char			m_portOut[2];
	unsigned char			m_portDir[2];

	// IRQ
	unsigned char			m_irqState;
	unsigned char			m_irqEnable;
	int						m_irq;

	// PA7
	unsigned char			m_pa7Dir;
	unsigned char			m_pa7Prev;

	// Timer
	unsigned char			m_timerShift;
	unsigned char			m_timerState;
	long					m_timerRemaining;

	// internal update methods
	void					UpdateIrqState();
	unsigned char			ApplyDir(int portNum);
	void					UpdatePa7State();

protected:

	Cpu6502					*m_cpu;

	virtual void			Step(int nbTicks);

public:

	// constructor and destructor
							Riot6532(Cpu6502 *cpu);
	virtual					~Riot6532() { }
	virtual void			Reset(void);

	virtual unsigned char	GetTimer();

	// overwrite to trigger action on Read/Write
	virtual void			ChangePortDir(int portNum, unsigned char dir) = 0;
	virtual void			WritePort(int portNum, unsigned char val, unsigned char dir) = 0;
	virtual unsigned char	ReadPort(int portNum) = 0;
	virtual void			TriggerIrq(int assertLine) = 0;

	// read/write Port registers
	virtual unsigned char	ReadRegister(unsigned short addr);
	virtual void			WriteRegister(unsigned short addr, unsigned char val);
};

#endif
