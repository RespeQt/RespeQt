//
// Riot1050 class
// (c) 2016 Eric BACHER
//

#ifndef RIOT1050_HPP
#define RIOT1050_HPP 1

#include "RiotDevices.hpp"

// Port A bits. Read drive number, motor ON flag and information from FDC
#define SWITCH1   0x01
#define SWITCH2   0x02
#define BANKSW    0x04
#define DRVMOTOR  0x08
#define HIGHTRK   0x10
#define SDENSITY  0x20
#define IRQ2793   0x40
#define DRQ2793   0x80

// Port B bits. Read and write to SIO, and stepper motor phases.
#define DATAOUT   0x01
#define VCCRDY    0x02
#define PHASE4    0x04
#define PHASE3    0x08
#define PHASE2    0x10
#define PHASE1    0x20
#define DATAIN    0x40
#define COMMAND   0x80

// This class emulates a riot with some devices connected to it (motor control and SIO lines).
class Riot1050 : public RiotDevices {

public:

	// constructor and destructor
							Riot1050(Cpu6502 *cpu, unsigned short driveNumber, Motor *motor, Sio *sio, Fdc *fdc);
	virtual					~Riot1050() { }

	// read/write Rom addresses
	virtual void			ChangePortDir(int portNum, unsigned char dir);
	virtual void			WritePort(int portNum, unsigned char val, unsigned char dir);
	virtual unsigned char	ReadPort(int portNum);
};

#endif
