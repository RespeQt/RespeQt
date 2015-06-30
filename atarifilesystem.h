#ifndef ATARIFILESYSTEM_H
#define ATARIFILESYSTEM_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "diskimage.h"

class AtariDirEntry
{
public:
    AtariDirEntry() {no = -1;}
    enum Attribute {
        Locked = 1,
        Hidden = 2,
        Archived = 4,
        Directory = 8,
        Dos10 = 16,
        Dos25 = 32,
        MyDos = 64
    };
    Q_DECLARE_FLAGS(Attributes, Attribute)

    quint16 firstSector;
    QByteArray atariName;    
    Attributes attributes;
    int no;
    quint16 dir;
    int size;
    QDateTime dateTime;

    QString name() const;
    QString niceName() const;
    QString baseName() const;
    QString suffix() const;
    QString attributeNames() const;

    QByteArray internalData;

    void makeFromAtariDosEntry(const QByteArray &entry, int aNo, int aDir, bool dd = false);
    void makeFromSpartaDosEntry(const QByteArray &entry, int aNo, int aDir);

    inline bool isValid() {return no != -1;}
};

Q_DECLARE_OPERATORS_FOR_FLAGS (AtariDirEntry::Attributes)

bool atariDirEntryNoLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryNoGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryNameLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryNameGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryExtensionLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryExtensionGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntrySizeLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntrySizeGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryDateLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryDateGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryNotesLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2);
bool atariDirEntryNotesGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2);

class AtariFileSystem: public QObject
{
    Q_OBJECT

protected:
    SimpleDiskImage *m_image;
    bool m_textConversion;
    bool m_isConnected;
    QByteArray bitmap;
    quint16 m_freeSectors;
    quint16 findFreeSector(quint16 from = 0);
    void allocateSector(quint16 sector);
    void freeSector(quint16 sector);
    bool sectorIsFree(quint16 sector);
public:
    AtariFileSystem(SimpleDiskImage *image) {m_image = image; m_textConversion = false;}
    virtual QList <AtariDirEntry> getEntries(quint16 dir) = 0;
    virtual uint totalCapacity() = 0;
    virtual uint freeSpace() = 0;
    virtual QString volumeLabel() = 0;
    virtual bool setVolumeLabel(const QString &label) = 0;
    bool extractRecursive(QList <AtariDirEntry> &entries, const QString &target);
    bool deleteRecursive(QList <AtariDirEntry> &entries);
    QList <AtariDirEntry> insertRecursive(quint16 dir, QStringList files);
    QByteArray findName(quint16 dir, QString name);
    virtual int findFreeFileNo(quint16 dir) = 0;
    virtual bool extract(const AtariDirEntry &entry, const QString &target) = 0;
    virtual AtariDirEntry insert(quint16 dir, const QString &name) = 0;
    virtual bool erase(const AtariDirEntry &entry) = 0;
    virtual bool rename(const AtariDirEntry &entry, const QByteArray &name) = 0;
    virtual bool setTime(const AtariDirEntry &entry, const QDateTime &time) = 0;
    virtual bool setReadOnly(const AtariDirEntry &entry) = 0;
    virtual bool setHidden(const AtariDirEntry &entry) = 0;
    virtual bool setArchived(const AtariDirEntry &entry) = 0;
    inline SimpleDiskImage *image() {return m_image;}
    inline bool textConversion() {return m_textConversion;}
    inline void setTextConversion(bool conv) {m_textConversion = conv;}
    virtual QString name() = 0;
    virtual int fileSystemCode() = 0;
    virtual bool removeDir(const AtariDirEntry &entry) = 0;
    virtual AtariDirEntry makeDir(quint16 dir, const QString &name) = 0;
    virtual quint16 rootDir() = 0;
    inline bool isConnected() {return m_isConnected;}
};

class Dos10FileSystem: public AtariFileSystem
{
    Q_OBJECT

protected:
    QByteArray vtoc;
    virtual bool writeBitmap();
public:
    Dos10FileSystem(SimpleDiskImage *image);
    QList <AtariDirEntry> getEntries(quint16 dir);
    uint totalCapacity();
    int findFreeFileNo(quint16 dir);
    uint freeSpace() {return m_freeSectors * (m_image->geometry().bytesPerSector() - 3);}
    QString volumeLabel();
    bool setVolumeLabel(const QString &label);
    bool extract(const AtariDirEntry &entry, const QString &target);
    AtariDirEntry insert(quint16 dir, const QString &name);
    AtariDirEntry makeDir(quint16 dir, const QString &name);
    bool erase(const AtariDirEntry &entry);
    bool rename(const AtariDirEntry &entry, const QByteArray &name);
    bool setTime(const AtariDirEntry &entry, const QDateTime &time);
    bool setReadOnly(const AtariDirEntry &entry);
    bool setHidden(const AtariDirEntry &entry);
    bool setArchived(const AtariDirEntry &entry);
    QString name() {return "Atari Dos 1.0";}
    int fileSystemCode() {return 1;}
    bool removeDir(const AtariDirEntry &entry);
    quint16 rootDir() {return 361;}
};

class Dos20FileSystem: public Dos10FileSystem
{
    Q_OBJECT

public:
    Dos20FileSystem(SimpleDiskImage *image);

    uint totalCapacity();

    QString name() {return "Atari Dos 2.0";}
    int fileSystemCode() {return 2;}
};

class Dos25FileSystem: public Dos20FileSystem
{
    Q_OBJECT

protected:
    QByteArray vtoc2;
    bool writeBitmap();

public:
    Dos25FileSystem(SimpleDiskImage *image);

    uint totalCapacity();

    QString name() {return "Atari Dos 2.5";}
    int fileSystemCode() {return 3;}
};

class MyDosFileSystem: public Dos20FileSystem
{
    Q_OBJECT

protected:
    QByteArray xvtoc;
    bool writeBitmap();

public:
    MyDosFileSystem(SimpleDiskImage *image);

    uint totalCapacity();

    QString name() {return "MyDos";}
    int fileSystemCode() {return 4;}
};

class SpartaDosFileSystem;

class SpartaDosFile
{
protected:
    SpartaDosFileSystem *m_fileSystem;
    QByteArray m_currentSector;
    QByteArray m_currentMap;
    quint16 m_currentMapOffset;
    quint16 m_currentSectorOffset;
    quint16 m_firstMap;
    uint m_size;
    uint m_position;
    bool m_isDir;

    bool toNextSector();
public:
    SpartaDosFile(SpartaDosFileSystem *fileSystem, quint16 firstMap, int size, bool isDir = false);
    bool seek(uint position);
    QByteArray read(uint bytes);
    bool write(const QByteArray &data, uint bytes);
};

class SpartaDosFileSystem: public AtariFileSystem
{
    Q_OBJECT
    friend class SpartaDosFile;
protected:
    quint16 m_rootDirMap;
    quint16 m_sectorCount;
    quint8 m_bitmapCount;
    quint16 m_firstBitmapSector;
    quint16 m_firstDataSector;
    quint16 m_firstDirSector;
    QByteArray m_volumeName;
    quint8 m_fileSystemVersion;
    quint8 m_sequenceNumber;

public:
    SpartaDosFileSystem(SimpleDiskImage *image);
    QList <AtariDirEntry> getEntries(quint16 dir);

    uint totalCapacity();
    uint freeSpace();
    QString volumeLabel();
    bool setVolumeLabel(const QString &label);

    int findFreeFileNo(quint16 dir);

    bool extract(const AtariDirEntry &entry, const QString &target);
    AtariDirEntry insert(quint16 dir, const QString &name);
    AtariDirEntry makeDir(quint16 dir, const QString &name);
    bool erase(const AtariDirEntry &entry);
    bool rename(const AtariDirEntry &entry, const QByteArray &name);

    bool setTime(const AtariDirEntry &entry, const QDateTime &time);

    bool setReadOnly(const AtariDirEntry &entry);

    bool setHidden(const AtariDirEntry &entry);

    bool setArchived(const AtariDirEntry &entry);
    QString name() {return "SpartaDos";}
    int fileSystemCode() {return 5;}
    bool removeDir(const AtariDirEntry &entry);
    quint16 rootDir() {return m_rootDirMap;}
};

#endif // ATARIFILESYSTEM_H
