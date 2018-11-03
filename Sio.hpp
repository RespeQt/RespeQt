//
// Sio class
// (c) 2016 Eric BACHER
//

#ifndef SIO_HPP
#define SIO_HPP 1

#include "Cpu6502.hpp"

#define TICKS_BEFORE_COMMAND_LOW    100
#define TICKS_BEFORE_SENDING_FRAME  1000       // t0 min=750 microseconds max=1600 microseconds
#define TICKS_BEFORE_SENDING_HIGH   150        // t1 min=650 microseconds max=950 microseconds
#define TICKS_WAITING_FOR_CMD_ACK   8000       // t2 min=0 microseconds max=16000 microseconds
#define TICKS_BEFORE_SENDING_DATA   650        // t3 min=1000 microseconds max=1600 microseconds
#define TICKS_WAITING_FOR_DATA_ACK  8000       // t4 min=850 microseconds max=16000 microseconds
#define TICKS_WAITING_FOR_COMPLETE  125000000  // t5 min=250 microseconds max=255000000 microseconds
#define TICKS_WAITING_FOR_DATA_OUT  2500

#define NO_STATUS        0x00  // emulator internal status meaning that the firmware did not send anything to the computer
#define STATUS_ACK       0x41  // 'A
#define STATUS_COMPLETE  0x43  // 'C
#define STATUS_ERROR     0x45  // 'E
#define STATUS_NAK       0x4E  // 'N
#define WAIT_FOR_DATA    0xFF  // emulator internal status meaning that firmware is expecting more data

// when the automata is waiting for the COMPLETE byte from the firmware, if we find a certain number of port B polling,
// it means that the firmware is expecting more data
#define NUMBER_OF_PORTB_POLLING_THRESHOLD  500

enum eSIOSTATE {
    STATE_IDLE,
    STATE_CMD_START,
    STATE_COMMAND_LOW,
    STATE_CMD_FRAME,
    STATE_COMMAND_HIGH,
    STATE_CMD_ACK,
    STATE_CMD_ACK_BITS,
    STATE_CMD_BEFORE_COMPLETE,
    STATE_CMD_COMPLETE,
    STATE_CMD_COMPLETE_BITS,
    STATE_DATA_OUTPUT,
    STATE_DATA_OUTPUT_BITS,
    STATE_DATA_FRAME,
    STATE_DATA_ACK,
    STATE_DATA_ACK_BITS,
    STATE_DATA_BEFORE_COMPLETE,
    STATE_DATA_COMPLETE,
    STATE_DATA_COMPLETE_BITS
};

// This class emulates a SIO connector.
class Sio {

private:

    Cpu6502					*m_cpu;
    long					m_frequency;
    long					m_ticksSinceReset;
    int						m_ticksBetweenBits;
    int						m_ticksBetweenPortB;
    int						m_ticksAfterStartBit;
    long					m_ticksFirstPortB;
    long					m_startCommandTicks;
    long					m_endCommandTicks;
    long					m_endSendDataTicks;

    // In and Out are relative to the drive (Out is from the Drive to the Computer).
    bool					m_vccLine;
    eSIOSTATE				m_computerState;
    int						m_computerTicks;
    int						m_remainingBits;
    int						m_remainingBytes;
    bool					m_invertedBits[10];
    int						m_bitNumber;
    unsigned char			m_currentByte;
    bool					m_byteSent;
    unsigned char			m_commandAnswer;
    bool					m_commandLine;
    unsigned char			m_commandStatus;
    bool					m_dataLineIn;
    bool					m_dataLineOut;
    bool					m_dataLineOutEnabled;
    int						m_dataOutCount;
    unsigned char			m_dataOutBuffer[1024];
    const char				*m_stateNames[18] = { "IDLE", "CMD_START", "COMMAND_LOW", "CMD_FRAME", "COMMAND_HIGH", "CMD_ACK", "CMD_ACK_BITS", "CMD_BEFORE_COMPLETE", "CMD_COMPLETE", "CMD_COMPLETE_BITS", "DATA_OUTPUT", "DATA_OUTPUT_BITS", "DATA_FRAME", "DATA_ACK", "DATA_ACK_BITS", "DATA_BEFORE_COMPLETE", "DATA_COMPLETE", "DATA_COMPLETE_BITS" };

    // Incoming data from Atari computer
    unsigned char			*m_dataFrame;
    int						m_dataLenth;

protected:

    void					StartFrameTransmission(void);
    bool					ContinueFrameTransmission(int nbTicks);
    bool					DetectByteTransmission(void);
    bool					ContinueByteTransmission(int nbTicks);
    void					SetState(eSIOSTATE state);
    void					PrepareBits(unsigned char data);

public:

    // constructor and destructor
                            Sio(Cpu6502 *cpu, long frequency, int ticksBetweenBits, int ticksAfterStartBit);
    virtual					~Sio() { }
    virtual void			Reset(void);

    // getters
    virtual bool			GetVccLine(void)             { return m_vccLine; }
    virtual bool			GetCommandLine(void)         { return m_commandLine; }
    virtual bool			GetDataLineIn(void)          { return m_dataLineIn; }
    virtual bool			GetDataLineOut(void)         { return m_dataLineOut; }
    virtual int				GetDataOutCount(void)        { return m_dataOutCount; }
    virtual unsigned char	*GetDataOutBuffer(void)      { return m_dataOutBuffer; }

    // setters
    virtual void			SetVccLine(bool bit)         { m_vccLine = bit; }
    virtual void			SetCommandLine(bool bit)     { m_commandLine = bit; }
    virtual void			SetDataLineIn(bool bit)      { m_dataLineIn = bit; }
    virtual void			SetDataLineOut(bool bit);    // used to send data from firmware to Atari
    virtual void			ResetDataOutCount(void)      { m_dataOutCount = 0; }
    virtual int             GetTicksBetweenPortB(void)   { return (m_ticksBetweenPortB != 0) ? m_ticksBetweenPortB : m_ticksBetweenBits; }
    virtual long            GetTicksForExecution(void)   { return (m_endCommandTicks != 0L) ? (long)(m_endCommandTicks - m_startCommandTicks) : 0L; }
    virtual long            GetTicksForSendingData(void) { return ((m_endCommandTicks != 0L) && (m_endSendDataTicks != 0L)) ? (long)(m_endSendDataTicks - m_endCommandTicks) : 0L; }

    // getters about sio state
    virtual bool			IsIdle(void)                 { return (m_computerState == STATE_IDLE) ? true : false; }
    virtual bool			IsWaitingForComplete(void)   { return (m_computerState == STATE_CMD_COMPLETE) ? true : false; }
    virtual unsigned char	GetCommandAnswer(void)       { return m_commandAnswer; }

    // enable/disable data out
    virtual void			EnableDataLineOut(bool on)   { m_dataLineOutEnabled = on; }

    // update transmission synchronized with the clock
    virtual void			Step(int nbTicks);
    virtual void			ReceiveCommandFrame(unsigned char *commandFrame);
    virtual void			ExecuteCommandFrame(void);
    virtual void			ReceiveDataFrame(unsigned char *dataFrame, int len);
};

#endif
