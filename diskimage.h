#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include <QFile>
#include <QTemporaryFile>

#include "sioworker.h"
#include "miscutils.h"

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
    bool isStandardQD() const;
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

class SimpleDiskImage: public SioDevice
{
    Q_OBJECT

public:
    SimpleDiskImage(SioWorker *worker);
    ~SimpleDiskImage();

    virtual bool open(const QString &fileName, FileTypes::FileType type);
    bool create(int untitledName);
    void reopen();
    virtual void close();
    bool save();
    bool saveAs(const QString &fileName);
    inline bool isOpen() const {return file.isOpen();}
    inline bool isReadOnly() const {return m_isReadOnly;}
    inline void setReadOnly(bool readOnly) {m_isReadOnly = readOnly;}
    inline bool isModified() const {return m_isModified;}
    inline bool isUnmodifiable() const {return m_isUnmodifiable;}
    inline bool isUnnamed() const {return m_isUnnamed;}
    inline void setEditDialog(DiskEditDialog *aDialog) {m_editDialog = aDialog; emit statusChanged(m_deviceNo);}
    inline DiskEditDialog* editDialog() {return m_editDialog;}

    void handleCommand(quint8 command, quint16 aux);
    virtual bool format(const DiskGeometry &geo);
    virtual bool readSector(quint16 sector, QByteArray &data);
    virtual bool writeSector(quint16 sector, const QByteArray &data);
    virtual void getStatus(QByteArray &status);
    
    inline DiskGeometry geometry() const {return m_geometry;}
    inline QString originalFileName() const {return m_originalFileName;}
    virtual QString description() const {return m_geometry.humanReadable();}

    int defaultFileSystem();

protected:
    DiskGeometry m_geometry, m_newGeometry;
    QTemporaryFile file;
    bool m_isReadOnly;
    bool m_isModified;
    bool m_isUnmodifiable;
    bool m_isUnnamed;
    QString m_originalFileName;
    QByteArray m_originalFileHeader;
    FileTypes::FileType m_originalImageType;
    bool m_gzipped;
    DiskEditDialog *m_editDialog;

    bool seekToSector(quint16 sector);
    void refreshNewGeometry();

    bool openAtr(const QString &fileName);
    bool openXfd(const QString &fileName);
    bool openDcm(const QString &fileName);
    bool openScp(const QString &fileName);
    bool openDi(const QString &fileName);

    bool saveAtr(const QString &fileName);
    bool saveXfd(const QString &fileName);
    bool saveDcm(const QString &fileName);
    bool saveScp(const QString &fileName);
    bool saveDi(const QString &fileName);
};

#endif // DISKIMAGE_H
