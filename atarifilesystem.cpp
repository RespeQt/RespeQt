#include "atarifilesystem.h"

#include <QtGlobal>
#include <QDir>
#include <QMessageBox>
#include "diskeditdialog.h"

/* Compare functions */

bool atariDirEntryNoLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.no < e2.no;
}

bool atariDirEntryNoGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.no > e2.no;
}

bool atariDirEntryNameLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.baseName() < e2.baseName();
}

bool atariDirEntryNameGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.baseName() > e2.baseName();
}

bool atariDirEntryExtensionLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.suffix() < e2.suffix();
}

bool atariDirEntryExtensionGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.suffix() > e2.suffix();
}

bool atariDirEntrySizeLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.size < e2.size;
}

bool atariDirEntrySizeGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.size > e2.size;
}

bool atariDirEntryDateLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.dateTime < e2.dateTime;
}

bool atariDirEntryDateGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.dateTime > e2.dateTime;
}

bool atariDirEntryNotesLessThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.attributeNames() < e2.attributeNames();
}

bool atariDirEntryNotesGreaterThan(const AtariDirEntry &e1, const AtariDirEntry &e2)
{
    return e1.attributeNames() > e2.attributeNames();
}

/* AtariDirEntry */

QString AtariDirEntry::name() const
{
    QString s = baseName();
    QString e = suffix();
    if (!e.isEmpty()) {
        s.append(".");
        s.append(e);
    }
    return s;
}

QString AtariDirEntry::niceName() const
{
    QString n = name();
    if (n == n.toUpper()) {
        return n.toLower();
    } else {
        return n;
    }
}

QString AtariDirEntry::baseName() const
{
    return QString::fromLatin1(atariName.left(8).constData()).trimmed();
}

QString AtariDirEntry::suffix() const
{
    return QString::fromLatin1(atariName.right(3).constData()).trimmed();
}

QString AtariDirEntry::attributeNames() const
{
    QString result;
    if (attributes & Locked) {
        result += "Locked";
    }
    if (attributes & Hidden) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "Hidden";
    }
    if (attributes & Archived) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "Archived";
    }
    if (attributes & Directory) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "Directory";
    }
    if (attributes & Dos10) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "Dos 1.0 file";
    }
    if (attributes & Dos25) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "Dos 2.5 file";
    }
    if (attributes & MyDos) {
        if (!result.isEmpty()) {
            result += ", ";
        }
        result += "MyDos file";
    }
    return result;
}

void AtariDirEntry::makeFromAtariDosEntry(const QByteArray &entry, int aNo, int aDir, bool dd)
{
    // Translate the attributes
    attributes = 0;

    internalData = entry;

    quint8 f = (quint8) entry.at(0);

    if (f & 0x10) {
        attributes |= Directory;
    }
    if (f & 0x20) {
        attributes |= Locked;
    }
    if ((f & 0x01) && !(f & 0x40)) {
        attributes |= Dos25;
    }
    if (f & 0x04) {
        attributes |= MyDos;
    }
    if (!(f & 0x02) && !(f & 0x10)) {
        attributes |= Dos10;
    }

    // Translate the name
    atariName = entry.mid(5, 11);

    // Translate the size in blocks
    if (attributes & Directory) {
        size = -1;
    } else {
        if (dd) {
            size = ((quint8) entry.at(1) + (quint8) entry.at(2) * 256) * 253;
        } else {
            size = ((quint8) entry.at(1) + (quint8) entry.at(2) * 256) * 125;
        }
    }

    // Translate the first sector
    firstSector = (quint8) entry.at(3) + (quint8) entry.at(4) * 256;

    // Put an invalid date
    dateTime = QDateTime();

    dir = aDir;
    no = aNo;
}

void AtariDirEntry::makeFromSpartaDosEntry(const QByteArray &entry, int aNo, int aDir)
{
    // Translate the attributes
    attributes = 0;

    internalData = entry;

    quint8 f = (quint8) entry.at(0);

    if (f & 0x01) {
        attributes |= Locked;
    }
    if (f & 0x02) {
        attributes |= Hidden;
    }
    if (f & 0x04) {
        attributes |= Archived;
    }
    if (f & 0x20) {
        attributes |= Directory;
    }

    // Translate the name
    atariName = entry.mid(6, 11);

    // Translate the size
    if (attributes & Directory) {
        size = -1;
    } else {
        size = (quint8)entry.at(3) + (quint8)entry.at(4) * 256 + (quint8)entry.at(5) * 65536;
    }

    // Translate the first sector
    firstSector = (quint8) entry.at(1) + (quint8) entry.at(2) * 256;

    // Translate the date/time
    int year = (quint8)entry.at(19) + 1900;
    if (year < 1980) {
        year += 100;
    }
    QDate date(year, (quint8)entry.at(18), (quint8)entry.at(17));
    QTime time((quint8)entry.at(20), (quint8)entry.at(21), (quint8)entry.at(22));
    dateTime = QDateTime(date, time);

    dir = aDir;
    no = aNo;
}

/* AtariFileSystem */

bool AtariFileSystem::extractRecursive(QList<AtariDirEntry> &entries, const QString &target)
{
    foreach (AtariDirEntry e, entries) {
        if (e.attributes & AtariDirEntry::Directory) {
            QString newDir = target + "/" + e.niceName();
            if (!QDir(newDir).mkdir(newDir)) {
                QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot create directory '%1'.").arg(e.niceName()));
                return false;
            }
            QList <AtariDirEntry> subs = getEntries(e.firstSector);
            bool res = extractRecursive(subs, newDir);
            if (!res) {
                return false;
            }
            continue;
        }
        if (!extract(e, target)) {
            return false;
        }
    }
    return true;
}

bool AtariFileSystem::deleteRecursive(QList<AtariDirEntry> &entries)
{
    foreach (AtariDirEntry e, entries) {
        if (e.attributes & AtariDirEntry::Directory) {
            QList <AtariDirEntry> subs = getEntries(e.firstSector);
            bool res = deleteRecursive(subs);
            if (!res) {
                return false;
            }
            removeDir(e);
            continue;
        }
        if (!erase(e)) {
            return false;
        }
    }
    return true;
}

QList <AtariDirEntry> AtariFileSystem::insertRecursive(quint16 dir, QStringList files)
{
    QList <AtariDirEntry> result;
    foreach (QString name, files) {
        AtariDirEntry entry;
        QFileInfo info(name);
        if (info.isDir()) {
            entry = makeDir(dir, name);
            if (!entry.isValid()) {
                return result;
            }
            QDir subDir(name);
            QStringList subList;
            foreach (QFileInfo i, subDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files)) {
                subList.append(i.absoluteFilePath());
            }

            insertRecursive(entry.firstSector, subList);
        } else {
            entry = insert(dir, name);
            if (!entry.isValid()) {
                return result;
            }
        }
        result.append(entry);
    }
    return result;
}

QByteArray AtariFileSystem::findName(quint16 dir, QString name)
{
    QList <AtariDirEntry> entries = getEntries(dir);
    QFileInfo info(name);
    QString baseName = info.completeBaseName().toUpper();
    QString extension = info.suffix().toUpper();
    QString atariName, pfx;
    QByteArray result;

    baseName.remove(QRegExp("[^A-Z0-9]"));
    baseName = baseName.left(8);
    if (baseName.isEmpty()) {
        baseName = "BADNAME";
    }
    extension.remove(QRegExp("[^A-Z0-9]"));
    extension = extension.left(3);
    while (extension.count() < 3) {
        extension.append(" ");
    }
    pfx = baseName;
    for (int i = 1; i < 99999999; i++) {
        atariName = baseName;
        while (atariName.count() < 8) {
            atariName.append(" ");
        }
        atariName.append(extension);
        result = atariName.toLatin1();
        bool found = false;
        foreach (AtariDirEntry e, entries) {
            if (e.atariName == result) {
                found = true;
                break;
            }
        }
        if (!found) {
            return result;
        }
        QString sfx = QString::number(i + 1);
        baseName = pfx;
        if (baseName.count() + sfx.count() > 8) {
            baseName.resize(8 - sfx.count());
        }
        baseName.append(sfx);
    }
    return QByteArray();
}

quint16 AtariFileSystem::findFreeSector(quint16 from)
{
    quint8 masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    quint16 sector;
    uint sectors = bitmap.count() * 8;

    uint startFrom = from;
    if (from < 4) {
        startFrom = 4;
    }

    for (sector = startFrom; sector < sectors; sector++) {
        if (bitmap.at(sector / 8) & masks[sector % 8]) {
            return sector;
        }
    }

    if (from == 0) {
        return 0;
    } else {
        return findFreeSector(0);
    }
}

void AtariFileSystem::allocateSector(quint16 sector)
{
    quint8 masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    bitmap[sector / 8] = bitmap.at(sector / 8) & ~masks[sector % 8];
    m_freeSectors--;
}

void AtariFileSystem::freeSector(quint16 sector)
{
    quint8 masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    bitmap[sector / 8] = bitmap.at(sector / 8) | masks[sector % 8];
    m_freeSectors++;
}

bool AtariFileSystem::sectorIsFree(quint16 sector)
{
    uint sectors = bitmap.count() * 8;
    if (sector < 4 || sector > sectors) {
        return false;
    }
    quint8 masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    return (bitmap.at(sector / 8) & masks[sector % 8]) != 0;
}

/* Dos10FileSystem */

Dos10FileSystem::Dos10FileSystem(SimpleDiskImage *image)
    : AtariFileSystem(image)
{
    m_image->readSector(360, vtoc);
    bitmap = vtoc.mid(10, 90);
    m_freeSectors = (quint8)vtoc.at(3) + (quint8)vtoc.at(4) * 256;
}

QList <AtariDirEntry> Dos10FileSystem::getEntries(quint16 dir)
{
    QList <AtariDirEntry> list;

    bool dd = m_image->geometry().bytesPerSector() == 256;

    int no = 0;
    for (quint16 s = dir; s < dir + 8; s++) {
        QByteArray data;
        m_image->readSector(s, data);
        for (uint e = 0; e < 8; e++, no++) {
            QByteArray dosEntry = data.mid(e * 16, 16);
            quint8 f = (quint8)dosEntry.at(0);
            if (f == 0) {
                goto bailout;
            }
            if (((f & 65) == 65) || (f & 128)) {
                continue;
            }
            AtariDirEntry a;
            a.makeFromAtariDosEntry(dosEntry, no, dir, dd);
            list.append(a);
        }
    }
bailout:

    return list;
}

uint Dos10FileSystem::totalCapacity()
{
    return 707 * 125;
}

QString Dos10FileSystem::volumeLabel()
{
    return QString();
}

bool Dos10FileSystem::setVolumeLabel(const QString &)
{
    return false;
}

bool Dos10FileSystem::extract(const AtariDirEntry &entry, const QString &target)
{
    QFile file(target + "/" + entry.niceName());
    QFile::OpenMode mode;

    mode = QFile::WriteOnly | QFile::Truncate;

    if (!file.open(mode)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot create file '%1'.").arg(entry.niceName()));
        return false;
    }

    quint16 sector = entry.firstSector;
    for (uint n = entry.size / (m_image->geometry().bytesPerSector() - 3); n > 0 && sector != 0; n--) {
        QByteArray data;
        if (!m_image->readSector(sector, data)) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot read '%1': %2").arg(entry.niceName()).arg(tr("Sector read failed.")));
            return false;
        }
        if (!(entry.attributes & AtariDirEntry::MyDos)) {
            int fileNo = (quint8)data.at(data.count() - 3) >> 2;
            if (fileNo != entry.no) {
                QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot read '%1': %2").arg(entry.niceName()).arg(tr("File number mismatch.")));
                return false;
            }
            sector = ((quint8)data.at(data.count() - 3) & 0x03) * 256 + (quint8)data.at(data.count() - 2);
        } else {
            sector = (quint8)data.at(data.count() - 3)  * 256 + (quint8)data.at(data.count() - 2);
        }
        uint size = (quint8)data.at(data.count() - 1);
        if (!(entry.attributes & AtariDirEntry::Dos10)) {
            data.resize(size);
        } else {
            if (size & 128) {
                data.resize(size - 128);
            } else {
                data.resize(125);
            }
        }
        QByteArray nData;
        nData.clear();
        if (m_textConversion) {
            int j = 0;
            for (int i = 0; i < data.count(); i++) {
            #ifdef Q_OS_WIN
                if (data.at(i) == '\r') {
                    // ignore carriage return
                } else if (data.at(i) == '\n') {
                    nData[j] = '\x9b';
                    j += 1;
                } else if (data.at(i) == '\x9b') {
                    nData[j] = '\r';
                    j += 1;
                    nData[j] = '\n';
                    j += 1;
                } else {
                    nData[j] = data.at(i);
                    j += 1;
                }
            #else
                if (data.at(i) == '\n') {
                    nData[j] = '\x9b';
                    j += 1;
                } else if (data.at(i) == '\x9b') {
                    nData[j] = '\n';
                    j += 1;
                } else {
                    nData[j] = data.at(i);
                    j += 1;
                }
            #endif
            }
        } else {
            nData = data;
        }
        if (file.write(nData) != nData.count()) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot write to '%1': %2").arg(file.fileName()).arg(file.errorString()));
            return false;
        }
    }

    return true;
}

AtariDirEntry Dos10FileSystem::insert(quint16 dir, const QString &name)
{
    AtariDirEntry result;
    QByteArray dosEntry = findName(dir, name);
    if (dosEntry.isEmpty()) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Cannot find a suitable file name.")));
        return result;
    }
    int no = findFreeFileNo(dir);
    if (no < 0) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Directory is full.")));
        return result;
    }

    dosEntry.prepend(QByteArray(5, 0));

    QFile file(name);

    QFile::OpenMode mode;

    if (m_textConversion) {
        mode = QFile::ReadOnly | QFile::Text;
    } else {
        mode = QFile::ReadOnly;
    }

    if (!file.open(mode)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot open '%1': %2").arg(name).arg(file.errorString()));
        return result;
    }

    if (file.size() > freeSpace()) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Disk is full.")));
        return result;
    }

    int dataSize = m_image->geometry().bytesPerSector() - 3;

    int sector, newSector;
    bool hiUsed = false;
    bool myDos = fileSystemCode() == 3 && (vtoc.at(0) > 2);

    int firstSector = findFreeSector(0);
    sector = firstSector;
    if (sector == 0) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Disk is full.")));
        return result;
    }
    allocateSector(sector);

    int sectorCount = 0;

    do {
        sectorCount++;
        if (sector > 719) {
            hiUsed = true;
        }
        int size = dataSize;
        if (file.bytesAvailable() < size) {
            size = file.bytesAvailable();
        }
        QByteArray data = file.read(size);

// Only in binary mode data.count must be = size, so if the file opened in text mode don't display an error
// The message below should normally never display unless there is a system error.
        if (data.count() < size) {
            if(mode == (QFile::ReadOnly | QFile::Text)) {
                size = data.count();
            } else {
                QMessageBox::critical(m_image->editDialog(), tr("File system error"), tr("Number of bytes (%1) read from '%2' is not equal to expected data size of (%3)").arg(data.count()).arg(name).arg(size));
            return result;
            }
        }
        if (!file.atEnd()) {
            newSector = findFreeSector(sector + 1);
            allocateSector(newSector);
        } else {
            newSector = 0;
        }
        if (m_textConversion) {
            for (int i = 0; i < data.count(); i++) {
                if (data.at(i) == '\n') {
                    data[i] = '\x9b';
                } else if (data.at(i) == '\x9b') {
                    data[i] = '\n';
                }
            }
        }
        data.resize(dataSize + 3);
        if (myDos) {
            data[dataSize] = newSector / 256;
            data[dataSize + 1] = newSector % 256;
            data[dataSize + 2] = size;
        } else {
            data[dataSize] = (newSector / 256) | (no * 4);
            data[dataSize + 1] = newSector % 256;
            if (fileSystemCode() != 0) {
                data[dataSize + 2] = size;
            } else {
                if (file.atEnd()) {
                    data[dataSize + 2] = size | 128;
                } else {
                    data[dataSize + 2] = 0;
                }
            }
        }
        if (!m_image->writeSector(sector, data)) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector write failed.")));
            return result;
        }
        sector = newSector;
    } while (!file.atEnd());

    quint16 dirsec = dir + no / 8;
    int start = (no % 8) * 16;
    QByteArray data;
    if (!image()->readSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector read failed.")));
        return result;
    }

    quint8 flag;

    if (myDos) {
        flag = 0x47;
    } else if (fileSystemCode() == 2 && hiUsed) {
        flag = 0x03;
    } else {
        flag = 0x42;
    }

    dosEntry[0] = flag;
    dosEntry[1] = sectorCount % 256;
    dosEntry[2] = sectorCount / 256;
    dosEntry[3] = firstSector % 256;
    dosEntry[4] = firstSector / 256;

    data.replace(start, 16, dosEntry);

    if (!image()->writeSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector write failed.")));
        return result;
    }

    writeBitmap();
    result.makeFromAtariDosEntry(dosEntry, no, dir, m_image->geometry().bytesPerSector() == 256);

    return result;
}

AtariDirEntry Dos10FileSystem::makeDir(quint16 dir, const QString &name)
{
    AtariDirEntry result;
    QByteArray dosEntry = findName(dir, name);
    if (dosEntry.isEmpty()) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Cannot find a suitable file name.")));
        return result;
    }
    int no = findFreeFileNo(dir);
    if (no < 0) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Directory is full.")));
        return result;
    }

    dosEntry.prepend(QByteArray(5, 0));
    dosEntry[0] = 0x10;
    dosEntry[1] = 8;

    int first = findFreeSector(369);
    int sector = first;
    if (sector == 0) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Disk is full.")));
        return result;
    }
    bool found = false;
    do {
        if (sectorIsFree(sector + 1) &&
                sectorIsFree(sector + 2) &&
                sectorIsFree(sector + 3) &&
                sectorIsFree(sector + 4) &&
                sectorIsFree(sector + 5) &&
                sectorIsFree(sector + 6) &&
                sectorIsFree(sector + 7)) {
            found = true;
            break;
        }
        sector = findFreeSector(sector + 1);
    } while (sector != first);

    if (!found) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Disk is full.")));
        return result;
    }

    QByteArray empty(m_image->geometry().bytesPerSector(), 0);

    for (int i = sector; i < sector + 8; i++) {
        allocateSector(i);
        if (!m_image->writeSector(i, empty)) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector write failed.")));
            return result;
        }
    }

    dosEntry[3] = sector % 256;
    dosEntry[4] = sector / 256;

    quint16 dirsec = dir + no / 8;
    int start = (no % 8) * 16;
    QByteArray data;
    if (!image()->readSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector read failed.")));
        return result;
    }

    data.replace(start, 16, dosEntry);

    if (!image()->writeSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot insert '%1': %2").arg(name).arg(tr("Sector write failed.")));
        return result;
    }

    writeBitmap();
    result.makeFromAtariDosEntry(dosEntry, no, dir, m_image->geometry().bytesPerSector() == 256);

    return result;
}

bool Dos10FileSystem::erase(const AtariDirEntry &entry)
{
    quint16 dirsec = entry.dir + entry.no / 8;
    int start = (entry.no % 8) * 16;
    QByteArray data;
    if (!image()->readSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Sector read failed.")));
        return false;
    }
    data[start] = 0x80;
    if (!image()->writeSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Sector write failed.")));
        return false;
    }

    quint16 sector = entry.firstSector;
    for (uint n = entry.size / (m_image->geometry().bytesPerSector() - 3); n > 0 && sector != 0; n--) {
        freeSector(sector);
        QByteArray data;
        if (!m_image->readSector(sector, data)) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Sector read failed.")));
            return false;
        }
        if (!(entry.attributes & AtariDirEntry::MyDos)) {
            int fileNo = (quint8)data.at(data.count() - 3) >> 2;
            if (fileNo != entry.no) {
                QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("File number mismatch.")));
                return false;
            }
            sector = ((quint8)data.at(data.count() - 3) & 0x03) * 256 + (quint8)data.at(data.count() - 2);
        } else {
            sector = (quint8)data.at(data.count() - 3)  * 256 + (quint8)data.at(data.count() - 2);
        }
    }

    if (!writeBitmap()) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Bitmap write failed.")));
        return false;
    }
    return true;
}

bool Dos10FileSystem::rename(const AtariDirEntry &entry, const QByteArray &name)
{
    quint16 dirsec = entry.dir + entry.no / 8;
    int start = (entry.no % 8) * 16;
    QByteArray data;
    if (!image()->readSector(dirsec, data)) {
        return false;
    }
    data.replace(start + 5, 11, name);
    if (!image()->writeSector(dirsec, data)) {
        return false;
    }
    return true;
}

bool Dos10FileSystem::setTime(const AtariDirEntry &, const QDateTime &)
{
    return false;
}

bool Dos10FileSystem::setReadOnly(const AtariDirEntry &)
{
    return false;
}

bool Dos10FileSystem::setHidden(const AtariDirEntry &)
{
    return false;
}

bool Dos10FileSystem:: setArchived(const AtariDirEntry &)
{
    return false;
}

int Dos10FileSystem::findFreeFileNo(quint16 dir)
{
    int no = 0;
    for (int sector = dir; sector < dir + 8; sector++) {
        QByteArray data;
        if (!m_image->readSector(sector, data)) {
            return -1;
        }
        for (int i = 0; i < 128; i += 16) {
            int f = (quint8)data.at(i);
            if (f == 0x80 || f == 0x00) {
                return no;
            }
            no++;
        }
    }
    return -1;
}

bool Dos10FileSystem::writeBitmap()
{
    QByteArray data;
    if (!m_image->readSector(360, data)) {
        return false;
    }
    data.replace(10, 90, bitmap);
    data[3] = m_freeSectors % 256;
    data[4] = m_freeSectors / 256;
    return m_image->writeSector(360, data);
}

bool Dos10FileSystem::removeDir(const AtariDirEntry &entry)
{
    quint16 dirsec = entry.dir + entry.no / 8;
    int start = (entry.no % 8) * 16;
    QByteArray data;
    if (!image()->readSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Sector read failed.")));
        return false;
    }
    data[start] = 0x80;
    if (!image()->writeSector(dirsec, data)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Sector write failed.")));
        return false;
    }

    for (int s = entry.firstSector; s < entry.firstSector + 8; s++) {
        freeSector(s);
    }

    if (!writeBitmap()) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot delete '%1': %2").arg(entry.niceName()).arg(tr("Bitmap write failed.")));
        return false;
    }

    return true;
}

/* Dos20FileSystem */

Dos20FileSystem::Dos20FileSystem(SimpleDiskImage *image)
    : Dos10FileSystem(image)
{
}

uint Dos20FileSystem::totalCapacity()
{
    return ((quint8)vtoc.at(1) + (quint8)vtoc.at(2) * 256) * (m_image->geometry().bytesPerSector() - 3);
}

/* Dos25FileSystem */

Dos25FileSystem::Dos25FileSystem(SimpleDiskImage *image)
    : Dos20FileSystem(image)
{
    m_image->readSector(1024, vtoc2);
    bitmap.append(vtoc2.mid(84, 38));
    m_freeSectors = (quint8)vtoc.at(3) + (quint8)vtoc.at(4) * 256 +
                    (quint8)vtoc2.at(122) + (quint8)vtoc2.at(123) * 256;
}

bool Dos25FileSystem::writeBitmap()
{
    QByteArray data;
    if (!m_image->readSector(360, data)) {
        return false;
    }
    data.replace(10, 90, bitmap.left(90));

    quint8 masks[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    quint16 sectors = 0;

    for (quint16 sector = 0; sector < 720; sector++) {
        if (bitmap.at(sector / 8) & masks[sector % 8]) {
            sectors++;
        }
    }
    data[3] = sectors % 256;
    data[4] = sectors / 256;
    if (!m_image->writeSector(360, data)) {
        return false;
    }

    if (!m_image->readSector(1024, data)) {
        return false;
    }
    data.replace(0, 122, bitmap.right(122));
    data[122] = (m_freeSectors - sectors) % 256;
    data[123] = (m_freeSectors - sectors) / 256;
    return m_image->writeSector(1024, data);
}

uint Dos25FileSystem::totalCapacity()
{
    return ((quint8)vtoc.at(1) + (quint8)vtoc.at(2) * 256) * (m_image->geometry().bytesPerSector() - 3);
}

/* MyDosFileSystem */

MyDosFileSystem::MyDosFileSystem(SimpleDiskImage *image)
    : Dos20FileSystem(image)
{
    int xvtocCount;
    if (m_image->geometry().bytesPerSector() == 256) {
        xvtocCount = (quint8)vtoc.at(0) - 2;
    } else {
        xvtocCount = ((quint8)vtoc.at(0) * 2) - 4;
    }
    if (xvtocCount < 0) {
        xvtocCount = 0;
    }
    xvtoc = QByteArray();
    for (quint16 s = 359; xvtocCount > 0; xvtocCount--, s--) {
        QByteArray data;
        m_image->readSector(s, data);
        xvtoc.append(data);
    }
    bitmap.append(vtoc.right(m_image->geometry().bytesPerSector() - 100));
    bitmap.append(xvtoc);
    bitmap.resize((image->geometry().sectorCount() + 8) / 8);
}

bool MyDosFileSystem::writeBitmap()
{
    QByteArray data;
    if (!m_image->readSector(360, data)) {
        return false;
    }

    int bps = m_image->geometry().bytesPerSector();

    int total = bitmap.count();
    if (total > bps - 10) {
        total = bps - 10;
    }

    data.replace(10, total, bitmap.left(total));

    data[3] = m_freeSectors % 256;
    data[4] = m_freeSectors / 256;
    if (!m_image->writeSector(360, data)) {
        return false;
    }

    if (bitmap.count() == total) {
        return true;
    }

    int xvtocCount = (bitmap.count() - total + bps - 1) / bps;

    for (quint16 s = 359; xvtocCount > 0; xvtocCount--, s--, total += bps) {
        int n = bitmap.count() - total;
        if (n > bps) {
            n = bps;
        }
        QByteArray data = bitmap.mid(total, n);
        data.append(QByteArray(bps - n, 0));
        if (!m_image->writeSector(s, data)) {
            return false;
        }
    }
    return true;
}

uint MyDosFileSystem::totalCapacity()
{
    return ((quint8)vtoc.at(1) + (quint8)vtoc.at(2) * 256) * (m_image->geometry().bytesPerSector() - 3);
}

/* SpartaDosFile */

SpartaDosFile::SpartaDosFile(SpartaDosFileSystem *fileSystem, quint16 firstMap, int size, bool )
{
    m_fileSystem = fileSystem;

    m_currentMapOffset = 4;
    m_fileSystem->m_image->readSector(firstMap, m_currentMap);

    m_currentSector = QByteArray();
    m_currentSectorOffset = 0;

    m_firstMap = firstMap;
    m_size = size;
    m_position = 0;
}

bool SpartaDosFile::toNextSector()
{
    /* Decide where to start searching new sectors */
    int from;
    if (m_isDir) {
        from = m_fileSystem->m_firstDirSector;
    } else {
        from = m_fileSystem->m_firstDataSector;
    }
    /* Read next map sector if current one is done */
    if (m_currentMapOffset > m_fileSystem->m_image->geometry().bytesPerSector()) {
        m_currentMapOffset = 4;
        int nextMapSector = 0;
        if (!m_currentMap.isEmpty()) {
            nextMapSector = (quint8)m_currentMap.at(0) + (quint8)m_currentMap.at(1) * 256;
        }
        /* Allocate a new sector map if there isn't a new one */
        if (nextMapSector == 0) {
            nextMapSector = m_fileSystem->findFreeSector(from);
            if (nextMapSector == 0) {
                return false;
            }
            m_fileSystem->allocateSector(nextMapSector);
        }
        m_fileSystem->m_image->readSector(nextMapSector, m_currentMap);
    }
    int newSector = (quint8)m_currentMap.at(m_currentMapOffset) + (quint8)m_currentMap.at(m_currentMapOffset + 1) * 256;

    if (newSector == 0) {
        newSector = m_fileSystem->findFreeSector(from);
        if (newSector == 0) {
            return false;
        }
        m_fileSystem->allocateSector(newSector);
    }

    m_fileSystem->m_image->readSector(newSector, m_currentSector);
    m_currentMapOffset += 2;
    return true;
}

bool SpartaDosFile::seek(uint position)
{
    int sectorSize = m_fileSystem->image()->geometry().bytesPerSector();
    int sector = position / sectorSize;
    int mapNo = sector / (sectorSize/2 - 2);
    m_fileSystem->m_image->readSector(m_firstMap, m_currentMap);
    int n = 0;
    while (n < mapNo) {
        int map = (quint8)m_currentMap.at(0) + (quint8)m_currentMap.at(1) * 256;
        m_fileSystem->m_image->readSector(map, m_currentMap);
        n++;
    }
    m_currentMapOffset = (sector - mapNo * (sectorSize/2 - 2))*2 + 4;
    m_currentSectorOffset = position % sectorSize;
    int newSector = (quint8)m_currentMap.at(m_currentMapOffset) + (quint8)m_currentMap.at(m_currentMapOffset + 1) * 256;
    m_fileSystem->m_image->readSector(newSector, m_currentSector);
    return true;
}

QByteArray SpartaDosFile::read(uint bytes)
{
    QByteArray result;
    while (bytes) {
        uint left = m_currentSector.count() - m_currentSectorOffset;
        if (bytes > left) {
            result.append(m_currentSector.right(left));
            bytes -= left;
            if (m_currentMapOffset >= m_currentMap.count()) {
                int nextMap = (quint8)m_currentMap.at(0) + (quint8)m_currentMap.at(1) * 256;
                if (nextMap == 0) {
                    m_currentMap.clear();
                    return result;
                }
                m_fileSystem->m_image->readSector(nextMap, m_currentMap);
                m_currentMapOffset = 4;
            }
            int sector = (quint8)m_currentMap.at(m_currentMapOffset) + (quint8)m_currentMap.at(m_currentMapOffset + 1) * 256;
            m_currentMapOffset += 2;
            if (sector == 0) {
                m_currentMap.clear();
                return result;
            }
            m_fileSystem->m_image->readSector(sector, m_currentSector);
            m_currentSectorOffset = 0;
        } else {
            result.append(m_currentSector.mid(m_currentSectorOffset, bytes));
            m_currentSectorOffset += bytes;
            bytes = 0;
        }
    }
    return result;
}

bool SpartaDosFile::write(const QByteArray &, uint /*bytes*/)
{
    return false;
}

/* SpartaDosFileSystem */

SpartaDosFileSystem::SpartaDosFileSystem(SimpleDiskImage *image)
        :AtariFileSystem(image)
{
    QByteArray boot;
    m_image->readSector(1, boot);
    m_rootDirMap = (quint8)boot.at(9) + (quint8)boot.at(10) * 256;
    m_sectorCount = (quint8)boot.at(11) + (quint8)boot.at(12) * 256;
    m_freeSectors = (quint8)boot.at(13) + (quint8)boot.at(14) * 256;
    m_bitmapCount = (quint8)boot.at(15);
    m_firstBitmapSector = (quint8)boot.at(16) + (quint8)boot.at(17) * 256;
    m_firstDataSector = (quint8)boot.at(18) + (quint8)boot.at(19) * 256;
    m_firstDirSector = (quint8)boot.at(20) + (quint8)boot.at(21) * 256;
    m_volumeName = boot.mid(22, 8);
    m_fileSystemVersion = (quint8)boot.at(32);
    m_sequenceNumber = (quint8)boot.at(38);

    QByteArray map;
    for (int i = m_firstBitmapSector; i < m_firstBitmapSector + m_bitmapCount; i++) {
        m_image->readSector(i, map);
        bitmap.append(map);
    }
}

QList <AtariDirEntry> SpartaDosFileSystem::getEntries(quint16 dir)
{
    QList <AtariDirEntry> list;

    SpartaDosFile sf(this, dir, -1);

    QByteArray firstMap;
    m_image->readSector(dir, firstMap);

    QByteArray dosEntry = sf.read(23);

    int dirLen = (quint8)dosEntry.at(3) + (quint8)dosEntry.at(4) * 256 + (quint8)dosEntry.at(5) * 65536 - 23;
    int no = 0;

    while (dirLen > 0) {
        AtariDirEntry entry;
        dosEntry = sf.read(23);
        int f = (quint8)dosEntry.at(0);
        if (f == 0) {
            break;
        }
        if ((f & 144) == 0) {
            entry.makeFromSpartaDosEntry(dosEntry, no, dir);
            list.append(entry);
        }
        dirLen -= 23;
        no++;
    }

    return list;
}

uint SpartaDosFileSystem::totalCapacity()
{
    return 0;
}

uint SpartaDosFileSystem::freeSpace()
{
    return 0;
}

QString SpartaDosFileSystem::volumeLabel()
{
    return QString();
}

bool SpartaDosFileSystem::setVolumeLabel(const QString &/*label*/)
{
    return false;
}

bool SpartaDosFileSystem::extract(const AtariDirEntry &entry, const QString &target)
{
    QFile file(target + "/" + entry.niceName());


    QFile::OpenMode mode;

    if (m_textConversion) {
        mode = QFile::WriteOnly | QFile::Truncate | QFile::Text;
    } else {
        mode = QFile::WriteOnly | QFile::Truncate;
    }

    if (!file.open(mode)) {
        QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot create file '%1'.").arg(entry.niceName()));
        return false;
    }

    SpartaDosFile sdf(this, entry.firstSector, entry.size);

    int rest = entry.size;

    QByteArray buffer;
    while (rest) {
        int bufSize = 8388608;
        if (bufSize > rest) {
            bufSize = rest;
        }
        buffer = sdf.read(bufSize);
        if (buffer.count() != bufSize) {
            bufSize = buffer.count();
            rest = bufSize;
        }
        if (m_textConversion) {
            for (int i = 0; i < buffer.count(); i++) {
                if (buffer.at(i) == '\n') {
                    buffer[i] = '\x9b';
                } else if (buffer.at(i) == '\x9b') {
                    buffer[i] = '\n';
                }
            }
        }
        if (file.write(buffer) != bufSize) {
            QMessageBox::critical(m_image->editDialog(), tr("Atari file system error"), tr("Cannot write to '%1'.").arg(entry.niceName()));
            return false;
        }
        rest -= bufSize;
    }

    return true;
}

AtariDirEntry SpartaDosFileSystem::insert(quint16 /*dir*/, const QString &/*name*/)
{
    return AtariDirEntry();
}

AtariDirEntry SpartaDosFileSystem::makeDir(quint16 /*dir*/, const QString &/*name*/)
{
    return AtariDirEntry();
}

bool SpartaDosFileSystem::erase(const AtariDirEntry &/*entry*/)
{
    return false;
}

bool SpartaDosFileSystem::rename(const AtariDirEntry &/*entry*/, const QByteArray &/*name*/)
{
    return false;
}

bool SpartaDosFileSystem::setTime(const AtariDirEntry &/*entry*/, const QDateTime &/*time*/)
{
    return false;
}

bool SpartaDosFileSystem::setReadOnly(const AtariDirEntry &/*entry*/)
{
    return false;
}

bool SpartaDosFileSystem::setHidden(const AtariDirEntry &/*entry*/)
{
    return false;
}

bool SpartaDosFileSystem::setArchived(const AtariDirEntry &/*entry*/)
{
    return false;
}

int SpartaDosFileSystem::findFreeFileNo(quint16 /*dir*/)
{
    return -1;
}

bool SpartaDosFileSystem::removeDir(const AtariDirEntry &/*entry*/)
{
    return false;
}
