//
// Fdc class
// (c) 2016 Eric BACHER
//

#include "Fdc.hpp"
#include "Emulator.h"

static int steppingRates1771[4] = { 12, 12, 20, 40 };
static int steppingRates2793[4] = { 6, 12, 20, 30 };

Fdc::Fdc(Cpu6502 *cpu, Track *track, long frequency, bool invertedBus, eFDC fdcType)
{
    m_cpu = cpu;
    m_track = track;
    m_fdcType = fdcType;
    m_frequency = frequency;
    m_invertedBus = invertedBus;
    int *steppingRates = (fdcType == FDC_1771) ? steppingRates1771 : steppingRates2793;
    for (int i = 0; i < 4; i++) {
        m_r1r0Cycles[i] = (int)(((long)steppingRates[i] * m_frequency) / 1000L);
    }
    Reset();
    UpdateStatus();
}

void Fdc::Reset(void)
{
    m_busy = false;
    m_indexPulse = false;
    m_ready = true;
    m_crcError = false;
    m_seekError = false;
    m_headEngaged = false;
    m_writeProtect = false;
    m_drq = false;
    m_irq = false;
    m_lostData = false;
    m_idNotFound = false;
    m_recordNotFound = false;
    m_writeFault = false;
    m_recordType = 0;
    m_r1r0 = 0;
    m_a1a0 = 0;
    m_i3i2i1i0 = 0;
    m_verify = false;
    m_headLoad = false;
    m_update = false;
    m_hld = false;
    m_multiple = false;
    m_synchronize = false;
    m_block = false;
    m_sideCompare = 0;
    m_sideCompareEnabled = false;
    m_trackRegister = 0xFF;
    m_sectorRegister = 0xFF;
    m_dataRegister = 0xFF;
    m_commandRegister = 0x00;
    m_dsr = 0xFF;
    m_direction = 0x01;
    m_type = FDC_TYPE1;
    m_task = FDC_RESTORE;
    m_automaton = AUT_NONE;
    m_cycles = 0;
    m_multiple = false;
    m_doubleDensity = false;
    m_nextBitIndex = 0;
}

void Fdc::Step(int nbTicks)
{
    if (m_busy) {
        switch (m_automaton) {
        case AUT_DELAY_R1R0:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                switch ((int) m_task) {
                case FDC_RESTORE:
                    if (!IsOnTrack00()) {
                        m_trackRegister += m_direction;
                        if (m_trackRegister == 0xFF) {
                            m_seekError = true;
                            EndCommand();
                        }
                        else {
                            m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
                        }
                    }
                    else {
                        m_trackRegister = 0;
                        if (m_verify) {
                            m_automaton = AUT_CHK_TRACK;
                        }
                        else {
                            EndCommand();
                        }
                    }
                    break;
                case FDC_SEEK:
                    if (m_trackRegister != m_dsr) {
                        m_trackRegister += m_direction;
                        m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
                    }
                    else {
                        if (m_verify) {
                            m_automaton = AUT_CHK_TRACK;
                        }
                        else {
                            EndCommand();
                        }
                    }
                    break;
                case FDC_STEP:
                case FDC_STEPIN:
                case FDC_STEPOUT:
                    if (m_verify) {
                        m_automaton = AUT_CHK_TRACK;
                    }
                    else {
                        EndCommand();
                    }
                    break;
                }
            }
            break;
        case AUT_CHK_TRACK:
            // TODO
            m_automaton = AUT_NONE;
            break;
        case AUT_RSEC_WAIT_HEAD:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                m_automaton = AUT_RSEC_FETCH_ID;
                m_nbHoles = 5;
                m_track->ResetIDMarkIndex();
                m_previousIndexPulse = m_indexPulse;
            }
            break;
        case AUT_RSEC_FETCH_ID:
            m_nextBitIndex = m_track->GetIDMarkIndex();
            if ((m_nextBitIndex != -1) && (m_doubleDensity == m_track->IsDoubleDensity())) {
                m_idBitIndex = m_nextBitIndex;
                m_headerIndex = 0;
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                m_crc16.Reset();
                m_crc16.Add(m_header[m_headerIndex]);
                m_headerIndex++;
                m_track->ResetIDMarkIndex();
                m_automaton = AUT_RSEC_FETCH_HDR;
            }
            else if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_nbHoles--;
                if (m_nbHoles <= 0) {
                    m_recordNotFound = true;
                    EndCommand();
                }
            }
            else if (m_track->GetRotationNumber() >= m_maxRotation) {
                m_recordNotFound = true;
                EndCommand();
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case AUT_RSEC_FETCH_HDR:
            if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_nbHoles--;
            }
            m_previousIndexPulse = m_indexPulse;
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                if (m_headerIndex < (int) (sizeof(m_header) - 2)) { // header bytes except CRC
                    m_crc16.Add(m_header[m_headerIndex]);
                }
                m_headerIndex++;
                if (m_headerIndex == sizeof(m_header)) {
                    if (m_header[1] != m_trackRegister) {
                        m_track->ResetIDMarkIndex();
                        m_automaton = AUT_RSEC_FETCH_ID;
                    }
                    else if (m_header[3] != m_sectorRegister) {
                        m_track->ResetIDMarkIndex();
                        m_automaton = AUT_RSEC_FETCH_ID;
                    }
                    else {
                        unsigned short readCrc = ((((unsigned short)m_header[5]) << 8) | (((unsigned short)m_header[6]) & 0xFF));
                        if (readCrc != m_crc16.GetCrc()) {
                            m_track->ResetIDMarkIndex();
                            m_crcError = true;
                            m_automaton = AUT_RSEC_FETCH_ID;
                        }
                        else {
                            m_track->ResetDataMarkIndex();
                            m_crcError = false;
                            m_automaton = AUT_RSEC_FETCH_DATA;
                            // compute the maximum index before a Data Address Mark should be found
                            m_maxBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 30);
                        }
                    }
                }
            }
            break;
        case AUT_RSEC_FETCH_DATA:
            m_nextBitIndex = m_track->GetDataMarkIndex();
            if (m_nextBitIndex != -1) {
                DumpDataAddressMark(m_header[1], m_header[2], m_header[3], m_header[4], m_nextBitIndex);
                m_headerIndex = 0;
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_dsr);
                m_recordType = ~m_dsr & ((m_fdcType == FDC_2793) ? 0x01 : 0x03);
                m_automaton = AUT_RSEC_READ_DATA;
                m_crc16.Reset();
                m_crc16.Add(m_dsr);
                m_cpu->Trace(FDC_MODULE, true, "read byte %02X - Crc=%04X", ((unsigned int)m_dsr) & 0xFF, ((unsigned int)m_crc16.GetCrc()) & 0xFFFF);
                if (m_block) {
                    m_nbBytes = 128 << (int)(m_header[4] & 0x03);
                }
                else {
                    m_nbBytes = 16 << (int)m_header[4];
                }
            }
            else if (m_track->IsOverByteRange(m_maxBitIndex)) {
                m_recordNotFound = true;
                EndCommand();
            }
            break;
        case AUT_RSEC_READ_DATA:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                if (m_drq) {
                    m_lostData = true;
                    m_drq = false;
                }
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_dsr);
                m_dataRegister = m_dsr;
                m_drq = true;
                m_crc16.Add(m_dsr);
                m_nbBytes--;
                m_cpu->Trace(FDC_MODULE, true, "read byte %02X - remaining %d bytes - Crc=%04X", ((unsigned int)m_dsr) & 0xFF, m_nbBytes, ((unsigned int)m_crc16.GetCrc()) & 0xFFFF);
                if (m_nbBytes <= 0) {
                    m_automaton = AUT_RSEC_READ_CRC;
                    m_nbBytes = 2;
                }
            }
            break;
        case AUT_RSEC_READ_CRC:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                unsigned char clock;
                unsigned char data;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &data);
                m_nbBytes--;
                if (m_nbBytes == 1) {
                    m_dsr = data;
                }
                else if (m_nbBytes <= 0) {
                    unsigned short readCRC = ((((unsigned short)m_dsr) << 8) | (((unsigned short)data) & 0xFF));
                    if (readCRC != m_crc16.GetCrc()) {
                        m_crcError = true;
                        EndCommand();
                    }
                    else {
                        if ((m_multiple) && (m_sectorRegister < m_dataRegister)) {
                            m_sectorRegister++;
                            m_track->ResetIDMarkIndex();
                            m_drq = false;
                            m_nbHoles = 5;
                            m_automaton = AUT_RSEC_FETCH_ID;
                        }
                        else {
                            EndCommand();
                        }
                    }
                }
            }
            break;
        case AUT_WSEC_WAIT_HEAD:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                m_automaton = AUT_WSEC_FETCH_ID;
                m_nbHoles = 5;
                m_track->ResetIDMarkIndex();
                m_previousIndexPulse = m_indexPulse;
            }
            break;
        case AUT_WSEC_FETCH_ID:
            if (m_writeProtect) {
                EndCommand();
            }
            m_nextBitIndex = m_track->GetIDMarkIndex();
            if ((m_nextBitIndex != -1) && (m_doubleDensity == m_track->IsDoubleDensity())) {
                m_headerIndex = 0;
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                m_crc16.Reset();
                m_crc16.Add(m_header[m_headerIndex]);
                m_headerIndex++;
                m_track->ResetIDMarkIndex();
                m_automaton = AUT_WSEC_FETCH_HDR;
            }
            else if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_nbHoles--;
                if (m_nbHoles <= 0) {
                    m_recordNotFound = true;
                    EndCommand();
                }
            }
            else if (m_track->GetRotationNumber() >= m_maxRotation) {
                m_recordNotFound = true;
                EndCommand();
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case AUT_WSEC_FETCH_HDR:
            if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_nbHoles--;
            }
            m_previousIndexPulse = m_indexPulse;
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                if (m_headerIndex < (int) (sizeof(m_header) - 2)) { // header bytes except CRC
                    m_crc16.Add(m_header[m_headerIndex]);
                }
                m_headerIndex++;
                if (m_headerIndex == sizeof(m_header)) {
                    if (m_header[1] != m_trackRegister) {
                        m_track->ResetIDMarkIndex();
                        m_automaton = AUT_WSEC_FETCH_ID;
                    }
                    else if (m_header[3] != m_sectorRegister) {
                        m_track->ResetIDMarkIndex();
                        m_automaton = AUT_WSEC_FETCH_ID;
                    }
                    else {
                        unsigned short readCrc = ((((unsigned short)m_header[5]) << 8) | (((unsigned short)m_header[6]) & 0xFF));
                        if (readCrc != m_crc16.GetCrc()) {
                            m_track->ResetIDMarkIndex();
                            m_crcError = true;
                            m_automaton = AUT_WSEC_FETCH_ID;
                        }
                        else {
                            m_crcError = false;
                            m_automaton = AUT_WSEC_DELAY2;
                            m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 2);
                        }
                    }
                }
            }
            break;
        case AUT_WSEC_DELAY2:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                m_drq = true;
                m_automaton = AUT_WSEC_DELAY8;
                m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 8);
            }
            break;
        case AUT_WSEC_DELAY8:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                if (m_drq) {
                    m_lostData = true;
                    EndCommand();
                }
                m_automaton = AUT_WSEC_DELAY1;
                m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 1);
            }
            break;
        case AUT_WSEC_DELAY1:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                if ((m_fdcType == FDC_2793) && (m_doubleDensity)) {
                    m_automaton = AUT_WSEC_DELAY11;
                    m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 1);
                }
                else {
                    m_automaton = AUT_WSEC_WRITEGATE;
                    m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 1);
                    m_nbBytes = 6;
                }
            }
            break;
        case AUT_WSEC_DELAY11:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                m_automaton = AUT_WSEC_WRITEGATE;
                m_nextBitIndex = m_track->AddBytesToIndex(m_nextBitIndex, 1);
                m_nbBytes = 12;
            }
            break;
        case AUT_WSEC_WRITEGATE:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                m_automaton = AUT_WSEC_ZEROS;
                m_nextWriteBitIndex = m_track->SetWriteGate(true);
            }
            // fall thru
        case AUT_WSEC_ZEROS:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, 0);
                m_nbBytes--;
                if (m_nbBytes <= 0) {
                    m_automaton = AUT_WSEC_DAM;
                }
            }
            break;
        case AUT_WSEC_DAM:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                m_crc16.Reset();
                m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_DATA_ADDR_MARK_CLOCK, m_crc16.Add(m_a1a0));
                m_automaton = AUT_WSEC_DATA;
                if (m_block) {
                    m_nbBytes = 128 << (int)(m_header[4] & 0x03);
                }
                else {
                    m_nbBytes = 16 << (int)m_header[4];
                }
                m_dsr = m_dataRegister;
                m_drq = true;
            }
            break;
        case AUT_WSEC_DATA:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, m_crc16.Add(m_dsr));
                m_nbBytes--;
                if (m_nbBytes <= 0) {
                    m_automaton = AUT_WSEC_CRC;
                    m_nbBytes = 2;
                }
                else {
                    if (m_drq) {
                        m_lostData = true;
                        m_dsr = 0xFF;
                    }
                    else {
                        m_dsr = m_dataRegister;
                        m_drq = true;
                    }
                }
            }
            break;
        case AUT_WSEC_CRC:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                m_nbBytes--;
                if (m_nbBytes == 1) {
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, (unsigned char)((m_crc16.GetCrc() >> 8) & 0xFF));
                }
                else {
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, (unsigned char)(m_crc16.GetCrc() & 0xFF));
                    m_automaton = AUT_WSEC_ONE;
                }
            }
            break;
        case AUT_WSEC_ONE:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, 0xFF);
                if ((m_multiple) && (m_sectorRegister < m_dataRegister)) {
                    m_sectorRegister++;
                    m_track->SetWriteGate(false);
                    m_track->ResetIDMarkIndex();
                    m_drq = false;
                    m_nbHoles = 5;
                    m_automaton = AUT_WSEC_FETCH_ID;
                }
                else {
                    EndCommand();
                }
            }
            break;
        case AUT_RADDR_DELAY:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                m_automaton = AUT_RADDR_FETCH_ID;
            }
            break;
        case AUT_RADDR_FETCH_ID:
            m_nextBitIndex = m_track->GetIDMarkIndex();
            if ((m_nextBitIndex != -1) && (m_doubleDensity == m_track->IsDoubleDensity())) {
                m_headerIndex = 0;
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                m_dataRegister = m_header[m_headerIndex];
                m_crc16.Reset();
                m_crc16.Add(m_header[m_headerIndex]);
                m_drq = true;
                m_headerIndex++;
                m_track->ResetIDMarkIndex();
                m_automaton = AUT_RADDR_FETCH_HDR;
            }
            else if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_nbHoles--;
                if (m_nbHoles <= 0) {
                    m_recordNotFound = true;
                    EndCommand();
                }
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case AUT_RADDR_FETCH_HDR:
            if (m_track->IsOverByteRange(m_nextBitIndex)) {
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_header[m_headerIndex]);
                m_dataRegister = m_header[m_headerIndex];
                m_drq = true;
                if (m_headerIndex < (int) (sizeof(m_header) - 2)) { // header bytes except CRC
                    m_crc16.Add(m_header[m_headerIndex]);
                }
                m_headerIndex++;
                if (m_headerIndex == sizeof(m_header)) {
                    unsigned short readCrc = ((((unsigned short)m_header[5]) << 8) | (((unsigned short)m_header[6]) & 0xFF));
                    m_crcError = (readCrc != m_crc16.GetCrc()) ? true : false;
                    EndCommand();
                }
            }
            break;
        case AUT_RTRK_DELAY:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                m_automaton = AUT_RTRK_WAIT;
            }
            break;
        case AUT_RTRK_WAIT:
            if (!m_indexPulse) {
                m_automaton = AUT_RTRK_PULSE;
            }
            break;
        case AUT_RTRK_PULSE:
            if (m_indexPulse) {
                m_automaton = AUT_RTRK_READ;
                m_previousBitIndex = m_track->GetRotationDataBit();
                m_nextBitIndex = m_track->AddBytesToIndex(m_previousBitIndex, 1);
                m_previousIndexPulse = m_indexPulse;
            }
            break;
        case AUT_RTRK_READ:
            if ((m_track->IsOverByteRange(m_nextBitIndex)) && (m_doubleDensity == m_track->IsDoubleDensity())) {
                if (m_drq) {
                    m_lostData = true;
                }
                unsigned char clock;
                m_nextBitIndex = m_track->ReadByte(m_nextBitIndex, &clock, &m_dsr);
                m_dataRegister = m_dsr;
                m_drq = true;
            }
            else if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                EndCommand();
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case AUT_WTRK_DELAY:
            m_cycles -= nbTicks;
            if (m_cycles <= 0) {
                m_automaton = AUT_WTRK_WAIT;
                m_drq = true;
            }
            break;
        case AUT_WTRK_WAIT:
            if (!m_indexPulse) {
                m_automaton = AUT_WTRK_PULSE;
            }
            break;
        case AUT_WTRK_PULSE:
            if (m_indexPulse) {
                if (m_drq) {
                    m_lostData = true;
                    EndCommand();
                }
                else {
                    m_automaton = AUT_WTRK_WRITE;
                    m_previousBitIndex = m_track->GetRotationDataBit();
                    m_nextBitIndex = m_track->AddBytesToIndex(m_previousBitIndex, 1);
                    m_previousIndexPulse = m_indexPulse;
                    m_cpu->Trace(FDC_MODULE, true, "Starting Write Track at index %d", m_nextBitIndex);
					m_nextWriteBitIndex = m_track->SetWriteGate(true);
                }
            }
            break;
        case AUT_WTRK_WRITE:
            if (m_track->IsOverByteRange(m_nextWriteBitIndex)) {
                if (m_dataRegister == DISK_CRC_MARK) {
                    if (m_crc16.GetBytesInCrc() > 0) {
                        m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, (unsigned char)((m_crc16.GetCrc() >> 8) & 0xFF));
                        m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, (unsigned char)(m_crc16.GetCrc() & 0xFF));
                        m_crc16.Reset();
                    }
                    else {
                        // if DISK_CRC_MARK is issued and no bytes have been added in CRC, no CRC is written
                        m_crc16.Abort();
                        m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, m_crc16.Add(m_dataRegister));
                    }
                }
                else if (m_dataRegister == DISK_INDEX_ADDR_MARK) {
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_INDEX_ADDR_MARK_CLOCK, m_dataRegister);
                }
                else if (m_dataRegister == DISK_ID_ADDR_MARK) {
                    m_crc16.Reset();
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_ID_ADDR_MARK_CLOCK, m_crc16.Add(m_dataRegister));
                }
                else if ((m_dataRegister >= DISK_DATA_ADDR_MARK1) && (m_dataRegister <= DISK_DATA_ADDR_MARK4)) {
                    m_crc16.Reset();
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_DATA_ADDR_MARK_CLOCK, m_crc16.Add(m_dataRegister));
                }
                else {
                    m_nextWriteBitIndex = m_track->WriteByte(m_nextWriteBitIndex, DISK_NORMAL_DATA_CLOCK, m_crc16.Add(m_dataRegister));
                }
                m_dataRegister = 0x00;
                m_drq = true;
            }
            else if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                EndCommand();
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case AUT_FINT_INDEX:
            if ((m_indexPulse != m_previousIndexPulse) && (m_indexPulse)) {
                m_irq = true;
                m_automaton = AUT_NONE;
            }
            break;
        default:
            break;
        }
    }
    UpdateStatus();
}

void Fdc::UpdateStatus()
{
    unsigned char status = 0;
    if (!m_ready) {
        status |= 0x80;
    }
    if (m_type == FDC_TYPE1) {
        if (m_writeProtect) {
            status |= 0x40;
        }
        if (m_headEngaged) {
            status |= 0x20;
        }
        if (m_seekError) {
            status |= 0x10;
        }
        if (m_crcError) {
            status |= 0x08;
        }
        if (m_fdcType == FDC_2793) {
            if (! IsOnTrack00()) {
                status |= 0x04;
            }
        }
        if (m_indexPulse) {
            status |= 0x02;
        }
    }
    else {
        switch (m_task) {
        case FDC_READADDR:
            if (m_idNotFound) {
                status |= 0x10;
            }
            if (m_crcError) {
                status |= 0x08;
            }
            break;
        case FDC_READ:
            status |= (unsigned char) ((m_recordType & 0x03) << 5);
            if (m_recordNotFound) {
                status |= 0x10;
            }
            if (m_crcError) {
                status |= 0x08;
            }
            break;
        case FDC_WRITETRK:
            if (m_writeProtect) {
                status |= 0x40;
            }
            if (m_writeFault) {
                status |= 0x20;
            }
            // fall thru
        case FDC_WRITE:
            if (m_recordNotFound) {
                status |= 0x10;
            }
            if (m_crcError) {
                status |= 0x08;
            }
            break;
        default:
            break;
        }
        if (m_lostData) {
            status |= 0x04;
        }
        if (m_drq) {
            status |= 0x02;
        }
    }
    if (m_busy) {
        status |= 0x01;
    }
    m_statusRegister = status;
}

void Fdc::ResetFlags(void)
{
    m_crcError = false;
    m_seekError = false;
    m_drq = false;
    m_lostData = false;
    m_idNotFound = false;
    m_recordNotFound = false;
    m_writeFault = false;
    m_recordType = 0;
}

unsigned char Fdc::ReadRegister(unsigned short addr)
{
    switch (addr & 0x03) {
    case 0: // status
        m_irq = false;
        if (m_ready) {
            m_statusRegister &= 0x7F;
        }
        else {
            m_statusRegister |= 0x80;
        }
        if (m_invertedBus) {
            return m_statusRegister ^ 0xFF;
        }
        return m_statusRegister;
    case 1: // track
        if (m_invertedBus) {
            return m_trackRegister ^ 0xFF;
        }
        return m_trackRegister;
    case 2: // sector
        if (m_invertedBus) {
            return m_sectorRegister ^ 0xFF;
        }
        return m_sectorRegister;
    case 3: // data
        m_drq = false;
        if (m_invertedBus) {
            return m_dataRegister ^ 0xFF;
        }
        return m_dataRegister;
    }
    return 0; // unreachable
}

void Fdc::WriteRegister(unsigned short addr, unsigned char val)
{
    switch (addr & 0x03) {
    case 0: // command
        ExecuteCommand(m_invertedBus ? val ^ 0xFF : val);
        break;
    case 1: // track
        m_trackRegister = (m_invertedBus ? val ^ 0xFF : val);
        break;
    case 2: // sector
        m_sectorRegister = (m_invertedBus ? val ^ 0xFF : val);
        break;
    case 3: // data
        m_drq = false;
        m_dataRegister = (m_invertedBus ? val ^ 0xFF : val);
        break;
    }
}

void Fdc::SetIndexPulse(int assertLine)
{
    m_indexPulse = assertLine != 0 ? true : false;
    UpdateStatus();
    m_track->DumpIndexPulse(m_indexPulse);
}

void Fdc::DumpFDCCommand(void)
{
    m_track->DumpFDCCommand(m_commandName[m_task],
            ((unsigned int)m_commandRegister) & 0xFF, ((unsigned int)m_trackRegister) & 0xFF,
            ((unsigned int)m_sectorRegister) & 0xFF, ((unsigned int)m_dataRegister) & 0xFF);
}

void Fdc::DumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int bitNumber)
{
    m_track->DumpDataAddressMark(track, side, sector, size, bitNumber);
}

void Fdc::DumpCPUInstructions(char *line)
{
    m_track->DumpCPUInstructions(line);
}

void Fdc::ForceInterrupt(void)
{
    m_cpu->Trace(FDC_MODULE, true, "FDC command ForceInterrupt(%d)", ((unsigned int)m_i3i2i1i0) & 0x0F);
    m_busy = false;
    m_automaton = AUT_NONE;
    unsigned char i3i2i1i0 = m_i3i2i1i0 & 0x0F;
    if (i3i2i1i0 != 0) {
        if (i3i2i1i0 & FORCE_INTERRUPT_IMMEDIATE) {
            m_irq = true;
        }
        else if (i3i2i1i0 & FORCE_INTERRUPT_INDEX) {
            m_previousIndexPulse = m_indexPulse;
            m_automaton = AUT_FINT_INDEX;
        }
        // other flags are not supported (not used in Atari 810 for example)
    }
    UpdateStatus();
    m_track->SetWriteGate(false);
}

void Fdc::EndCommand(void)
{
    m_cpu->Trace(FDC_MODULE, true, "End of current command");
    m_busy = false;
    m_automaton = AUT_NONE;
    m_irq = true;
    m_track->SetWriteGate(false);
    UpdateStatus();
    if (m_task == FDC_READ) {
        m_track->DumpReadSector(m_header[1], m_header[2], m_header[3], m_header[4], 0xFF - m_statusRegister, m_idBitIndex);
    }
}

void Fdc::ExecuteCommand(unsigned char command)
{
    m_irq = false;
    bool isForceInterrupt = ((command & 0xF0) == 0xD0);
    if (isForceInterrupt) {
        if (m_busy) {
            m_i3i2i1i0 = command & 0x0F;
            ForceInterrupt();
        }
        else {
            m_type = FDC_TYPE1;
            m_drq = false;
            UpdateStatus();
        }
        return;
    }
    if (m_busy) {
        m_cpu->Trace(FDC_MODULE, true, "Cancel previous command");
        EndCommand();
    }
    m_busy = true;
    m_commandRegister = command;
    ResetFlags();
    DecodeCommand();
    if ((! m_ready) && ((m_type == FDC_TYPE2) || (m_type == FDC_TYPE3))) {
        m_cpu->Trace(FDC_MODULE, true, "Controller not ready");
    }
    else {
        DumpFDCCommand();
        StartCommand();
    }
    UpdateStatus();
}

void Fdc::DecodeCommand(void)
{
    if (m_commandRegister & 0x80) {
        if (m_commandRegister & 0x40) {
            if (m_commandRegister & 0x20) {
                m_type = FDC_TYPE3;
                m_eDelay = ((m_fdcType == FDC_2793) && (m_commandRegister & 0x04)) ? true : false;
                if (m_commandRegister & 0x10) {
                    m_task = FDC_WRITETRK;
                }
                else {
                    m_synchronize = (m_commandRegister & 0x01) ? false : true;
                    m_task = FDC_READTRK;
                }
            }
            else {
                if (m_commandRegister & 0x10) {
                    m_i3i2i1i0 = m_commandRegister & 0x0F;
                    m_type = FDC_TYPE4;
                    m_task = FDC_FORCEINT;
                }
                else {
                    m_type = FDC_TYPE3;
                    m_eDelay = ((m_fdcType == FDC_2793) && (m_commandRegister & 0x04)) ? true : false;
                    m_task = FDC_READADDR;
                }
            }
        }
        else {
            m_type = FDC_TYPE2;
            m_eDelay = ((m_fdcType == FDC_2793) && (m_commandRegister & 0x04)) ? true : false;
            if (m_commandRegister & 0x20) {
                if (m_fdcType == FDC_2793) {
                    m_a1a0 = DISK_DATA_ADDR_MARK3 | (~m_commandRegister & 0x01);
                    m_sideCompareEnabled = (m_commandRegister & 0x02) ? true : false;
                    m_sideCompare = (m_commandRegister & 0x08) >> 3;
                    m_block = true;
                }
                else {
                    m_a1a0 = DISK_DATA_ADDR_MARK1 | (~m_commandRegister & 0x03);
                    m_block = (m_commandRegister & 0x08) ? true : false;
                }
                m_hld = (m_commandRegister & 0x04) ? true : false;
                m_multiple = (m_commandRegister & 0x10) ? true : false;
                m_task = FDC_WRITE;
            }
            else {
                if (m_fdcType == FDC_2793) {
                    m_sideCompareEnabled = (m_commandRegister & 0x02) ? true : false;
                    m_sideCompare = (m_commandRegister & 0x08) >> 3;
                    m_block = true;
                }
                else {
                    m_block = (m_commandRegister & 0x08) ? true : false;
                }
                m_hld = (m_commandRegister & 0x04) ? true : false;
                m_multiple = (m_commandRegister & 0x10) ? true : false;
                m_task = FDC_READ;
            }
        }
    }
    else {
        m_type = FDC_TYPE1;
        m_r1r0 = m_commandRegister & 0x03;
        m_verify = (m_commandRegister & 0x04) ? true : false;
        m_headLoad = (m_commandRegister & 0x08) ? true : false;
        if (m_commandRegister & 0x40) {
            m_update = (m_commandRegister & 0x10) ? true : false;
            if (m_commandRegister & 0x20) {
                m_task = FDC_STEPOUT;
            }
            else {
                m_task = FDC_STEPIN;
            }
        }
        else {
            if (m_commandRegister & 0x20) {
                m_update = (m_commandRegister & 0x10) ? true : false;
                m_task = FDC_STEP;
            }
            else {
                if (m_commandRegister & 0x10) {
                    m_task = FDC_SEEK;
                }
                else {
                    m_task = FDC_RESTORE;
                }
            }
        }
    }
}

void Fdc::StartCommand(void)
{
    if (m_type == FDC_TYPE1) {
        if (m_hld) {
            m_headEngaged = true;
        }
        switch (m_task) {
        case FDC_RESTORE:
            m_trackRegister = 0xFF;
            m_dataRegister = 0x00;
            m_dsr = m_dataRegister;
            m_direction = 0xFF;
            if (!IsOnTrack00()) {
                m_trackRegister += m_direction;
                m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
                m_automaton = AUT_DELAY_R1R0;
            }
            else {
                m_trackRegister = 0;
                if (m_verify) {
                    m_automaton = AUT_CHK_TRACK;
                }
                else {
                    EndCommand();
                }
            }
            break;
        case FDC_SEEK:
            m_dsr = m_dataRegister;
            if (m_trackRegister != m_dsr) {
                m_direction = (m_dsr > m_trackRegister) ? 0x01 : 0xFF;
                m_trackRegister += m_direction;
                m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
                m_automaton = AUT_DELAY_R1R0;
            }
            else {
                if (m_verify) {
                    m_automaton = AUT_CHK_TRACK;
                }
                else {
                    EndCommand();
                }
            }
            break;
        case FDC_STEP:
            if (m_update) {
                m_trackRegister += m_direction;
            }
            m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
            m_automaton = AUT_DELAY_R1R0;
            break;
        case FDC_STEPIN:
            m_direction = 0x01;
            if (m_update) {
                m_trackRegister += m_direction;
            }
            m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
            m_automaton = AUT_DELAY_R1R0;
            break;
        case FDC_STEPOUT:
            m_direction = 0xFF;
            if (m_update) {
                m_trackRegister += m_direction;
            }
            m_cycles = m_r1r0Cycles[m_r1r0 & 0x03];
            m_automaton = AUT_DELAY_R1R0;
            break;
        default:
            break;
        }
    }
    else {
        m_headEngaged = true;
        m_drq = false;
        switch (m_task) {
        case FDC_READ:
            if (m_hld) {
                m_automaton = AUT_RSEC_WAIT_HEAD;
                m_cycles = (1000000L / m_frequency) * 10000L;
            }
            else {
                m_automaton = AUT_RSEC_FETCH_ID;
                m_nbHoles = 5;
                m_track->ResetIDMarkIndex();
                m_previousIndexPulse = m_indexPulse;
            }
            m_maxRotation = m_track->GetRotationNumber() + 2;
            break;
        case FDC_WRITE:
            if (m_hld) {
                m_automaton = AUT_WSEC_WAIT_HEAD;
                m_cycles = (1000000L / m_frequency) * 10000L;
            }
            else {
                m_automaton = AUT_WSEC_FETCH_ID;
                m_nbHoles = 5;
                m_track->ResetIDMarkIndex();
                m_previousIndexPulse = m_indexPulse;
            }
            m_maxRotation = m_track->GetRotationNumber() + 2;
            break;
        case FDC_READADDR:
            if (m_eDelay) {
                m_automaton = AUT_RADDR_DELAY;
                m_cycles = (1000000L / m_frequency) * 15000L;
            }
            else {
                m_automaton = AUT_RADDR_FETCH_ID;
            }
            m_nbHoles = 5;
            m_track->ResetIDMarkIndex();
            m_previousIndexPulse = m_indexPulse;
            break;
        case FDC_READTRK:
            if (m_fdcType == FDC_2793) {
                if (m_eDelay) {
                    m_automaton = AUT_RTRK_DELAY;
                    m_cycles = (1000000L / m_frequency) * 15000L;
                }
                else {
                    m_automaton = AUT_RTRK_WAIT;
                    m_drq = true;
                }
            }
            else {
                m_automaton = AUT_RTRK_DELAY;
                m_cycles = (1000000L / m_frequency) * 10000L;
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case FDC_WRITETRK:
            if (m_fdcType == FDC_2793) {
                if (m_eDelay) {
                    m_automaton = AUT_WTRK_DELAY;
                    m_cycles = (1000000L / m_frequency) * 15000L;
                }
                else {
                    m_automaton = AUT_WTRK_WAIT;
                    m_drq = true;
                }
            }
            else {
                m_automaton = AUT_WTRK_DELAY;
                m_cycles = (1000000L / m_frequency) * 10000L;
            }
            m_previousIndexPulse = m_indexPulse;
            break;
        case FDC_FORCEINT:
            ForceInterrupt();
            break;
        default:
            break;
        }
    }
}
