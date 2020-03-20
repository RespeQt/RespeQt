//
// Motor class
// (c) 2016 Eric BACHER
//

#include <cstring>
#include <cmath>
#include "Motor.hpp"
#include "Emulator.h"

// these tables and the associated code were taken from Altirra 3.0 sources (file diskdrivefull.cpp)
static const int kOffsetTables[2][16] = {

    // 810 (two adjacent phases required, noninverted)
    {
        -1, -1, -1,  1,
        -1, -1,  2, -1,
        -1,  0, -1, -1,
         3, -1, -1, -1
    },

    // 1050 (one phase required, inverted)
    {
        -1, -1, -1, -1,
        -1, -1, -1,  0,
        -1, -1, -1,  1,
        -1,  2,  3, -1
    }
};
// end of table taken from Altirra 3.0 sources (file diskdrivefull.cpp)

Motor::Motor(Cpu6502 *cpu, Track *track, long frequency, eMOTOR motorType)
{
	m_cpu = cpu;
	m_track = track;
	m_frequency = frequency;
    m_motorType = motorType;
    if (motorType == MOTOR_TWO_PHASES) {
        m_allowedPhases[0] = MOTOR_PHASE4 | MOTOR_PHASE1;
        m_allowedPhases[1] = MOTOR_PHASE3 | MOTOR_PHASE4;
        m_allowedPhases[2] = MOTOR_PHASE2 | MOTOR_PHASE3;
        m_allowedPhases[3] = MOTOR_PHASE1 | MOTOR_PHASE2;
        m_maxStep = 39;
        m_initialSteppingTime = 2600; // (5200L * m_frequency) / 1000000L;
    }
    else {
        m_allowedPhases[0] = MOTOR_PHASE2 | MOTOR_PHASE3 | MOTOR_PHASE4;
        m_allowedPhases[1] = MOTOR_PHASE1 | MOTOR_PHASE3 | MOTOR_PHASE4;
        m_allowedPhases[2] = MOTOR_PHASE1 | MOTOR_PHASE2 | MOTOR_PHASE4;
        m_allowedPhases[3] = MOTOR_PHASE1 | MOTOR_PHASE2 | MOTOR_PHASE3;
        m_maxStep = 79;
        m_initialSteppingTime = 4000; // 10000;
    }
	m_motorOn = false;
	m_steppingTime = 0;
	m_phase = m_allowedPhases[0];
    m_nextStep = m_currentStep = 0;
    m_normalSpeed = true;
    m_currentTrack = 0;
}

void Motor::Reset(void)
{
	m_motorOn = false;
	m_steppingTime = 0;
	m_phase = m_allowedPhases[0];
    m_nextStep = m_currentStep = 0;
    m_normalSpeed = true;
    m_track->Reset();
}

void Motor::SetReady(bool bReady)
{
    m_track->InsertDisk(bReady, m_currentStep);
}

void Motor::SetMotorOn(bool bOn)
{
    if (bOn != m_motorOn) {
        m_motorOn = bOn;
        m_track->SetMotorOn(bOn);
    }
}

void Motor::Rotate(int phase)
{
    // check if this new phase will move the head
    phase = phase & MOTOR_ALL_PHASES;
    int nextPhaseIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (phase == m_allowedPhases[i]) {
            nextPhaseIndex = i;
            break;
        }
    }
    if (nextPhaseIndex != -1) {
        int oldPhase = m_phase;
        m_phase = phase;
        int currentPhaseIndex = 0;
        for (int i = 0; i < 4; i++) {
            if (oldPhase == m_allowedPhases[i]) {
                currentPhaseIndex = i;
                break;
            }
        }
        // check if the head is already moving
        if (m_steppingTime != 0) {
            // TODO: if the head is already moving (between 2 tracks) the following code is wrong.
            // BUT the known firmwares do not send 2 consecutive step orders without waiting for the 5200 microseconds
            // So this is not an issue...
        }
        // check if the next phase is close to the current one
        int diff = nextPhaseIndex - currentPhaseIndex;
        if ((diff != 0) && (abs(diff) != 2)) {

            // fix diff
            if (diff == -3) {
                diff = 1;
            }
            else if (diff == 3) {
                diff = -1;
            }

            // check the physical limit of the head
            int step = m_currentStep + diff;
            if ((step >= 0) && (step <= m_maxStep)) {

                // this code is extracted from Altirra 3.0 sources (file diskdrivefull.cpp)
                const int newOffset = kOffsetTables[m_motorType][phase];
                int direction = 0;
                if (newOffset >= 0) {
                    switch(((unsigned int)newOffset - m_currentStep) & 3) {
                    case 1:		// step in (increasing track number)
                        direction = 1;
                        break;
                    case 3:		// step out (decreasing track number)
                        direction = -1;
                        break;
                    case 0:
                    case 2:
                    default:
                        // no step or indeterminate -- ignore
                        break;
                    }
                }
                // end of code extracted from Altirra 3.0 sources (file diskdrivefull.cpp)

                if (direction != 0) {
                    // check the physical limit of the head
                    int step = m_currentStep + direction;
                    if ((step >= 0) && (step <= m_maxStep)) {
                        // the head is going to move one step
                        int oldSteppingTime = m_steppingTime;
                        m_steppingTime = m_initialSteppingTime;
                        m_nextStep = step;
                        m_cpu->Trace(TRK_MODULE, true, "Seeking step %d (phase %02X) with a stepping time of %d ticks (previous steppingTime remaining %d)", m_nextStep, (int) phase<<2, m_steppingTime, oldSteppingTime);
                    }
                }
            }
        }
    }
}

void Motor::Step(int nbTicks)
{
	if (m_motorOn) {
		m_track->Step(nbTicks);
	}
	if (m_steppingTime != 0) {
		m_steppingTime -= nbTicks;
        if (m_steppingTime <= 0) {
            m_steppingTime = 0;
            m_track->ChangeTrack(m_nextStep);
            m_currentStep = m_nextStep;
            m_currentTrack = (m_motorType == MOTOR_TWO_PHASES) ? m_currentStep : (m_currentStep >> 1);
            m_cpu->Trace(TRK_MODULE, true, "Head is on step %d (logical track %d)", m_currentStep, m_currentTrack);
        }
	}
}

void Motor::SetSpeed(bool normalSpeed)
{
    if (m_normalSpeed != normalSpeed) {
        m_normalSpeed = normalSpeed;
        m_track->SetSpeed(normalSpeed);
    }
}
