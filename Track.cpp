//
// Track class
// (c) 2016 Eric BACHER
//

#include <memory.h>
#include <malloc.h>
#include "Track.hpp"
#include "Emulator.h"
#include "FirmwareDiskImage.hpp"

Track::Track(AtariDrive *drive, int rpm, long frequency, eTRACK trackType)
{
    m_drive = drive;
    m_rpm = rpm;
    m_frequency = frequency;
    m_trackType = trackType;
    m_currentRotationCycle = 0L;
    m_readCurrentDataBit = 0;
    m_writeCurrentDataBit = 0;
    // divisor depending on the frequency (4 for 810, 8 for 1050)
    m_divisor = (int)(frequency / 125000L);
    // number of CPU cycles per rotation depending on the CPU frequency and the drive RPM
    m_cyclesPerRotation = (60L * m_frequency) / (long)m_rpm;
    m_fastCyclesPerRotation = (60L * m_frequency) / (long)DISK_NORMAL_RPM; // (104166 for 810, 208333 for 1050)
    m_slowCyclesPerRotation = (60L * m_frequency) / (long)DISK_SLOWEST_RPM;
    // compute the number of data bits. There are 2 values:
    // - m_writeDataBitsPerRotation is the number of bits that can be written at the current RPM (depends only on m_rpm)
    // - m_readDataBitsPerRotation is the number of bits of the track (which may have been recorded at a different speed)
    m_writeDataBitsPerRotation = (int)((m_cyclesPerRotation + m_divisor - 1) / m_divisor);
    m_normalDataBitsPerRotation = (int)((m_fastCyclesPerRotation + m_divisor - 1) / m_divisor); // 26042
    m_maxDataBitsPerRotation = (int)((m_slowCyclesPerRotation + m_divisor - 1) / m_divisor);
    // round it to have a full byte at the end of the buffer (not real but easier to handle)
    m_writeDataBitsPerRotation = (int)((m_writeDataBitsPerRotation + 7) & 0xFFFFFFF8);
    m_normalDataBitsPerRotation = (int)((m_normalDataBitsPerRotation + 7) & 0xFFFFFFF8); // 26042
    m_maxDataBitsPerRotation = (int)((m_maxDataBitsPerRotation + 7) & 0xFFFFFFF8);
    // set current data bits
    SetReadDataBitsPerRotation(m_writeDataBitsPerRotation);
    // allocate a buffer (with the max size) where each byte contains one clock bit and one data bit
    m_trackReadBuffer = (unsigned char *)malloc(m_maxDataBitsPerRotation + 8);
    m_trackWriteBuffer = (unsigned char *)malloc(m_maxDataBitsPerRotation + 8);
    // unformatted content
    memset(m_trackReadBuffer, 0x80, m_readDataBitsPerRotation);
    m_readPreviousDataBit = -1;
    m_rotationNumber = 0;
    m_dirty = false;
    m_track00 = false;
    m_doubleDensity = false;
    m_diskInserted = false;
    m_motorOn = false;
    m_writtenDataBitCounter = 0;
    m_writeGate = false;
    m_writeIndex = 0;
    m_bresenhamFix = 0;
    m_trackNumber = 0;
}

Track::~Track()
{
    free(m_trackWriteBuffer);
    free(m_trackReadBuffer);
}

// this method is called to change the write speed.
// It computes how many bits we can write in the track
void Track::SetRPM(int rpm)
{
    m_rpm = rpm;
    // compute cycles per rotation from rpm
    m_cyclesPerRotation = (60L * m_frequency) / (long)m_rpm;
    // check that the current position is still inside the new limits
    if (m_currentRotationCycle > m_cyclesPerRotation) {
        m_currentRotationCycle -= m_cyclesPerRotation;
        m_readCurrentDataBit = (int)(m_currentRotationCycle / m_divisor);
    }
    // compute number of bits we can write at this speed
    m_writeDataBitsPerRotation = (int)(m_cyclesPerRotation / m_divisor);
    // compute speed used to create the track from the number of bits
    int initialWriteCyclesPerRotation = (int)(m_readDataBitsPerRotation * m_divisor);
    // determine how to adapt the number of bits read compared to the current cycles per rotation (in UpdateRotationCycle method)
    ComputeBresenhamValues(initialWriteCyclesPerRotation, m_cyclesPerRotation);
}

// this method is used to change the size of the current track.
// It is called when loading a track from the disk image (ATR, PRO, ATX,...) to set the number of bits that have been written in the track.
void Track::SetReadDataBitsPerRotation(int writeDataBitsPerRotation)
{
    // check if the requested size is too large for the allocated buffer.
    // This would be useless to accept large size as a real drive could not read it reliably
    if (writeDataBitsPerRotation > m_maxDataBitsPerRotation) {
        m_readDataBitsPerRotation = m_maxDataBitsPerRotation;
    }
    else {
        m_readDataBitsPerRotation = writeDataBitsPerRotation;
    }
    // check that the current position is still inside the new limits
    if (m_readCurrentDataBit > m_readDataBitsPerRotation) {
        m_readCurrentDataBit -= m_readDataBitsPerRotation;
    }
    // adapt number of cycles to be sure that the last bit of the track has the same number of cycles as the others
    m_cyclesPerRotation = m_readDataBitsPerRotation * m_divisor;
    // determine how to adapt the number of bits read compared to the current cycles per rotation (in UpdateRotationCycle method)
    ComputeBresenhamValues((int)m_cyclesPerRotation, m_cyclesPerRotation);
}

void Track::ComputeBresenhamValues(int originalWriteCyclesPerRotation, int currentReadCyclesPerRotation)
{
    // compute delta values for Bresenham algorithm
    if (originalWriteCyclesPerRotation > currentReadCyclesPerRotation) {
        m_bresenhamError = originalWriteCyclesPerRotation;
        m_bresenhamDy = currentReadCyclesPerRotation << 1;
        m_bresenhamFix = 1;
    }
    else if (originalWriteCyclesPerRotation < currentReadCyclesPerRotation) {
        m_bresenhamError = currentReadCyclesPerRotation;
        m_bresenhamDy = originalWriteCyclesPerRotation << 1;
        m_bresenhamFix = -1;
    }
    else {
        m_bresenhamError = originalWriteCyclesPerRotation;
        m_bresenhamDy = currentReadCyclesPerRotation << 1;
        m_bresenhamFix = 0;
    }
    m_bresenhamDx = m_bresenhamError << 1;
    m_bresenhamSum = 0;
}

void Track::UpdateRotationCycle(int nbTicks)
{
    for (int i = 0; i < nbTicks; i++) {
        // use the Bresenham algorithm to adapt the number of bits of the track to the current RPM speed
        m_currentRotationCycle++;
        m_bresenhamError = m_bresenhamError - m_bresenhamDy;
        if (m_bresenhamError <= 0) {
            m_bresenhamError += m_bresenhamDx;
        }
        else {
            m_bresenhamSum += m_bresenhamFix;
        }
        if (m_currentRotationCycle >= m_cyclesPerRotation) {
            // another rotation is completed
            m_currentRotationCycle -= m_cyclesPerRotation;
            m_bresenhamSum = 0;
            m_rotationNumber++;
            m_drive->Trace(TRK_MODULE, true, "Start of track");
        }
    }
    m_readCurrentDataBit = (int)((m_currentRotationCycle + m_bresenhamSum) / m_divisor);
    m_writeCurrentDataBit = (int)(m_currentRotationCycle / m_divisor);
}

void Track::Step(int nbTicks)
{
    if (m_motorOn) {
		UpdateRotationCycle(nbTicks);
        // check if we have found a clock mark (read mode only)
        if ((! m_writeGate) && (m_readPreviousDataBit != -1)) {
            if (m_readPreviousDataBit > m_readCurrentDataBit) {
                while (m_readPreviousDataBit < m_readDataBitsPerRotation) {
                    CheckClockMark(m_readPreviousDataBit);
                    m_readPreviousDataBit++;
                }
                m_readPreviousDataBit = 0;
            }
            while (m_readPreviousDataBit < m_readCurrentDataBit) {
                CheckClockMark(m_readPreviousDataBit);
                m_readPreviousDataBit++;
            }
        }
        m_readPreviousDataBit = m_readCurrentDataBit;
    }
}

void Track::CheckClockMark(int bitNumber)
{
    if (!m_diskInserted) {
        return;
    }
    unsigned char clock;
    unsigned char data;
    bitNumber -= 8;
    if (bitNumber < 0) {
        bitNumber += m_readDataBitsPerRotation;
    }
    int nextIndex = ReadByte(bitNumber, &clock, &data);
    if ((clock == DISK_ID_ADDR_MARK_CLOCK) && (data == DISK_ID_ADDR_MARK)) {
        unsigned char header[6];
        Crc16 crc16;
        crc16.Add(data);
        for (unsigned int i = 0; i < sizeof(header); i++) {
            nextIndex = ReadByte(nextIndex, &clock, &header[i]);
            if (i < sizeof(header) - 2) {
                crc16.Add(header[i]);
            }
        }
        unsigned short crc = (((unsigned int)header[4]) << 8) | (((unsigned int)header[5]) & 0xFF);
        m_idMarkIndex = bitNumber;
        if (crc == crc16.GetCrc()) {
            DumpIDAddressMarks(((unsigned int)header[0]) & 0xFF, ((unsigned int)header[1]) & 0xFF, ((unsigned int)header[2]) & 0xFF,
                    ((unsigned int)header[3]) & 0xFF, crc, bitNumber);
        }
    }
    if ((clock == DISK_DATA_ADDR_MARK_CLOCK) && (data >= DISK_DATA_ADDR_MARK1) && (data <= DISK_DATA_ADDR_MARK4)) {
        m_dataMarkIndex = bitNumber;
    }
}

int Track::SetWriteGate(bool writeOn)
{
    if (m_writeGate != writeOn) {
        if (writeOn) {
            // this is the point in the track where we start writing. A separate buffer is used for writing.
            // But at the end of the FDC operation, the write buffer is merged with the read buffer starting at this index.
            m_readStartDataBit = m_readCurrentDataBit;
            // this index is updated by WriteByte and is the next offset in m_trackWriteBuffer when the FDC writes data.
            m_writeIndex = 0;
            // we keep track of the number of bits to know if the write operation has written more than the track capacity.
            m_writtenDataBitCounter = 0;
        }
        else {
            // fill read buffer with data in write buffer
            FlushWrite();
        }
        // reset address mark detection
        m_idMarkIndex = false;
        m_dataMarkIndex = false;
        // switch Read/Write mode
        m_writeGate = writeOn;
    }
    return m_writeCurrentDataBit;
}

void Track::FlushWrite(void)
{
    if ((m_writeGate) && (m_writtenDataBitCounter > 0)) {
        if (m_writtenDataBitCounter > m_writeDataBitsPerRotation) {
            // a wrapping occurred in the write buffer. It means that a full track and maybe more has been written
            //
            // a full track has been written and no wrapping occurred in the read buffer
            // |++++++++++++++++++S+++++++++++++++++E++++++| read buffer (S=m_readStartDataBit, E=m_readCurrentDataBit)
            // |*****************I**********************************| write buffer (I=m_writeIndex)
            // |******************S*****************E***************| read buffer after merge
            // or
            // a full track has been written and a wrapping occurred in the read buffer
            // |+++++++++E++++++++S++++++++++++++++++++++++| read buffer (S=m_readStartDataBit, E=m_readCurrentDataBit)
            // |*****************************************I**********| write buffer (I=m_writeIndex)
            // |*********E********S*********************************| read buffer after merge
            memcpy(&m_trackReadBuffer[0],
                   &m_trackWriteBuffer[0],
                   m_writeDataBitsPerRotation);
            if (m_readDataBitsPerRotation != m_writeDataBitsPerRotation) {
                SetReadDataBitsPerRotation(m_writeDataBitsPerRotation);
            }
        }
        else {
            // no wrapping occurred in the write buffer. It means that part of the track (maybe a sector) has been written
            if (m_readCurrentDataBit > m_readStartDataBit) {
                // no wrapping occurred in the read buffer while writing in the write buffer
                // |------------------S+++++++++++++++++E------| read buffer (S=m_readStartDataBit, E=m_readCurrentDataBit)
                // |*******************I--------------------------------| write buffer (I=m_writeIndex)
                int nbOriginalDataBits = m_readCurrentDataBit - m_readStartDataBit;
                // make some space just before m_readCurrentDataBit (or remove space)
                // |------------------S+++++++++++++++++##E------|
                if (nbOriginalDataBits != m_writeIndex) {
                    memmove(&m_trackReadBuffer[m_readStartDataBit + m_writeIndex],
                            &m_trackReadBuffer[m_readCurrentDataBit],
                            m_readDataBitsPerRotation - m_readCurrentDataBit);
                }
                // and copy the write buffer into the read buffer
                // |------------------S*******************E------|
                memcpy(&m_trackReadBuffer[m_readStartDataBit],
                       &m_trackWriteBuffer[0],
                       m_writeIndex);
                if (nbOriginalDataBits != m_writeIndex) {
                    SetReadDataBitsPerRotation(m_readDataBitsPerRotation - (nbOriginalDataBits - m_writeIndex));
                }
            }
            else {
                // a wrapping occurred in the read buffer while writing in the write buffer
                // |+++++++++E--------S++++++++++++++++++++++++| read buffer (S=m_readStartDataBit, E=m_readCurrentDataBit)
                // |*****************************************I----------| write buffer (I=m_writeIndex)
                int nbOriginalDataBits = m_readCurrentDataBit + (m_readDataBitsPerRotation - m_readStartDataBit);
                int nbBitsToCopyAtEnd = m_writeIndex - m_readCurrentDataBit;
                // make some space at the end of the track (or remove space) and copy first data part
                // |+++++++++E--------S**************************|
                memcpy(&m_trackReadBuffer[m_readStartDataBit],
                       &m_trackWriteBuffer[0],
                       nbBitsToCopyAtEnd);
                // copy the second part of the write buffer
                // |*********E--------S**************************|
                memcpy(&m_trackReadBuffer[0],
                       &m_trackWriteBuffer[nbBitsToCopyAtEnd],
                       m_readCurrentDataBit);
                if (nbOriginalDataBits != m_writeIndex) {
                    SetReadDataBitsPerRotation(m_readDataBitsPerRotation - (nbOriginalDataBits - m_writeIndex));
                }
            }
        }
    }
}

void Track::Clear(void)
{
    // fill track with byte $00 (normal clock)
    memset(m_trackReadBuffer, 0x80, m_maxDataBitsPerRotation + 8);
    memset(m_trackWriteBuffer, 0x80, m_maxDataBitsPerRotation + 8);
}

void Track::Reset(void)
{
    ResetDirty();
    m_writeGate = false;
    m_trackNumber = 0;
    m_doubleDensity = m_diskImager->ReadTrack(m_trackNumber, this);
    m_track00 = true;
}

void Track::InsertDisk(bool bInserted, int currentTrack)
{
    bool oldInserted = m_diskInserted;
    m_diskInserted = bInserted;
    if ((! oldInserted) && (bInserted)) {
        ChangeTrack(currentTrack);
    }
}

void Track::ChangeTrack(int newTrackNumber)
{
    // newTrackNumber can be 0-79 on a 1050 due to half step.
    // It must be divided by 2 to know the "logical track". Odd numbers are between logical tracks
    FlushTrack();
    m_trackNumber = newTrackNumber;
    m_doubleDensity = m_diskImager->ReadTrack(m_trackNumber, this);
    m_track00 = (newTrackNumber == 0);
}

void Track::FlushTrack(void)
{
    FlushWrite();
    if (IsDirty()) {
        m_diskImager->WriteTrack(m_trackNumber, this);
    }
    ResetDirty();
    ResetIDMarkIndex();
    ResetDataMarkIndex();
}

int Track::FindNextIdAddressMark(int bitNumber)
{
    unsigned char clock;
    unsigned char data;
    if (m_diskInserted) {
        while (bitNumber < m_readDataBitsPerRotation) {
            ReadByte(bitNumber, &clock, &data);
            if ((clock == DISK_ID_ADDR_MARK_CLOCK) && (data == DISK_ID_ADDR_MARK)) {
                return bitNumber;
            }
            bitNumber++;
        }
    }
    return -1;
}

int Track::FindNextDataAddressMark(int bitNumber)
{
    unsigned char clock;
    unsigned char data;
    if (m_diskInserted) {
        for (int i = 0; i < DISK_MAX_BYTES_BETWEEN_ID_AND_DATA; i++) {
            int nextBitNumber = ReadByte(bitNumber, &clock, &data);
            if ((clock == DISK_DATA_ADDR_MARK_CLOCK) && (data >= DISK_DATA_ADDR_MARK1) && (data <= DISK_DATA_ADDR_MARK4)) {
                return bitNumber;
            }
            bitNumber = nextBitNumber;
        }
    }
    return -1;
}

int Track::AddBytesToIndex(int bitNumber, int nbBytes)
{
    bitNumber += nbBytes * 8;
    if (bitNumber >= m_readDataBitsPerRotation) {
        bitNumber -= m_readDataBitsPerRotation;
    }
    else if (bitNumber < 0) {
        bitNumber += m_readDataBitsPerRotation;
    }
    return bitNumber;
}

// byte can start at any bit (not aligned byte)
bool Track::IsOverByteRange(int bitNumber)
{
    int dataBitsPerRotation = m_writeGate ? m_writeDataBitsPerRotation : m_readDataBitsPerRotation;
    int currentDataBit = m_writeGate ? m_writeCurrentDataBit : m_readCurrentDataBit;
    bool wrap = false;
    int lastBitNumber = bitNumber + 7;
    if (lastBitNumber >= dataBitsPerRotation) {
        wrap = true;
        lastBitNumber -= dataBitsPerRotation;
    }
    if (wrap) {
        if ((currentDataBit >= bitNumber) && (currentDataBit <= dataBitsPerRotation)) {
            return true;
        }
        if ((currentDataBit >= 0) && (currentDataBit <= lastBitNumber)) {
            return true;
        }
    }
    else if ((currentDataBit >= bitNumber) && (currentDataBit <= lastBitNumber)) {
        return true;
    }
    return false;
}

bool Track::ReadData(int bitNumber)
{
    if ((bitNumber >= 0) && (bitNumber < m_readDataBitsPerRotation)) {
        return (m_trackReadBuffer[bitNumber] & 0x01) ? true : false;
    }
    return false;
}

bool Track::ReadClock(int bitNumber)
{
    if ((bitNumber >= 0) && (bitNumber < m_readDataBitsPerRotation)) {
        return (m_trackReadBuffer[bitNumber] & 0x80) ? true : false;
    }
    return false;
}

// write a data bit on track.
// This method is the one that really writes on track (as if it was the drive head)
void Track::WriteDataOnTrack(int bitNumber, bool bit)
{
    if (m_diskInserted) {
        if (m_writeGate) {
            // a write is performed by the FDC
            if (! IsDirty()) {
                m_dirty = true;
                m_diskImager->NotifyDirty();
            }
            if (bit) {
                m_trackWriteBuffer[bitNumber] |= 0x01;
            }
            else {
                m_trackWriteBuffer[bitNumber] &= 0xFE;
            }
        }
        else {
            // a write is performed by the disk imager to load the track into memory
            if (bit) {
                m_trackReadBuffer[bitNumber] |= 0x01;
            }
            else {
                m_trackReadBuffer[bitNumber] &= 0xFE;
            }
        }
        m_writtenDataBitCounter++;
    }
}

// write a clock bit on track.
// This method is the one that really writes on track (as if it was the drive head)
void Track::WriteClockOnTrack(int bitNumber, bool bit)
{
    if (m_diskInserted) {
        if (m_writeGate) {
            // a write is performed by the FDC
            if (! IsDirty()) {
                m_dirty = true;
                m_diskImager->NotifyDirty();
            }
            if (bit) {
                m_trackWriteBuffer[bitNumber] |= 0x80;
            }
            else {
                m_trackWriteBuffer[bitNumber] &= 0x7F;
            }
        }
        else {
            // a write is performed by the disk imager to load the track into memory
            if (bit) {
                m_trackReadBuffer[bitNumber] |= 0x80;
            }
            else {
                m_trackReadBuffer[bitNumber] &= 0x7F;
            }
        }
    }
}

// write a data bit on track.
// It goes through the drive to apply electronics for weak bits
void Track::WriteData(int bitNumber, bool bit)
{
    WriteDataOnTrack(bitNumber, bit);
}

// write a clock bit on track.
// It goes through the drive to apply electronics for weak bits
void Track::WriteClock(int bitNumber, bool bit)
{
    m_drive->WriteClock(bitNumber, bit);
}

int Track::ReadByte(int bitNumber, unsigned char *clock, unsigned char *data)
{
    *clock = *data = 0;
    if (bitNumber < 0) {
        bitNumber += m_readDataBitsPerRotation;
    }
    int nbMissingClock = 0;
    bool weakBits = false;
    for (int i = 0; i < 8; i++) {
        if (bitNumber >= m_readDataBitsPerRotation) {
            bitNumber -= m_readDataBitsPerRotation;
        }
        // count number of consecutive missing clock bits
        bool clockMark = ReadClock(bitNumber);
        if (clockMark) {
            nbMissingClock = 0;
        }
        else {
            nbMissingClock++;
            // check for 5 consecutive missing clocks to detect weak bits
            if (nbMissingClock >= 5) {
                weakBits = true;
            }
        }
        *clock |= clockMark ? (1 << (7 - i)) : 0;
        *data |= ReadData(bitNumber) ? (1 << (7 - i)) : 0;
        bitNumber++;
    }
    // return random values if weak bits are detected
    if (weakBits) {
        *data = qrand() & 0xFF;
    }
    return bitNumber;
}

int Track::ReadRawByte(int bitNumber, unsigned char *clock, unsigned char *data, unsigned char *weak)
{
    // ReadRawByte is similar to ReadByte except that it fills 'weak' parameter to know if there
    // are weak bits but it does not randomize data. This is to prevent the sector from changing
    // each time it is saved.
    *clock = *data = *weak = 0;
    if (bitNumber < 0) {
        bitNumber += m_readDataBitsPerRotation;
    }
    int nbMissingClock = 0;
    for (int i = 0; i < 8; i++) {
        if (bitNumber >= m_readDataBitsPerRotation) {
            bitNumber -= m_readDataBitsPerRotation;
        }
        // count number of consecutive missing clock bits
        bool clockMark = ReadClock(bitNumber);
        if (clockMark) {
            nbMissingClock = 0;
        }
        else {
            nbMissingClock++;
            // check for 5 consecutive missing clocks to detect weak bits
            if (nbMissingClock >= 5) {
                *weak = 0xFF;
            }
        }
        *clock |= clockMark ? (1 << (7 - i)) : 0;
        *data |= ReadData(bitNumber) ? (1 << (7 - i)) : 0;
        bitNumber++;
    }
    return bitNumber;
}

int Track::WriteByte(int bitNumber, unsigned char clock, unsigned char data)
{
    // Write Byte is called form FDC so it can really write only if the Write Gate is active
    if (m_writeGate) {
        for (int i = 0; i < 8; i++) {
            if (m_writeIndex >= m_writeDataBitsPerRotation) {
                m_writeIndex -= m_writeDataBitsPerRotation;
            }
            WriteClock(m_writeIndex, (clock & (1 << (7 - i))) ? true : false);
            WriteData(m_writeIndex, (data & (1 << (7 - i))) ? true : false);
            m_writeIndex++;
        }
    }
    bitNumber += 8;
    if (bitNumber >= m_writeDataBitsPerRotation) {
        bitNumber -= m_writeDataBitsPerRotation;
    }
    return bitNumber;
}

int Track::WriteRawByte(int bitNumber, unsigned char clock, unsigned char data, int num)
{
    // WriteRawByte is called from the disk imager to load a track in memory.
    // It should fill read buffer that's why the write gate is turned off
    bool oldWriteGate = m_writeGate;
    m_writeGate = false;
    while (num-- > 0) {
        if (bitNumber < 0) {
            bitNumber += m_readDataBitsPerRotation;
        }
        for (int i = 0; i < 8; i++) {
            if (bitNumber >= m_readDataBitsPerRotation) {
                bitNumber -= m_readDataBitsPerRotation;
            }
            WriteClock(bitNumber, (clock & (1 << (7 - i))) ? true : false);
            WriteData(bitNumber, (data & (1 << (7 - i))) ? true : false);
            bitNumber++;
        }
    }
    m_writeGate = oldWriteGate;
    return bitNumber;
}

int Track::WriteRawBytes(int bitNumber, unsigned char clock, unsigned char *bytes, int len)
{
    while (len-- > 0) {
        bitNumber = WriteRawByte(bitNumber, clock, *bytes++);
    }
    return bitNumber;
}

void Track::MoveBits(int to, int from, int nbBits)
{
    if (to < from) {
        for (int i = 0; i < nbBits; i++) {
            m_trackReadBuffer[to + i] = m_trackReadBuffer[from + i];
        }
    }
    else {
        for (int i = nbBits - 1; i >= 0; i--) {
            m_trackReadBuffer[to + i] = m_trackReadBuffer[from + i];
        }
    }
}

void Track::CropBits(int to, int from)
{
    for (int i = 0; i < m_readDataBitsPerRotation - from; i++) {
        m_trackReadBuffer[to + i] = m_trackReadBuffer[from + i];
    }
    SetReadDataBitsPerRotation(m_readDataBitsPerRotation - from + to);
}

void Track::SetMotorOn(bool motorOn)
{
    m_motorOn = motorOn;
    m_diskImager->dumpMotorOnOff(motorOn, m_readCurrentDataBit, m_readDataBitsPerRotation);
}

void Track::SetSpeed(bool normalSpeed)
{
    m_drive->SetSpeed(normalSpeed);
}

void Track::DumpFDCCommand(const char *commandName, unsigned int command, unsigned int track, unsigned int sector, unsigned int data)
{
    m_diskImager->dumpFDCCommand(commandName, command, track, sector, data, m_readCurrentDataBit, m_readDataBitsPerRotation);
}

void Track::DumpIndexPulse(bool indexPulse)
{
    m_diskImager->dumpIndexPulse(indexPulse, m_readCurrentDataBit, m_readDataBitsPerRotation);
}

void Track::DumpIDAddressMarks(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned short crc, int bitNumber)
{
    m_diskImager->dumpIDAddressMarks(track, side, sector, size, crc, bitNumber, m_readDataBitsPerRotation);
}

void Track::DumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int bitNumber)
{
    m_diskImager->dumpDataAddressMark(track, side, sector, size, bitNumber, m_readDataBitsPerRotation);
}

void Track::DumpReadSector(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned char status, int bitNumber)
{
    m_diskImager->dumpReadSector(track, side, sector, size, status, bitNumber);
}

void Track::DumpCPUInstructions(char *line)
{
    m_diskImager->dumpCPUInstructions(line, m_readCurrentDataBit, m_readDataBitsPerRotation);
}
