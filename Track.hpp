//
// Track class
// (c) 2016 Eric BACHER
//

#ifndef TRACK_HPP
#define TRACK_HPP 1

enum eTRACK { TRACK_FULL, TRACK_HALF };

#define DISK_NORMAL_RPM 288
#define DISK_SLOW_RPM 272
#define DISK_SLOWEST_RPM 250 // very slow speed to be able to build very large track. This is NOT the real lowest speed supported by physical drive !

#define DISK_NORMAL_DATA_CLOCK      0xFF
#define DISK_DATA_ADDR_MARK_CLOCK   0xC7
#define DISK_INDEX_ADDR_MARK_CLOCK  0xD7
#define DISK_ID_ADDR_MARK_CLOCK     0xC7
#define DISK_NO_DATA_CLOCK          0x00 // weak byte

#define DISK_CRC_MARK         0xF7
#define DISK_DATA_ADDR_MARK1  0xF8
#define DISK_DATA_ADDR_MARK2  0xF9
#define DISK_DATA_ADDR_MARK3  0xFA
#define DISK_DATA_ADDR_MARK4  0xFB
#define DISK_INDEX_ADDR_MARK  0xFC
#define DISK_SPARE_MARK       0xFD
#define DISK_ID_ADDR_MARK     0xFE

#define DISK_MAX_BYTES_BETWEEN_ID_AND_DATA  28

class AtariDrive;
class FirmwareDiskImage;

// This class emulates a spinning disk with the current track where the head is located.
class Track {

private:

	// rotation speed, cpu speed and number of cycles per rotation
    AtariDrive				*m_drive;
    FirmwareDiskImage		*m_diskImager;
	int						m_rpm;
	long					m_frequency;
    eTRACK					m_trackType;
	long					m_cyclesPerRotation;
    long					m_fastCyclesPerRotation;
    long					m_slowCyclesPerRotation;
    int						m_readDataBitsPerRotation;
    int						m_writeDataBitsPerRotation;
    int						m_normalDataBitsPerRotation;
    int						m_maxDataBitsPerRotation;
    int						m_divisor;
    int						m_bresenhamFix;
    int						m_bresenhamError;
    int						m_bresenhamDx;
    int						m_bresenhamDy;
    int						m_bresenhamSum;

	// current position (in cpu cycles and bit number) on the track
    long					m_currentRotationCycle;
    int						m_readPreviousDataBit;
    int						m_readCurrentDataBit;
    int						m_writeCurrentDataBit;
    int						m_readStartDataBit;
    int						m_idMarkIndex;
	int						m_dataMarkIndex;
	int						m_rotationNumber;
    int						m_writtenDataBitCounter;
    bool                    m_writeGate;
    int						m_writeIndex;

	// track buffer. Each byte has 2 bits: bit 7 is the clock and bit 0 is the data
    unsigned char			*m_trackReadBuffer;
    unsigned char			*m_trackWriteBuffer;
    bool					m_dirty;
    bool					m_doubleDensity;
    bool					m_track00;
    bool					m_diskInserted;
    bool					m_motorOn;
    int						m_trackNumber;

public:

	// constructor and destructor
                            Track(AtariDrive *drive, int rpm, long frequency, eTRACK trackType);
	virtual					~Track();
    virtual void            SetDiskImager(FirmwareDiskImage *diskImager) { m_diskImager = diskImager; }

	// activate disk
	virtual void			Step(int nbTicks);
    virtual void			UpdateRotationCycle(int nbTicks);

	// change track
    virtual void			Reset(void);
    virtual void			Clear(void);
    virtual void			InsertDisk(bool bInserted, int currentTrack);
    virtual void			ChangeTrack(int newTrackNumber);
    virtual void			FlushTrack(void);

    // getter/setter
	virtual long			GetCyclesPerRotation(void) { return m_cyclesPerRotation; }
    virtual int				GetReadDataBitsPerRotation(void) { return m_readDataBitsPerRotation; }
    virtual int				GetNormalDataBitsPerRotation(void) { return m_normalDataBitsPerRotation; }
    virtual int				GetMaxDataBitsPerRotation(void) { return m_maxDataBitsPerRotation; }
    virtual int				GetRotationDataBit(void) { return m_readCurrentDataBit; }
	virtual int				AddBytesToIndex(int bitNumber, int nbBytes);
	virtual bool			IsOverByteRange(int bitNumber);
	virtual int				GetIDMarkIndex(void) { return m_idMarkIndex; }
	virtual void			ResetIDMarkIndex(void) { m_idMarkIndex = -1; }
	virtual int				GetDataMarkIndex(void) { return m_dataMarkIndex; }
	virtual void			ResetDataMarkIndex(void) { m_dataMarkIndex = -1; }
	virtual bool			IsDirty(void) { return m_dirty; }
	virtual void			ResetDirty(void) { m_dirty = false; }
	virtual int				GetRotationNumber(void) { return m_rotationNumber; }
    virtual eTRACK			GetTrackType(void) { return m_trackType; }
    virtual bool			IsDoubleDensity(void) { return m_doubleDensity; }
    virtual bool			IsOnTrack00(void) { return m_track00; }
    virtual void            SetMotorOn(bool motorOn);
    virtual void            SetSpeed(bool normalSpeed);
    virtual void            SetRPM(int rpm);
    virtual void            SetReadDataBitsPerRotation(int writeDataBitsPerRotation);
    virtual void            SetMaxDataBitsPerRotation(void) { SetReadDataBitsPerRotation(m_maxDataBitsPerRotation); }
    virtual void            ResetWrittenDataBitCounter(void) { m_writtenDataBitCounter = 0; }
    virtual void            AdjustWrittenDataBitCounter(int nbBits) { m_writtenDataBitCounter += nbBits; }
    virtual int             GetWrittenDataBitCounter(void) { return m_writtenDataBitCounter; }
    virtual void            FlushWrite(void);
    virtual int             SetWriteGate(bool writeOn);
    virtual bool            IsWriteGate(void) { return m_writeGate; }
    virtual void            ComputeBresenhamValues(int originalWriteCyclesPerRotation, int currentReadCyclesPerRotation);

	// locate address marks in the track
	virtual int				FindNextIdAddressMark(int bitNumber);
	virtual int				FindNextDataAddressMark(int bitNumber);

	// read and write clock and data byte on disk
	virtual int				ReadByte(int bitNumber, unsigned char *clock, unsigned char *data);
    virtual int				ReadRawByte(int bitNumber, unsigned char *clock, unsigned char *data, unsigned char *weak);
    virtual int				WriteByte(int bitNumber, unsigned char clock, unsigned char data);
    virtual int				WriteRawByte(int bitNumber, unsigned char clock, unsigned char data, int num = 1);
    virtual int				WriteRawBytes(int bitNumber, unsigned char clock, unsigned char *bytes, int len);

    // read and write clock or data bit on disk
    virtual bool			ReadData(int bitNumber);
    virtual bool			ReadClock(int bitNumber);
    virtual void			WriteData(int bitNumber, bool bit);
    virtual void			WriteClock(int bitNumber, bool bit);
    virtual void			WriteDataOnTrack(int bitNumber, bool bit);
    virtual void			WriteClockOnTrack(int bitNumber, bool bit);
    virtual void			MoveBits(int to, int from, int nbBits);
    virtual void			CropBits(int to, int from);
    virtual void			CheckClockMark(int bitNumber);

    // debug
    virtual void            DumpFDCCommand(const char *commandName, unsigned int command, unsigned int track, unsigned int sector, unsigned int data);
    virtual void            DumpIndexPulse(bool indexPulse);
    virtual void            DumpIDAddressMarks(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned short crc, int bitNumber);
    virtual void            DumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int bitNumber);
    virtual void            DumpReadSector(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned char status, int bitNumber);
    virtual void            DumpCPUInstructions(char *line);
};

#endif
