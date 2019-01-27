//
// Sio class
// (c) 2016 Eric BACHER
//

#include "Emulator.h"

Sio::Sio(Cpu6502 *cpu, long frequency, int ticksBetweenBits, int ticksAfterStartBit)
{
	m_cpu = cpu;
	m_frequency = frequency;
	m_ticksBetweenBits = ticksBetweenBits;
	m_ticksAfterStartBit = ticksAfterStartBit;
	Reset();
}

void Sio::Reset(void)
{
	m_vccLine = true;
	m_computerState = STATE_IDLE;
	m_computerTicks = 0;
	m_commandLine = false;
	m_dataLineIn = false;
	m_dataLineOut = true;
}

void Sio::SetState(eSIOSTATE state)
{
	if (m_computerState != state) {
        eSIOSTATE oldSimplifiedState = (m_computerState == STATE_DATA_OUTPUT_BITS) ? STATE_DATA_OUTPUT : m_computerState;
        eSIOSTATE newSimplifiedState = (state == STATE_DATA_OUTPUT_BITS) ? STATE_DATA_OUTPUT : state;
        if (oldSimplifiedState != newSimplifiedState) {
            m_cpu->Trace(SIO_MODULE, true, "Set SIO state to %s", m_stateNames[state]);
        }
        m_computerState = state;
        //m_cpu->Trace(SIO_MODULE, true, "Set SIO state to %s", m_stateNames[state]);
	}
}

void Sio::PrepareBits(unsigned char data)
{
	m_invertedBits[0] = true; // start bit
	for (int i = 0; i < 8; i++) {
		m_invertedBits[i + 1] = (data & (1 << i)) ? false : true; // data bit
	}
	m_invertedBits[9] = false; // stop bit
}

void Sio::StartFrameTransmission(void)
{
    m_computerTicks += m_ticksBetweenBits;
    m_remainingBytes = m_dataLenth;
    PrepareBits(m_dataFrame[m_dataLenth - m_remainingBytes]);
    m_cpu->Trace(SIO_MODULE, true, "Receiving byte : %02X", ((unsigned int)m_dataFrame[m_dataLenth - m_remainingBytes]) & 0xFF);
    m_remainingBytes--;
    m_bitNumber = 0;
    m_dataLineIn = m_invertedBits[m_bitNumber];
    m_bitNumber++;
}

bool Sio::ContinueFrameTransmission(int nbTicks)
{
    if (m_bitNumber < 10) {
        m_computerTicks -= nbTicks;
        if (m_computerTicks <= 0) {
            m_computerTicks += m_ticksBetweenBits;
            m_dataLineIn = m_invertedBits[m_bitNumber];
            if ((m_bitNumber >= 1) && (m_bitNumber < 9)) {
                m_cpu->Trace(SIO_MODULE, true, "Receiving bit %d : %d", m_bitNumber - 1, m_dataLineIn ? 0 : 1);
            }
            m_bitNumber++;
            if (m_bitNumber == 10) {
                if (m_remainingBytes > 0) {
                    PrepareBits(m_dataFrame[m_dataLenth - m_remainingBytes]);
                    m_cpu->Trace(SIO_MODULE, true, "Receiving byte : %02X", ((unsigned int)m_dataFrame[m_dataLenth - m_remainingBytes]) & 0xFF);
                    m_remainingBytes--;
                    m_bitNumber = 0;
                }
                else {
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool Sio::DetectByteTransmission(void)
{
	if ((m_dataLineOut == false) && m_dataLineOutEnabled) {
		m_remainingBytes = 0;
		m_remainingBits = 9;
		m_currentByte = 0;
        m_ticksBetweenPortB = 0;
        m_ticksFirstPortB = 0;
        m_byteSent = false;
		return true;
	}
	return false;
}

bool Sio::ContinueByteTransmission(int nbTicks)
{
    if ((m_byteSent) || (! m_dataLineOutEnabled)) {
        return false;
    }
    m_computerTicks -= nbTicks;
    if (m_computerTicks <= 0) {
        return false;
    }
    return true;
}

void Sio::SetDataLineOut(bool bit)
{
    if (((m_computerState == STATE_CMD_ACK_BITS) || (m_computerState == STATE_CMD_COMPLETE_BITS) || (m_computerState == STATE_DATA_OUTPUT_BITS)
         || (m_computerState == STATE_DATA_ACK_BITS) || (m_computerState == STATE_DATA_COMPLETE_BITS))) {
        if ((! m_byteSent) && (m_remainingBits > 0)) {
			m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
            m_remainingBits--;
            if (m_remainingBits > 0) {
                if (m_remainingBits == 8) {
                    m_ticksFirstPortB = m_ticksSinceReset;
                }
                else if ((m_remainingBits == 1) && (m_ticksFirstPortB != 0)) {
                    m_ticksBetweenPortB = (m_ticksSinceReset - m_ticksFirstPortB) / 7;
                }
                m_cpu->Trace(SIO_MODULE, true, "Bit %d sent : %d", (8 - m_remainingBits), 0);
                m_currentByte >>= 1;
                if (bit) {
                    m_currentByte |= 0x80;
                }
            }
            else {
                int speed = (m_ticksBetweenPortB == 0) ? m_ticksBetweenBits : m_ticksBetweenPortB;
                m_cpu->Trace(SIO_MODULE, true, "Byte %02X sent using %d ticks/bit", (unsigned int)m_currentByte, speed);
                m_byteSent = true;
            }
        }
    }
    m_dataLineOut = bit;
}

void Sio::Step(int nbTicks)
{
    m_ticksSinceReset += (long) nbTicks;
	switch (m_computerState) {

		// Starting point for command frame
		case STATE_CMD_START:
			m_computerTicks -= nbTicks;
			if (m_computerTicks <= 0) {
				SetState(STATE_COMMAND_LOW);
				m_computerTicks = TICKS_BEFORE_SENDING_FRAME;
				m_commandLine = true;
				m_dataLineIn = false;
				m_dataOutCount = 0;
			}
			break;
		case STATE_COMMAND_LOW:
			m_computerTicks -= nbTicks;
			if (m_computerTicks <= 0) {
				SetState(STATE_CMD_FRAME);
				StartFrameTransmission();
			}
			break;
		case STATE_CMD_FRAME:
            if (! ContinueFrameTransmission(nbTicks)) {
				SetState(STATE_COMMAND_HIGH);
				m_computerTicks = TICKS_BEFORE_SENDING_HIGH;
			}
			break;
		case STATE_COMMAND_HIGH:
			m_computerTicks -= nbTicks;
			if (m_computerTicks <= 0) {
				SetState(STATE_CMD_ACK);
				m_computerTicks = TICKS_WAITING_FOR_CMD_ACK;
				m_commandLine = false;
			}
			break;
		case STATE_CMD_ACK:
			if (DetectByteTransmission()) {
				SetState(STATE_CMD_ACK_BITS);
				m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
			}
			else {
				m_computerTicks -= nbTicks;
				if (m_computerTicks <= 0) {
					SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_CMD_ACK_BITS:
			if (! ContinueByteTransmission(nbTicks)) {
				if (m_currentByte == STATUS_ACK) {
					m_computerTicks += m_ticksBetweenBits;
				}
				else {
					m_computerTicks = 0;
				}
                m_commandAnswer = m_currentByte;
                SetState(STATE_IDLE);
            }
			break;

		// Starting point for command execution
		case STATE_CMD_BEFORE_COMPLETE:
			m_computerTicks -= nbTicks;
			if (m_computerTicks <= 0) {
				SetState(STATE_CMD_COMPLETE);
				m_computerTicks = TICKS_WAITING_FOR_COMPLETE;
			}
			break;
		case STATE_CMD_COMPLETE:
			// We do not know if more data is going to be sent from the computer to the drive.
			// Wait for COMPLETE byte but check if the firmware is polling Port B.
			// If polling is detected it means that the firmware is expecting more data.
			// See NUMBER_OF_PORTB_POLLING_THRESHOLD constant.
			if (DetectByteTransmission()) {
				SetState(STATE_CMD_COMPLETE_BITS);
                m_endCommandTicks = m_ticksSinceReset;
                m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
			}
			else {
				m_computerTicks -= nbTicks;
				if (m_computerTicks <= 0) {
					SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_CMD_COMPLETE_BITS:
			if (! ContinueByteTransmission(nbTicks)) {
				m_commandStatus = m_currentByte;
				SetState(STATE_DATA_OUTPUT);
                m_computerTicks = TICKS_WAITING_FOR_DATA_OUT;
			}
			break;
		case STATE_DATA_OUTPUT:
			// We do not know if output data is going to be sent to the computer.
			// Wait for a byte but check if the firmware is polling Port B.
			// If polling is detected it means that the firmware has nothing to send.
			if (DetectByteTransmission()) {
                SetState(STATE_DATA_OUTPUT_BITS);
				m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
			}
			else {
				m_computerTicks -= nbTicks;
				if (m_computerTicks <= 0) {
					m_commandAnswer = m_commandStatus;
                    m_endSendDataTicks = m_ticksSinceReset;
                    SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_DATA_OUTPUT_BITS:
			if (! ContinueByteTransmission(nbTicks)) {
				m_dataOutBuffer[m_dataOutCount++] = m_currentByte;
				SetState(STATE_DATA_OUTPUT);
				m_computerTicks += TICKS_WAITING_FOR_DATA_OUT;
			}
			break;

		// Starting point for data frame
		case STATE_DATA_FRAME:
            if (!ContinueFrameTransmission(nbTicks)) {
				SetState(STATE_DATA_ACK);
				m_computerTicks = TICKS_WAITING_FOR_DATA_ACK;
			}
			break;
		case STATE_DATA_ACK:
			if (DetectByteTransmission()) {
				SetState(STATE_DATA_ACK_BITS);
				m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
			}
			else {
				m_computerTicks -= nbTicks;
				if (m_computerTicks <= 0) {
					SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_DATA_ACK_BITS:
			if (!ContinueByteTransmission(nbTicks)) {
				if (m_currentByte == STATUS_ACK) {
					SetState(STATE_DATA_BEFORE_COMPLETE);
					m_computerTicks += m_ticksBetweenBits;
				}
				else {
					m_commandAnswer = m_currentByte;
					SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_DATA_BEFORE_COMPLETE:
			m_computerTicks -= nbTicks;
			if (m_computerTicks <= 0) {
				SetState(STATE_DATA_COMPLETE);
				m_computerTicks = TICKS_WAITING_FOR_COMPLETE;
			}
			break;
		case STATE_DATA_COMPLETE:
			if (DetectByteTransmission()) {
				SetState(STATE_DATA_COMPLETE_BITS);
				m_computerTicks = m_ticksBetweenBits + m_ticksAfterStartBit;
			}
			else {
				m_computerTicks -= nbTicks;
				if (m_computerTicks <= 0) {
					SetState(STATE_IDLE);
					m_computerTicks = 0;
				}
			}
			break;
		case STATE_DATA_COMPLETE_BITS:
			if (!ContinueByteTransmission(nbTicks)) {
				m_commandAnswer = m_currentByte;
				SetState(STATE_IDLE);
			}
			break;
		default:
			SetState(STATE_IDLE);
			m_computerTicks = 0;
			break;
	}
}

void Sio::ReceiveCommandFrame(unsigned char *commandFrame)
{
    if (m_vccLine) {
        m_dataFrame = commandFrame;
        m_dataLenth = 5;
        SetState(STATE_CMD_START);
        m_computerTicks = TICKS_BEFORE_COMMAND_LOW;
        m_commandLine = false;
        m_commandAnswer = NO_STATUS;
    }
}

void Sio::ExecuteCommandFrame()
{
    SetState(STATE_CMD_BEFORE_COMPLETE);
    m_startCommandTicks = m_ticksSinceReset;
    m_endCommandTicks = 0L;
    m_endSendDataTicks = 0L;
    m_commandLine = false;
    m_commandAnswer = NO_STATUS;
}

void Sio::ReceiveDataFrame(unsigned char *dataFrame, int len)
{
	m_dataFrame = dataFrame;
	m_dataLenth = len;
    SetState(STATE_DATA_FRAME);
	StartFrameTransmission();
    m_computerTicks = m_ticksBetweenBits;
    m_commandLine = false;
    m_commandAnswer = NO_STATUS;
}
