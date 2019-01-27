//
// Riot810 class
// (c) 2016 Eric BACHER
//

#ifndef RIOT810_HPP
#define RIOT810_HPP 1

#include "RiotDevices.hpp"

// Port A bits. Read drive number, motor ON flag and information from FDC
#define SWITCH1   0x01
#define DRVMOTOR  0x02
#define SWITCH2   0x04
#define BANKSW    0x08
#define WPT1771   0x10
#define VSS6532   0x20
#define IRQ1771   0x40
#define DRQ1771   0x80

// Port B bits. Read and write to SIO, and stepper motor phases.
#define DATAOUT   0x01
#define VCCRDY    0x02
#define PHASE4    0x04
#define PHASE3    0x08
#define PHASE2    0x10
#define PHASE1    0x20
#define COMMAND   0x40
#define DATAIN    0x80

// This class emulates a riot with some devices connected to it (motor control and SIO lines).
class Riot810 : public RiotDevices {

public:

	// constructor and destructor
							Riot810(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc);
	virtual					~Riot810() { }

	// read/write Rom addresses
	virtual void			ChangePortDir(int portNum, unsigned char dir);
	virtual void			WritePort(int portNum, unsigned char val, unsigned char dir);
	virtual unsigned char	ReadPort(int portNum);
};

#endif
