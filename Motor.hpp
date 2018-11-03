//
// Motor class
// (c) 2016 Eric BACHER
//

#ifndef MOTOR_HPP
#define MOTOR_HPP 1

#include "Cpu6502.hpp"
#include "Track.hpp"

enum eMOTOR { MOTOR_TWO_PHASES, MOTOR_ONE_PHASE };

#define MOTOR_PHASE1     0x08
#define MOTOR_PHASE2     0x04
#define MOTOR_PHASE3     0x02
#define MOTOR_PHASE4     0x01
#define MOTOR_ALL_PHASES (MOTOR_PHASE1 | MOTOR_PHASE2 | MOTOR_PHASE3 | MOTOR_PHASE4)

// This class emulates a drive motor chip.
class Motor {

private:

	Cpu6502					*m_cpu;
	Track					*m_track;
    eMOTOR					m_motorType;
	bool					m_motorOn;
	int						m_phase;
	int						m_steppingTime;
    int						m_currentStep;
    int						m_nextStep;
    int						m_currentTrack;
    int						m_allowedPhases[4];
    int						m_maxStep;
    bool                    m_normalSpeed;
	long					m_frequency;
	int						m_initialSteppingTime;

public:

    // constructor and destructor
                            Motor(Cpu6502 *cpu, Track *disk, long frequency, eMOTOR motorType);
	virtual					~Motor() { }
	virtual void			Reset(void);
    virtual void			SetReady(bool bReady);

	// getter
	virtual Track			*GetTrack(void) { return m_track; }
	virtual int				GetPhase(void) { return m_phase; }
	virtual int				GetCurrentTrack(void) { return m_currentTrack; }
	virtual bool			GetMotorOn(void) { return m_motorOn; }

    // activate stepper motor
	virtual void			SetMotorOn(bool bOn);
	virtual void			Rotate(int phase);
	virtual void			Step(int nbTicks);

    // change disk motor rotation speed
    virtual void			SetSpeed(bool normalSpeed);
};

#endif
