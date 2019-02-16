//
// FirmwareDiskImage class
// (c) 2018 Eric BACHER
//

#ifndef FIRMWAREDISKIMAGE_H
#define FIRMWAREDISKIMAGE_H

#include "diskimage.h"
#include "Emulator.h"

enum eWRITESTATE { STATE_WRITE_SECTOR_HEADER, STATE_WRITE_DATA_MARK, STATE_WRITE_SECTOR_DATA, STATE_WRITE_DATA_CRC };

class FirmwareDiskImage: public SimpleDiskImage
{
    Q_OBJECT

protected:

    QString                 m_hardwareName;
    AtariDrive              *m_atariDrive;
    bool                    m_displayDriveHead;
    bool                    m_displayFdcCommands;
    bool                    m_displayIndexPulse;
    bool                    m_displayMotorOnOff;
    bool                    m_displayIDAddressMarks;
    bool                    m_displayTrackInformation;
    bool                    m_displayCpuInstructions;
    bool                    m_powerOnWithDiskInserted;
    bool                    m_trackInformationDisplayed[40];
    bool                    m_initializing;
	bool                    m_traceOn;
	QString                 m_traceFilename;
	QFile                   *m_traceFile;
	QTextStream             *m_traceStream;
    unsigned int            m_lastReadSectorTrack;
    unsigned int            m_lastReadSectorNumber;
    unsigned char           m_lastReadSectorStatus;
    int                     m_lastReadSectorRotation;

    unsigned char           computeChksum(unsigned char *commandBuf, int len);
    virtual bool            writeErrorInsteadOfNak();
    virtual bool            writeStatus(unsigned char status);
    virtual bool            writeRawFrame(QByteArray data);
    virtual QString         commandDescription(quint8 command, quint16 aux);
    virtual int             getExpectedDataFrameSize(quint8 command, quint16 aux);
    virtual void            sendChipCode();
    virtual int             getUploadCodeStartAddress(quint8 command, quint16 aux, QByteArray &data);
    virtual bool            setTrace(bool on);

public:
                            FirmwareDiskImage(SioWorker *worker, eHARDWARE eHardware, QString &hardwareName, class Rom *rom, int iRpm, bool powerOnWithDiskInserted);
    virtual                 ~FirmwareDiskImage();
    void                    setDiskImager(FirmwareDiskImage *diskImager);

    virtual bool            open(const QString &fileName, FileTypes::FileType type);
    virtual void            close();
    virtual bool            save();
    virtual bool            saveAs(const QString &fileName);
    virtual void            setReady(bool bReady);
    virtual void            setLeverOpen(bool open);
    virtual void            setChipMode(bool enable);
    virtual void            handleCommand(quint8 command, quint16 aux);
    virtual QString         description() const;
    virtual QString         convertWd1771Status(quint8 status, quint16 weakBits, quint8 shortSize, int bitShift);

    // read or write a track (called from Track.cpp)
    virtual bool			ReadTrack(int trackNumber, Track *track);
    virtual void			FillTrackFromAtrImage(int trackNumber, Track *track);
    virtual void			FillTrackFromProImage(int trackNumber, Track *track);
    virtual void			FillTrackFromAtxImage(int trackNumber, Track *track);
    virtual void			FillTrackFromScpImage(int trackNumber, Track *track);
    virtual void			WriteTrack(int trackNumber, Track *track);
    virtual void			NotifyDirty();

    virtual bool            IsDisplayDriveHead(void);
    virtual void            SetDisplayDriveHead(bool active);
    virtual bool            IsDisplayFdcCommands(void);
    virtual void            SetDisplayFdcCommands(bool active);
    virtual bool            IsDisplayIndexPulse(void);
    virtual void            SetDisplayIndexPulse(bool active);
    virtual bool            IsDisplayMotorOnOff(void);
    virtual void            SetDisplayMotorOnOff(bool active);
    virtual bool            IsDisplayIDAddressMarks(void);
    virtual void            SetDisplayIDAddressMarks(bool active);
    virtual bool            IsDisplayTrackInformation(void);
    virtual void            SetDisplayTrackInformation(bool active);
    virtual bool            IsDisplayCpuInstructions(void);
    virtual void            SetDisplayCpuInstructions(bool active);
    virtual QString         GetTraceFilename(void);
    virtual void            SetTraceFilename(QString filename);

    virtual QString         hardwareName() const { return m_hardwareName; }
    void                    dumpFDCCommand(const char *commandName, unsigned int command, unsigned int track, unsigned int sector, unsigned int data, int currentRotation, int maxRotation);
    void                    dumpIndexPulse(bool indexPulse, int currentRotation, int maxRotation);
    void                    dumpMotorOnOff(bool motorOn, int currentRotation, int maxRotation);
    void                    dumpIDAddressMarks(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned short crc, int currentRotation, int maxRotation);
    void                    dumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int currentRotation, int maxRotation);
    void                    dumpReadSector(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned char status, int currentRotation);
    void                    dumpCPUInstructions(const char *buf, int currentRotation, int maxRotation);
    void                    dumpCommandToFile(QString &command);
    void                    dumpDataToFile(QByteArray &data);
    void					disassembleBuffer(unsigned char *data, int len, unsigned short address);
    void                    displayReadSectorStatus(unsigned int trackNumber, Track *track, unsigned int sectorNumber, unsigned char status, int currentRotation, int maxRotation);
    void					displayRawTrack(int trackNumber, Track *track);
    void					debugMessage(const char *msg, ...);
};

#endif // FIRMWAREDISKIMAGE_H
