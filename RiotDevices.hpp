//
// RiotDevices class
// (c) 2016 Eric BACHER
//

#ifndef RIOTDEVICES_HPP
#define RIOTDEVICES_HPP 1

#include "Cpu6502.hpp"
#include "Riot6532.hpp"
#include "Motor.hpp"
#include "Sio.hpp"
#include "Fdc.hpp"

// This class emulates a riot with some devices connected to it (motor control and SIO lines).
class RiotDevices : public Riot6532 {

protected:

	// drive number (1=D1:, 2=D2:,...)
	unsigned short			m_driveNumber;

	 // bank selection
	unsigned short			m_bankNumber;

	// devices
	Motor					*m_motor;
	Sio						*m_sio;
	Fdc						*m_fdc;

	// sio state
	int						m_portBRead;
	bool					m_sioWaitingForMoreData;

public:

    // constructor and destructor
							RiotDevices(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc);
	virtual					~RiotDevices() { }
	virtual void			Reset(void);

	// getters and setters
	virtual Motor			*GetMotor() { return m_motor; }
	virtual Sio				*GetSio() { return m_sio; }
	virtual Fdc				*GetFdc() { return m_fdc; }
	virtual unsigned short	GetDriveNumber(void) { return m_driveNumber; }
	virtual unsigned short	GetBankNumber(void) { return m_bankNumber; }
	virtual void			SetBankNumber(unsigned short bankNumber) { m_bankNumber = bankNumber; }

	// sio access
	virtual bool			IsSioWaitingForMoreData(void);
	virtual bool			IsSioIdle(void) { return m_sio->IsIdle(); }
	virtual unsigned char	GetSioCommandAnswer(void) { return m_sio->GetCommandAnswer(); }
    virtual void			ReceiveSioCommandFrame(unsigned char *commandFrame);
    virtual void			ExecuteSioCommandFrame(void);
	virtual int				CopySioDataFrame(unsigned char *buf);
	virtual void			ReceiveSioDataFrame(unsigned char *buf, int len);

	// do work synchronized with time
	virtual void			Step(int nbTicks);

	// read/write Rom addresses
	virtual void			ChangePortDir(int portNum, unsigned char dir) = 0;
	virtual void			WritePort(int portNum, unsigned char val, unsigned char dir) = 0;
	virtual unsigned char	ReadPort(int portNum) = 0;
	virtual void			TriggerIrq(int assertLine);
};

#endif
