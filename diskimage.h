/*
 * diskimage.h
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include <QFile>
#include <QList>
#include <QTemporaryFile>
#include <QElapsedTimer>
#include <QtDebug>

#include <crc16.h>
#include "disassembly810.h"
#include "disassembly1050.h"

#include "sioworker.h"
#include "miscutils.h"

#define DISK_CRC_MARK         0xF7
#define DISK_DATA_ADDR_MARK1  0xF8
#define DISK_DATA_ADDR_MARK2  0xF9
#define DISK_DATA_ADDR_MARK3  0xFA
#define DISK_DATA_ADDR_MARK4  0xFB
#define DISK_INDEX_ADDR_MARK  0xFC
#define DISK_SPARE_MARK       0xFD
#define DISK_ID_ADDR_MARK     0xFE

class AtariFileSystem;
class DiskEditDialog;

class DiskGeometry: public QObject
{
    Q_OBJECT

private:
    bool m_isDoubleSided;
    quint8 m_tracksPerSide;
    quint16 m_sectorsPerTrack;
    quint16 m_bytesPerSector;
    uint m_sectorCount;
    uint m_totalSize;

public:
    DiskGeometry(const DiskGeometry &other);
    DiskGeometry();
    void initialize(const DiskGeometry &other);
    void initialize(bool aIsDoubleSided, quint8 aTracksPerSide, quint16 aSectorsPerTrack, quint16 aBytesPerSector);
    void initialize(uint aTotalSize, quint16 aBytesPerSector);
    void initialize(uint aTotalSize);
    void initialize(const QByteArray &percom);
    bool isEqual(const DiskGeometry &other);
    bool isStandardSD() const;
    bool isStandardED() const;
    bool isStandardDD() const;
    bool isStandardDSDD() const;
    bool isStandardDSQD() const;
    inline bool isDoubleSided() const {return m_isDoubleSided;}
    inline quint8 tracksPerSide() const {return m_tracksPerSide;}
    inline quint16 sectorsPerTrack() const {return m_sectorsPerTrack;}
    inline quint16 bytesPerSector() const {return m_bytesPerSector;}
    quint16 bytesPerSector(quint16 sector);
    inline uint sectorCount() const {return m_sectorCount;}
    inline uint totalSize() const {return m_totalSize;}
    QByteArray toPercomBlock();
    QString humanReadable() const;
};

struct ProSectorInfo
{
    bool used;
    bool paired;
    bool inserted;
    bool firstPass;
    quint8 sectorNumber;
    quint8 totalDuplicate;
    quint8 duplicateOffset[5];
    quint8 driveStatus;
    quint8 wd1771Status;
    quint8 reservedByte;
    quint16 absoluteSector;
    bool notEmpty;
    quint8 fillByte;
    quint16 weakBits;
    quint8 sectorTiming;
    quint8 shortSectorSize;
    quint8 lastSectorRead;
    quint16 beforeSlot;
    QByteArray sectorData;
};

class AtxSectorInfo: public QObject
{
    Q_OBJECT

private:
    quint8 m_sectorNumber;
    quint8 m_sectorStatus;
    quint16 m_sectorPosition;
    QByteArray m_sectorData;
    quint16 m_sectorWeakOffset;

public:
    AtxSectorInfo(quint8 sectorNumber, quint8 sectorStatus, quint16 sectorPosition) {
        m_sectorNumber = sectorNumber;
        m_sectorStatus = sectorStatus;
        m_sectorPosition = sectorPosition;
        m_sectorWeakOffset = (quint16)0xFFFF;
    }
    virtual ~AtxSectorInfo() { }
    quint8 sectorNumber() { return m_sectorNumber; }
    quint8 sectorStatus() { return m_sectorStatus; }
    quint16 sectorPosition() { return m_sectorPosition; }
    QByteArray sectorData();
    QByteArray rawSectorData() { return m_sectorData; }
    quint8 byteAt(int pos);
    quint8 rawByteAt(int pos);
    quint8 fillByte();
    int size() { return (((wd1771Status() & 0x06) == 0) && ((wd1771Status() & 0x08) != 0)) ? m_sectorData.size() * 2 : m_sectorData.size(); }
    void setSectorData(QByteArray &sectorData) { m_sectorData = sectorData; }
    void copySectorData(const QByteArray &sectorData);
    quint16 sectorWeakOffset() { return m_sectorWeakOffset; }
    void setSectorWeakOffset(quint16 sectorWeakOffet);
    quint8 driveStatus() { return 0x10; }
    quint8 wd1771Status() { return (~m_sectorStatus & 0x3E) | 0xC1; }
    void setWd1771Status(quint8 status) { m_sectorStatus |= ~status & 0x3E; }
    int dataMarkOffset(int headerOffset, int shift);
};

class AtxTrackInfo: public QObject
{
    Q_OBJECT

private:
    QList<AtxSectorInfo*> m_sectors;

public:
    AtxTrackInfo() { }
    virtual ~AtxTrackInfo() { clear(); }
    AtxSectorInfo* add(quint8 sectorNumber, quint8 sectorStatus, quint16 sectorPosition);
    int count(quint8 sectorNumber);
    int numberOfExtendedSectors();
    int numberOfSectorsWithData();
    void clear();
    int size() { return m_sectors.size(); }
    AtxSectorInfo* at(int pos) { return m_sectors.at(pos); }
    AtxSectorInfo* find(quint8 sectorNumber, quint16 distance);
    int duplicateIndex(AtxSectorInfo *sectorInfo, int sectorNumber);
    int shortSectorSize(int track, int sectorIndex, int *bitShift);
};

enum BOOT_STATE
{
    NOT_BOOTED = 0,
    FIRST_SECTOR_1,
    READ_OTHER_SECTOR,
    SECOND_SECTOR_1
};

class Board: public QObject
{
    Q_OBJECT

protected:
    // For Chip 810 and Super Archiver 1050 emulation
    bool m_chipOpen;
    unsigned short m_lastArchiverUploadCrc16;
    quint16 m_lastArchiverSpeed;
    // For Happy 810/1050 emulation
    bool m_happyEnabled;
    bool m_happy1050;
    unsigned short m_lastHappyUploadCrc16;
    bool m_happyPatchInProgress;
    // For translator
    bool m_translatorActive;
    enum BOOT_STATE m_translatorState;
    // For tool disk
    bool m_toolDiskActive;

public:
    // For Chip 810 and Super Archiver 1050 emulation
    unsigned char m_chipRam[32];
    QByteArray m_trackData;
    // For Happy 810/1050 emulation
    QByteArray m_happyRam;

public:
    Board();
    ~Board();
    Board *getCopy();
    void setFromCopy(Board *info);
    bool hasHappySignature();
    inline void setChipOpen(bool open) {m_chipOpen = open;}
    inline bool isChipOpen() const {return m_chipOpen;}
    inline void setTranslatorActive(bool active) {m_translatorActive = active;}
    inline bool isTranslatorActive() const {return m_translatorActive && (m_translatorState != SECOND_SECTOR_1);}
    inline void setTranslatorState(enum BOOT_STATE state) {m_translatorState = state;}
    inline int getTranslatorState() const {return m_translatorState;}
    inline void setToolDiskActive(bool active) {m_toolDiskActive = active;}
    inline bool isToolDiskActive() const {return m_toolDiskActive;}
    inline void setLastArchiverUploadCrc16(unsigned short crc16) {m_lastArchiverUploadCrc16 = crc16;}
    inline unsigned short getLastArchiverUploadCrc16() const {return m_lastArchiverUploadCrc16;}
    inline void setLastArchiverSpeed(quint16 speed) {m_lastArchiverSpeed = speed;}
    inline quint16 getLastArchiverSpeed() const {return m_lastArchiverSpeed;}
    inline void setHappyEnabled(bool enabled) {m_happyEnabled = enabled;}
    inline bool isHappyEnabled() const {return m_happyEnabled;}
    inline void setHappy1050(bool is1050) {m_happy1050 = is1050;}
    inline bool isHappy1050() const {return m_happy1050;}
    inline void setHappyPatchInProgress(bool progress) {m_happyPatchInProgress = progress;}
    inline bool isHappyPatchInProgress() const {return m_happyPatchInProgress;}
    inline void setLastHappyUploadCrc16(unsigned short crc16) {m_lastHappyUploadCrc16 = crc16;}
    inline unsigned short getLastHappyUploadCrc16() const {return m_lastHappyUploadCrc16;}
};

class SimpleDiskImage: public SioDevice
{
    Q_OBJECT

public:
    SimpleDiskImage(SioWorkerPtr worker);
    virtual ~SimpleDiskImage();

    virtual bool open(const QString &fileName, FileTypes::FileType type);
    virtual bool create(int untitledName);
    virtual void reopen();
    virtual void close();
    virtual bool save();
    virtual bool saveAs(const QString &fileName);
    inline bool isOpen() const {return file.isOpen();}
    inline bool isReadOnly() const {return m_isReadOnly;}
    inline void setReadOnly(bool readOnly) {m_isReadOnly = readOnly;}
    inline bool isLeverOpen() const {return m_isLeverOpen;}
    virtual void setLeverOpen(bool open);
    inline bool isModified() const {return m_isModified;}
    inline bool isUnmodifiable() const {return m_isUnmodifiable;}
    inline bool isUnnamed() const {return m_isUnnamed;}
    inline bool isReady() const {return m_isReady;}
    inline bool isHappyEnabled() const {return m_board.isHappyEnabled();}
    inline bool isChipOpen() const {return m_board.isChipOpen();}
    inline bool isTranslatorActive() const {return m_board.isTranslatorActive();}
    inline bool isToolDiskActive() const {return m_board.isToolDiskActive();}
    inline bool hasSeveralSides() const {return m_numberOfSides > 1;}
    virtual QString getNextSideLabel();
    inline QString getNextSideFilename() {return m_nextSideFilename;}
    inline void setEditDialog(DiskEditDialog *aDialog) {m_editDialog = aDialog; emit statusChanged(m_deviceNo);}
    inline DiskEditDialog* editDialog() {return m_editDialog;}
    inline Board *getBoardInfo() {return m_board.getCopy();}
    inline void setBoardInfo(Board *info) {m_board.setFromCopy(info);}

    virtual void handleCommand(quint8 command, quint16 aux);
    virtual bool format(const DiskGeometry &geo);
    virtual bool readHappySectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data);
    virtual bool readHappySkewAlignment(bool happy1050);
    virtual void readTrack(quint16 aux, QByteArray &data, int length);
    virtual bool readSectorStatuses(QByteArray &data);
    virtual bool readSectorUsingIndex(quint16 aux, QByteArray &data);
    virtual bool readSector(quint16 aux, QByteArray &data);
    virtual bool readSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly);
    virtual bool resetTrack(quint16 aux);
    virtual bool writeTrack(quint16 aux, const QByteArray &data);
    virtual bool writeTrackWithSkew(quint16 aux, const QByteArray &data);
    virtual bool writeSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy);
    virtual bool writeFuzzySector(quint16 aux, const QByteArray &data);
    virtual bool writeSector(quint16 aux, const QByteArray &data);
    virtual bool writeSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8 sideNumber, quint8 sectorNumber, quint8 sectorSize, const QByteArray &data, bool crcError, int weakOffset);
    virtual void getStatus(QByteArray &status);
    virtual int sectorsInCurrentTrack();
    virtual void setReady(bool bReady);
    virtual void setChipMode(bool enable);
    virtual void setHappyMode(bool enable);
    virtual void setOSBMode(bool enable);
    virtual void setToolDiskMode(bool enable);
    virtual void setDisplayTransmission(bool active);
    virtual void setSpyMode(bool enable);
    virtual void setTrackLayout(bool enable);
    virtual void setDisassembleUploadedCode(bool enable);
    virtual void setTranslatorAutomaticDetection(bool enable);
    virtual void setTranslatorDiskImagePath(const QString &filename);
    virtual void setToolDiskImagePath(const QString &filename);
    virtual void setActivateChipModeWithTool(bool activate);
    virtual void setActivateHappyModeWithTool(bool activate);

    inline DiskGeometry geometry() const {return m_geometry;}
    virtual QString originalFileName() const;
    virtual QString description() const;

    virtual int getUploadCodeStartAddress(quint8 command, quint16 aux, QByteArray &data);
    virtual void disassembleCode(QByteArray &data, unsigned short address, bool drive1050, bool happy);

    int defaultFileSystem();

protected:
    DiskGeometry m_geometry, m_newGeometry;
    QTemporaryFile file;
    bool m_isReadOnly;
    bool m_isLeverOpen;
    bool m_isModified;
    bool m_isUnmodifiable;
    bool m_isUnnamed;
    bool m_isReady;
    QString m_originalFileName;
    QByteArray m_originalFileHeader;
    FileTypes::FileType m_originalImageType;
    bool m_gzipped;
    DiskEditDialog *m_editDialog;
    int m_currentSide;
    int m_numberOfSides;
    QString m_nextSideFilename;
    bool m_displayTransmission;
    bool m_dumpDataFrame;
    bool m_displayTrackLayout;
    bool m_disassembleUploadedCode;
    bool m_translatorAutomaticDetection;
    QString m_translatorDiskImagePath;
    bool m_OSBMode;
    QString m_toolDiskImagePath;
    bool m_toolDiskMode;
    quint16 m_trackNumber;
    qint64 m_lastTime;
    qint64 m_lastDistance;
    quint16 m_lastSector;
    quint8 m_driveStatus;
    quint8 m_wd1771Status;
    int m_sectorsInTrack;
    QElapsedTimer m_timer;
    QByteArray m_diagData;
    bool m_conversionInProgress;
    disassembly810 m_disassembly810;
    disassembly1050 m_disassembly1050;
    int m_remainingAddress;
    QByteArray m_remainingBytes;
    // Pro specific data
    ProSectorInfo m_proSectorInfo[1040 + 256]; // to support an enhanced density PRO file + 256 phantom sectors
    quint16 m_trackContent[200];
	// Atx specific data
    AtxTrackInfo m_atxTrackInfo[40];
    // data for Happy or Archiver
    Board m_board;
    // Tanslator and tool disk
    SimpleDiskImage *m_translatorDisk;
    SimpleDiskImage *m_toolDisk;
    bool m_activateChipModeWithTool;
    bool m_activateHappyModeWithTool;

    bool seekToSector(quint16 sector);
    void refreshNewGeometry();
    bool sameSoftware(const QString &fileName, const QString &otherFileName);

    bool openAtr(const QString &fileName);
    bool openXfd(const QString &fileName);
    bool openPro(const QString &fileName);
    bool openAtx(const QString &fileName);
    bool openDcm(const QString &fileName);
    bool openDi(const QString &fileName);

    bool saveAtr(const QString &fileName);
    bool saveXfd(const QString &fileName);
    bool savePro(const QString &fileName);
    bool saveAtx(const QString &fileName);
    bool saveDcm(const QString &fileName);
    bool saveDi(const QString &fileName);

    bool saveAsAtr(const QString &fileName, FileTypes::FileType destImageType);
    bool saveAsPro(const QString &fileName, FileTypes::FileType destImageType);
    bool saveAsAtx(const QString &fileName, FileTypes::FileType destImageType);

    bool createAtr(int untitledName);
    bool createPro(int untitledName);
    bool createAtx(int untitledName);

    bool formatAtr(const DiskGeometry &geo);
    bool formatPro(const DiskGeometry &geo);
    bool formatAtx(const DiskGeometry &geo);

    bool readHappyAtrSectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data);
    bool readHappyProSectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data);
    bool readHappyAtxSectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data);

    bool readHappyAtrSkewAlignment(bool happy1050);
    bool readHappyProSkewAlignment(bool happy1050);
    bool readHappyAtxSkewAlignment(bool happy1050);

    bool writeHappyAtrTrack(int trackNumber, bool happy1050);
    bool writeHappyProTrack(int trackNumber, bool happy1050);
    bool writeHappyAtxTrack(int trackNumber, bool happy1050);

    bool writeHappyAtrSectors(int trackNumber, int afterSectorNumber, bool happy1050);
    bool writeHappyProSectors(int trackNumber, int afterSectorNumber, bool happy1050);
    bool writeHappyAtxSectors(int trackNumber, int afterSectorNumber, bool happy1050);

    void readAtrTrack(quint16 aux, QByteArray &data, int length);
    void readProTrack(quint16 aux, QByteArray &data, int length);
    void readAtxTrack(quint16 aux, QByteArray &data, int length);

    bool readAtrSectorStatuses(QByteArray &data);
    bool readProSectorStatuses(QByteArray &data);
    bool readAtxSectorStatuses(QByteArray &data);

    bool readAtrSectorUsingIndex(quint16 aux, QByteArray &data);
    bool readProSectorUsingIndex(quint16 aux, QByteArray &data);
    bool readAtxSectorUsingIndex(quint16 aux, QByteArray &data);

    bool readAtrSector(quint16 aux, QByteArray &data);
    bool readProSector(quint16 aux, QByteArray &data);
    bool readAtxSector(quint16 aux, QByteArray &data);

    bool readAtrSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly);
    bool readProSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly);
    bool readAtxSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly);

    bool resetAtrTrack(quint16 aux);
    bool resetProTrack(quint16 aux);
    bool resetAtxTrack(quint16 aux);

    bool writeAtrTrack(quint16 aux, const QByteArray &data);
    bool writeProTrack(quint16 aux, const QByteArray &data);
    bool writeAtxTrack(quint16 aux, const QByteArray &data);

    bool writeAtrTrackWithSkew(quint16 aux, const QByteArray &data);
    bool writeProTrackWithSkew(quint16 aux, const QByteArray &data);
    bool writeAtxTrackWithSkew(quint16 aux, const QByteArray &data);

    bool writeAtrSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy);
    bool writeProSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy);
    bool writeAtxSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy);

    bool writeFuzzyAtrSector(quint16 aux, const QByteArray &data);
    bool writeFuzzyProSector(quint16 aux, const QByteArray &data);
    bool writeFuzzyAtxSector(quint16 aux, const QByteArray &data);

    bool writeAtrSector(quint16 aux, const QByteArray &data);
    bool writeProSector(quint16 aux, const QByteArray &data);
    bool writeAtxSector(quint16 aux, const QByteArray &data);

    bool writeAtrSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8 sideNumber, quint8 sectorNumber, quint8 sectorSize, const QByteArray &data, bool crcError, int weakOffset);
    bool writeProSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8 sideNumber, quint8 sectorNumber, quint8 sectorSize, const QByteArray &data, bool crcError, int weakOffset);
    bool writeAtxSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8 sideNumber, quint8 sectorNumber, quint8 sectorSize, const QByteArray &data, bool crcError, int weakOffset);

    int sectorsInCurrentAtrTrack();
    int sectorsInCurrentProTrack();
    int sectorsInCurrentAtxTrack();

    bool fillProSectorInfo(const QString &fileName, QFile *sourceFile, quint16 slot, quint16 absoluteSector);
    quint16 findPositionInProTrack(int track, int indexInProSector, bool withoutData);
    void guessWeakSectorInPro(int slot);
    bool findMappingInProTrack(int nbSectors, QByteArray &mapping);
    bool findMappingInAtxTrack(int nbSectors, QByteArray &mapping);
    quint8 writeProSectorHeader(quint8 dataSize, quint16 sectorSlot, quint8 postDataCrc, quint8 preIDField, quint8 postIDCrc, quint8 track, quint8 index, quint8 nextSector);
    quint8 writeAtxSectorHeader(quint8 dataSize, QByteArray &sectorData, quint8 postDataCrc, quint8 preIDField, quint8 postIDCrc, quint8 track, quint8 index, quint8 nextSector);
    bool writeCommandAck();
    bool writeDataAck();
    bool writeCommandNak();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    QByteArray readDataFrame(uint size);
    bool writeDataFrame(QByteArray data);
    quint8 computeVersionByte();
    quint16 getBigIndianWord(QByteArray &array, int offset);
    quint16 getLittleIndianWord(QByteArray &array, int offset);
    quint32 getLittleIndianLong(QByteArray &array, int offset);
    void setLittleIndianWord(QByteArray &array, int offset, quint16 value);
    void setLittleIndianLong(QByteArray &array, int offset, quint32 value);
	void fillBuffer(char *line, unsigned char *buf, int len, int ofs, bool dumpAscii);
    void dumpBuffer(unsigned char *buf, int len);
    bool executeArchiverCode(quint16 aux, QByteArray &data);
    void readHappyTrack(int trackNumber, bool happy1050);
    bool writeHappyTrack(int trackNumber, bool happy1050);
    QByteArray readHappySectors(int trackNumber, int afterSectorNumber, bool happy1050);
    bool writeHappySectors(int trackNumber, int afterSectorNumber, bool happy1050);
    int findNearestSpeed(int speed);
    void setTranslatorActive(bool resetTranslatorState);
    bool translatorDiskImageAvailable();
    void closeTranslator();
    void setToolDiskActive();
    bool toolDiskImageAvailable();
    void closeToolDisk();
};

#endif // DISKIMAGE_H
