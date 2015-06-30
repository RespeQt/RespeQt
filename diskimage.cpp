#include "diskimage.h"
#include "zlib.h"

#include <QFileInfo>
#include "aspeqtsettings.h"
#include <QDir>
#include "atarifilesystem.h"
#include "diskeditdialog.h"

#include <QtDebug>

/* DiskGeometry */

DiskGeometry::DiskGeometry()
    :QObject()
{
    m_isDoubleSided = false;
    m_tracksPerSide = 0;
    m_sectorsPerTrack = 0;
    m_bytesPerSector = 0;
    m_totalSize = 0;
    m_sectorCount = 0;
}

DiskGeometry::DiskGeometry(const DiskGeometry &other)
    :QObject()
{
    initialize(other);
}

void DiskGeometry::initialize(const DiskGeometry &other)
{
    m_isDoubleSided = other.isDoubleSided();
    m_tracksPerSide = other.tracksPerSide();
    m_sectorsPerTrack = other.sectorsPerTrack();
    m_bytesPerSector = other.bytesPerSector();
    m_sectorCount = other.sectorCount();
    m_totalSize = other.totalSize();
}

void DiskGeometry::initialize(bool aIsDoubleSided, quint8 aTracksPerSide, quint16 aSectorsPerTrack, quint16 aBytesPerSector)
{
    m_isDoubleSided = aIsDoubleSided;
    m_tracksPerSide = aTracksPerSide;
    m_sectorsPerTrack = aSectorsPerTrack;
    m_bytesPerSector = aBytesPerSector;
    m_sectorCount = (m_isDoubleSided + 1) * m_tracksPerSide * m_sectorsPerTrack;
    if (m_bytesPerSector == 256) {
        m_totalSize = m_sectorCount * 128;
        if (m_totalSize > 384) {
            m_totalSize += (m_bytesPerSector - 128) * (m_sectorCount - 3);
        }
    } else {
        m_totalSize = m_sectorCount * m_bytesPerSector;
    }
}

void DiskGeometry::initialize(uint aTotalSize, quint16 aBytesPerSector)
{
    bool ds;
    quint8 tps;
    quint16 spt;

    if (aTotalSize == 92160 && aBytesPerSector == 128) {
        ds = false;
        tps = 40;
        spt = 18;
    } else if (aTotalSize == 133120 && aBytesPerSector == 128) {
        ds = false;
        tps = 40;
        spt = 26;
    } else if (aTotalSize == 183936 && aBytesPerSector == 256) {
        ds = false;
        tps = 40;
        spt = 18;
    } else if (aTotalSize == 368256 && aBytesPerSector == 256) {
        ds = true;
        tps = 40;
        spt = 18;
    } else {
        if (aBytesPerSector == 256) {
            if (aTotalSize <= 384) {
                spt = (aTotalSize + 127) / 128;
            } else {
                spt = (aTotalSize + 384 + 255) / 256;
            }
        } else {
            spt = (aTotalSize + aBytesPerSector - 1) / aBytesPerSector;
        }
        ds = false;
        tps = 1;
    }

    initialize(ds, tps, spt, aBytesPerSector);
}

void DiskGeometry::initialize(uint aTotalSize)
{
    bool ds;
    quint8 tps;
    quint16 spt;
    quint16 bps;

    if (aTotalSize == 92160) {
        ds = false;
        tps = 40;
        spt = 18;
        bps = 128;
    } else if (aTotalSize == 133120) {
        ds = false;
        tps = 40;
        spt = 26;
        bps = 128;
    } else if (aTotalSize == 183936) {
        ds = false;
        tps = 40;
        spt = 18;
        bps = 256;
    } else if (aTotalSize == 368256) {
        ds = true;
        tps = 40;
        spt = 18;
        bps = 256;
    } else {
        if ((aTotalSize - 384) % 256 == 0) {
            spt = (aTotalSize - 384) / 256;
            bps = 256;
        } else {
            spt = (aTotalSize + 127) / 128;
            bps = 128;
        }
        ds = false;
        tps = 1;
    }

    initialize(ds, tps, spt, bps);
}

void DiskGeometry::initialize(const QByteArray &percom)
{
    quint8 aTracksPerSide = (quint8)percom.at(0);
    quint16 aSectorsPerTrack = (quint8)percom.at(2) * 256 + (quint8)percom.at(3);
    bool aIsDoubleSided = (quint8)percom.at(4);
    quint16 aBytesPerSector = (quint8)percom.at(6) * 256 + (quint8)percom.at(7);
    initialize(aIsDoubleSided, aTracksPerSide, aSectorsPerTrack, aBytesPerSector);
}

bool DiskGeometry::isEqual(const DiskGeometry &other)
{
    return
            m_isDoubleSided == other.isDoubleSided() &&
            m_tracksPerSide == other.tracksPerSide() &&
            m_sectorsPerTrack == other.sectorsPerTrack() &&
            m_bytesPerSector == other.bytesPerSector();
}

bool DiskGeometry::isStandardSD() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 128;
}

bool DiskGeometry::isStandardED() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 26 && m_bytesPerSector == 128;
}

bool DiskGeometry::isStandardDD() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 256;
}

bool DiskGeometry::isStandardQD() const
{
    return m_isDoubleSided && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 256;
}

quint16 DiskGeometry::bytesPerSector(quint16 sector)
{
    quint16 result = m_bytesPerSector;
    if (result == 256 && sector <= 3) {
        result = 128;
    }
    return result;
}

QByteArray DiskGeometry::toPercomBlock()
{
    DiskGeometry temp;
    QByteArray percom(12, 0);
    percom[0] = m_tracksPerSide;
    percom[1] = 1; // Step rate
    percom[2] = m_sectorsPerTrack / 256;
    percom[3] = m_sectorsPerTrack % 256;
    percom[4] = m_isDoubleSided;
    percom[5] = (m_bytesPerSector != 128) * 4 | (m_tracksPerSide == 77) * 2;
    percom[6] = m_bytesPerSector / 256;
    percom[7] = m_bytesPerSector % 256;
    percom[8] = 0xff;
    temp.initialize(percom);
    return percom;
}

QString DiskGeometry::humanReadable() const
{
    QString result, density, sides;

    if (isStandardSD()) {
        result = tr("SD Diskette");
    } else if (isStandardED()) {
        result = tr("ED Diskette");
    } else if (isStandardDD()) {
        result = tr("DD Diskette");
    } else if (isStandardQD()) {
        result = tr("QD Diskette");
    } else if (m_tracksPerSide == 1) {
        if (m_bytesPerSector == 128) {
            result = tr("%1 sector SD HardDrive").arg(m_sectorCount);
        } else if (m_bytesPerSector == 256) {
            result = tr("%1 sector DD HardDrive").arg(m_sectorCount);
        } else {
            result = tr("%1 sector, %2 bytes/sector HardDrive").arg(m_sectorCount).arg(m_bytesPerSector);
        }
    } else {
        result = tr("%1 %2 tracks/side, %3 sectors/track, %4 bytes/sector diskette")
                 .arg(m_isDoubleSided?tr("DS"):tr("SS"))
                 .arg(m_tracksPerSide)
                 .arg(m_sectorsPerTrack)
                 .arg(m_bytesPerSector);
    }

    return tr("%1 (%2k)").arg(result).arg((m_totalSize + 512) / 1024);
}

/* SimpleDiskImage */

SimpleDiskImage::SimpleDiskImage(SioWorker *worker)
    : SioDevice(worker)
{
    m_editDialog = 0;
}

SimpleDiskImage::~SimpleDiskImage()
{
    if (isOpen()) {
        close();
    }
}

bool SimpleDiskImage::openAtr(const QString &fileName)
{
    QFile *sourceFile;

    if (m_originalImageType == FileTypes::Atr) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    bool repaired = false;

    // Try to open the source file
    if (!sourceFile->open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(sourceFile->errorString());
        delete sourceFile;
        return false;
    }

    // Try to read the header
    QByteArray header;
    header = sourceFile->read(16);
    if (header.size() != 16) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        delete sourceFile;
        return false;
    }

    // Validate the magic number
    quint16 magic = ((quint8)header[0]) + ((quint8)header[1]) * 256;
    if (magic != 0x0296) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Not a valid ATR file."));
        delete sourceFile;
        return false;
    }

    // Decode image metadata
    quint16 sizeLo = ((quint8)header[2]) + ((quint8)header[3]) * 256;
    quint16 secSize = ((quint8)header[4]) + ((quint8)header[5]) * 256;
    quint32 sizeHi = ((quint8)header[6]) + ((quint8)header[7]) * 256;
    quint64 size = (sizeLo + sizeHi * 65536) * 16;

    // Try to create the temporary file
    file.setFileTemplate(QDir::temp().absoluteFilePath("aspeqt-temp-XXXXXX"));
    if (!file.open()) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot create temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        delete sourceFile;
        return false;
    }

    // Copy the source file to temporary file
    while (!sourceFile->atEnd()) {
        int bufsize = 16777216;
        QByteArray buffer = sourceFile->read(bufsize);
        if (buffer.length() != bufsize && !sourceFile->atEnd()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot read from file: %1.").arg(sourceFile->errorString()));
            delete sourceFile;
            file.close();
            return false;
        }
        if (file.write(buffer) != buffer.length()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot write to temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
            delete sourceFile;
            file.close();
            return false;
        }
    }

    quint64 imageSize = file.size();

    // Check if the reported image size is consistent with the actual size
    // 
    if (size != imageSize) {
        qWarning() << "!w" << tr("Image size of '%1' is reported as %2 bytes in the header but it's actually %3.")
                       .arg(fileName)
                       .arg(size)
                       .arg(imageSize);
        size = imageSize;
        repaired = true;
    }

    // Validate sector size
    if (secSize != 128 && secSize != 256 && secSize != 512 && secSize != 8192) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Unknown sector size (%1).").arg(secSize));
        delete sourceFile;
        file.close();
        return false;
    }

    // Some .atr file sizes are larger than the reported size
    // in the header. This may be due to extraneous bytes added to the end
    // of the file by various file conversion utilities
    // Those files usually still work fine when loaded to a drive.
    // So only reject a file if the actual size of the file is < size specified
    // in .atr file header --- 

    // Validate the image size
    bool sizeValid = true;
    if (secSize == 256) {
        // Handle double density images
        if (size <= 384) {
            // Handle double density images with less than 4 sectors
            if (size % 128 != 0) {
                sizeValid = false;
            }
        } else {
            // Handle double density images with 4 or more sectors
            if ((size + 384) % 256 != 0) {
                // 
                if (size / 256 < 720 ) {
                    sizeValid = false;
                }
            }
        }
    } else {
        // Handle non-double density images
        if (size % secSize != 0) {
            // Single Density                                      
            if ((size < 133120) && (size / secSize < 720 )) {
                sizeValid = false;
            } else {
              // Enhanced Density                                  
                if ((size >= 133120) && (size / secSize < 1040 )) {
                    sizeValid = false;
                }
            }
        }
    }


    if (!sizeValid) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Invalid image size (%1).").arg(size));
        file.close();
        delete sourceFile;
        return false;
    }

    // Validate disk geometry
    DiskGeometry geometry;
    geometry.initialize(size, secSize);
    if (geometry.sectorCount() > 65535) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Too many sectors in the image (%1).").arg(geometry.sectorCount()));
        file.close();
        delete sourceFile;
        return false;
    }

    // Check unsupported ATR extensions
    if (header[8] || header[9] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15]) {
        qWarning() << "!w" << tr("The file '%1' has some unrecognized fields in its header.")
                      .arg(fileName);
    }

    if (!file.resize(size)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot resize temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        file.close();
        delete sourceFile;
        return false;
    }

    // Update the image information
    m_geometry.initialize(geometry);
    refreshNewGeometry();
    m_isReadOnly = sourceFile->isWritable();
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = repaired;
    m_isUnmodifiable = false;
    m_isUnnamed = false;

    delete sourceFile;
    return true;
}

bool SimpleDiskImage::openXfd(const QString &fileName)
{
    QFile *sourceFile;

    if (m_originalImageType == FileTypes::Xfd) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    if (!sourceFile->open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(sourceFile->errorString());
        delete sourceFile;
        return false;
    }

    file.setFileTemplate(QDir::temp().absoluteFilePath("aspeqt-temp-XXXXXX"));
    if (!file.open()) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot create temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        delete sourceFile;
        return false;
    }

    while (!sourceFile->atEnd()) {
        int bufsize = 16777216;
        QByteArray buffer = sourceFile->read(bufsize);
        if (buffer.length() != bufsize && !sourceFile->atEnd()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot read from file: %1.").arg(sourceFile->errorString()));
            delete sourceFile;
            return false;
        }
        if (file.write(buffer) != buffer.length()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot write to temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
            delete sourceFile;
            return false;
        }
    }

    quint64 size = file.size();

    if ((size % 128) != 0) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Invalid image size (%1).").arg(size));
        delete sourceFile;
        file.close();
        return false;
    }

    m_geometry.initialize(size);
    refreshNewGeometry();
    m_originalFileHeader = QByteArray(16, 0);
    m_isReadOnly = sourceFile->isWritable();
    m_originalFileName = fileName;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = false;

    delete sourceFile;
    return true;
}

bool SimpleDiskImage::openDcm(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("DCM images are not supported yet."));
    return false;
}

bool SimpleDiskImage::openScp(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("SCP images are not supported yet."));
    return false;
}

bool SimpleDiskImage::openDi(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("DI images are not supported yet."));
    return false;
}

bool SimpleDiskImage::saveAtr(const QString &fileName)
{
    if (m_originalFileHeader.size() != 16) {
        m_originalFileHeader = QByteArray(16, 0);
    }

    // Put signature
    m_originalFileHeader[0] = 0x96;
    m_originalFileHeader[1] = 0x02;

    // Encode meta data
    m_originalFileHeader[2] = (m_geometry.totalSize() >> 4) % 256;
    m_originalFileHeader[3] = (m_geometry.totalSize() >> 12) % 256;
    m_originalFileHeader[4] = m_geometry.bytesPerSector() % 256;
    m_originalFileHeader[5] = m_geometry.bytesPerSector() / 256;
    m_originalFileHeader[6] = (m_geometry.totalSize() >> 20) % 256;
    m_originalFileHeader[7] = (m_geometry.totalSize() >> 28) % 256;

    // Try to open the output file
    QFile *outputFile;

    if (m_originalImageType == FileTypes::Atr) {
        outputFile = new QFile(fileName);
    } else {
        outputFile = new GzFile(fileName);
    }

    if (!outputFile->open(QFile::WriteOnly | QFile::Truncate)) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile->errorString());
        delete outputFile;
        return false;
    }

    // Try to write the header
    if (outputFile->write(m_originalFileHeader) != 16) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile->errorString());
        delete outputFile;
        return false;
    }

    // Try to copy the temporary file back
    if (!file.reset()) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot rewind temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        delete outputFile;
        return false;
    }

    while (file.bytesAvailable() != 0) {
        int bufsize = 16777216;
        if (bufsize > file.bytesAvailable()) {
            bufsize = file.bytesAvailable();
        }
        QByteArray buffer = file.read(bufsize);
        if (buffer.length() != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot read from temporay file %1: %2").arg(file.fileName()).arg(file.errorString()));
            delete outputFile;
            return false;
        }
        if (outputFile->write(buffer) != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            delete outputFile;
            return false;
        }
    }

    DiskGeometry guess;
    guess.initialize(m_geometry.totalSize(), m_geometry.bytesPerSector());

    if (!guess.isEqual(m_geometry)) {
        qWarning() << "!w" << tr("Detailed geometry information will be lost when reopening '%1' due to ATR file format limitations.").arg(fileName);
    }

    m_originalFileName = fileName;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = false;
    emit statusChanged(m_deviceNo);

    delete outputFile;
    return true;
}

bool SimpleDiskImage::saveXfd(const QString &fileName)
{
    DiskGeometry guess;
    guess.initialize(m_geometry.totalSize());

    if (!guess.isEqual(m_geometry)) {
        if (guess.bytesPerSector() == m_geometry.bytesPerSector()) {
            qWarning() << "!w" << tr("Detailed disk geometry information will be lost when reopening '%1' due to XFD file format limitations.")
                          .arg(fileName);
        } else {
            qCritical() << "!e" << tr("XFD file format cannot handle this disk geometry. Try saving '%1' as ATR.").arg(fileName);
            return false;
        }
    }

    // Try to open the output file
    QFile outputFile(fileName);
    if (!outputFile.open(QFile::WriteOnly | QFile::Truncate)) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile.errorString());
        return false;
    }

    // Try to copy the temporary file back
    if (!file.reset()) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot rewind temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        return false;
    }

    while (file.bytesAvailable() != 0) {
        int bufsize = 16777216;
        if (bufsize > file.bytesAvailable()) {
            bufsize = file.bytesAvailable();
        }
        QByteArray buffer = file.read(bufsize);
        if (buffer.length() != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot read from temporay file %1: %2").arg(file.fileName()).arg(file.errorString()));
            return false;
        }
        if (outputFile.write(buffer) != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile.errorString());
            return false;
        }
    }

    m_originalFileName = fileName;
    m_originalImageType = FileTypes::Xfd;
    m_originalFileHeader.clear();
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = false;
    emit statusChanged(m_deviceNo);

    return true;
}

bool SimpleDiskImage::saveDcm(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving DCM images is not supported yet."));
    return false;
}

bool SimpleDiskImage::saveScp(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving SCP images is not supported yet."));
    return false;
}

bool SimpleDiskImage::saveDi(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving DI images is not supported yet."));
    return false;
}

bool SimpleDiskImage::saveAs(const QString &fileName)
{
    if (fileName.endsWith(".ATR", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Atr;
        return saveAtr(fileName);
    } else if (fileName.endsWith(".XFD", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Xfd;
        return saveXfd(fileName);
    } else if (fileName.endsWith(".DCM", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Dcm;
        return saveDcm(fileName);
    } else if (fileName.endsWith(".SCP", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Scp;
        return saveScp(fileName);
    } else if (fileName.endsWith(".DI", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Di;
        return saveDi(fileName);
    } else if (fileName.endsWith(".ATZ", Qt::CaseInsensitive) || fileName.endsWith(".ATR.GZ", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::AtrGz;
        return saveAtr(fileName);
    } else if (fileName.endsWith(".XFZ", Qt::CaseInsensitive) || fileName.endsWith(".XFD.GZ", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::XfdGz;
        return saveXfd(fileName);
    } else {
        qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Unknown file extension."));
        return false;
    }
}

bool SimpleDiskImage::save()
{
    switch (m_originalImageType) {
        case FileTypes::Atr:
        case FileTypes::AtrGz:
            return saveAtr(m_originalFileName);
            break;
        case FileTypes::Xfd:
        case FileTypes::XfdGz:
            return saveXfd(m_originalFileName);
            break;
        case FileTypes::Dcm:
        case FileTypes::DcmGz:
            return saveDcm(m_originalFileName);
            break;
        case FileTypes::Scp:
        case FileTypes::ScpGz:
            return saveScp(m_originalFileName);
            break;
        case FileTypes::Di:
        case FileTypes::DiGz:
            return saveDi(m_originalFileName);
            break;
        default:
            qCritical() << "!e" << tr("Cannot save '%1': %2").arg(m_originalFileName).arg(tr("Unknown file type."));
            return false;
    }
}

bool SimpleDiskImage::open(const QString &fileName, FileTypes::FileType type)
{
    m_originalImageType = type;

    switch (type) {
        case FileTypes::Atr:
        case FileTypes::AtrGz:
            return openAtr(fileName);
            break;
        case FileTypes::Xfd:
        case FileTypes::XfdGz:
            return openXfd(fileName);
            break;
        case FileTypes::Dcm:
        case FileTypes::DcmGz:
            return openDcm(fileName);
            break;
        case FileTypes::Scp:
        case FileTypes::ScpGz:
            return openScp(fileName);
            break;
        case FileTypes::Di:
        case FileTypes::DiGz:
            return openDi(fileName);
            break;
        default:
            qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Unknown file type."));
            return false;
    }
}

bool SimpleDiskImage::create(int untitledName)
{
    file.setFileTemplate(QDir::temp().absoluteFilePath("aspeqt-temp-XXXXXX"));
    if (!file.open()) {
        qCritical() << "!e" << tr("Cannot create new image: Cannot create temporary file '%2': %3.")
                      .arg(file.fileName())
                      .arg(file.errorString());
        return false;
    }

    m_geometry.initialize(file.size());
    refreshNewGeometry();
    m_originalFileHeader.clear();
    m_isReadOnly = false;
    m_originalFileName = tr("Untitled image %1").arg(untitledName);
    m_originalImageType = FileTypes::Atr;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = true;

    return true;
}

void SimpleDiskImage::reopen()
{
    close();
    open(m_originalFileName, m_originalImageType);
}

void SimpleDiskImage::close()
{
    if (m_editDialog) {
        delete m_editDialog;
    }
    file.close();
}

void SimpleDiskImage::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
        case 0x22:  // Format ED
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                if (m_isReadOnly) {
                    sio->port()->writeError();
                    qWarning() << "!w" << tr("[%1] Format ED denied.").arg(deviceName());
                    return;
                }
                m_newGeometry.initialize(false, 40, 26, 128);
                if (format(m_newGeometry)) {
                    QByteArray data(128, 0);
                    data[0] = data[1] = 0xFF;
                    sio->port()->writeComplete();
                    sio->port()->writeDataFrame(data);
                    qDebug() << "!n" << tr("[%1] Format ED.").arg(deviceName());
                } else {
                    sio->port()->writeError();
                    qCritical() << "!e" << tr("[%1] Format ED failed.").arg(deviceName());
                }
                break;
            }
        case 0x3F:  // Speed poll
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                sio->port()->writeComplete();
                QByteArray speed(1, 0);
                speed[0] = sio->port()->speedByte();
                sio->port()->writeDataFrame(speed);
                qDebug() << "!n" << tr("[%1] Speed poll.").arg(deviceName());
                break;
            }
        case 0x4e:  // Get PERCOM block
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                QByteArray percom = m_newGeometry.toPercomBlock();
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(percom);
                qDebug() << "!n" << tr("[%1] Get PERCOM block (%2).")
                               .arg(deviceName())
                               .arg(m_newGeometry.humanReadable());
                break;
            }
        case 0x4f:  // Set PERCOM block
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                QByteArray percom = sio->port()->readDataFrame(12);
                if (percom.isEmpty()) {
                    sio->port()->writeDataNak();
                    return;
                }
                sio->port()->writeDataAck();
                m_newGeometry.initialize(percom);

                sio->port()->writeComplete();
                qDebug() << tr("[%1] Set PERCOM block (%2).")
                              .arg(deviceName())
                              .arg(m_newGeometry.humanReadable());
                break;
            }
        case 0x66:  // Format with custom sector skewing
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                QByteArray percom = sio->port()->readDataFrame(128);
                if (percom.isEmpty()) {
                    sio->port()->writeDataNak();
                    break;
                }
                sio->port()->writeDataAck();
                m_newGeometry.initialize(percom);
                if (!m_isReadOnly) {
                    if (!format(m_newGeometry)) {
                        qCritical() << "!e" << tr("[%1] Format with custom sector skewing failed.")
                                       .arg(deviceName());
                        break;
                    }
                    sio->port()->writeComplete();
                    qWarning() << "!w" << tr("[%1] Format with custom sector skewing (%2).")
                                   .arg(deviceName())
                                   .arg(m_newGeometry.humanReadable());
                } else {
                    sio->port()->writeError();
                    qWarning() << "!w" << tr("[%1] Format with custom sector skewing denied.")
                                   .arg(deviceName());
                }
                break;
            }
        case 0x21:  // Format
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                if (!m_isReadOnly) {
                    format(m_newGeometry);
                    QByteArray data(m_newGeometry.bytesPerSector(), 0);
                    data[0] = 0xff;
                    data[1] = 0xff;
                    sio->port()->writeComplete();
                    sio->port()->writeDataFrame(data);
                    qWarning() << "!w" << tr("[%1] Format.").arg(deviceName());
                } else {
                    sio->port()->writeError();
                    qWarning() << "!w" << tr("[%1] Format denied.").arg(deviceName());
                    break;
                }

                break;
            }
        case 0x50:  // Write sector and verify
        case 0x57:  // Write sector without verifying
            {
                if (aux >= 1 && aux <= m_geometry.sectorCount()) {
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }
                    QByteArray data = sio->port()->readDataFrame(m_geometry.bytesPerSector(aux));
                    if (!data.isEmpty()) {
                        sio->port()->writeDataAck();
                        if (m_isReadOnly) {
                            sio->port()->writeError();
                            qWarning() << "!w" << tr("[%1] Write sector %2 denied.").arg(deviceName()).arg(aux);
                            break;
                        }
                        if (writeSector(aux, data)) {
                            sio->port()->writeComplete();
                            qDebug() << "!n" << tr("[%1] Write sector %2 (%3 bytes).").arg(deviceName()).arg(aux).arg(data.size());
                        } else {
                            sio->port()->writeError();
                            qCritical() << "!e" << tr("[%1] Write sector %2 failed.")
                                           .arg(deviceName())
                                           .arg(aux);
                        }
                    } else {
                        qCritical() << "!e" << tr("[%1] Write sector %2 data frame failed.")
                                       .arg(deviceName())
                                       .arg(aux);
                        sio->port()->writeDataNak();
                    }
                } else {
                    sio->port()->writeCommandNak();
                    qWarning() << "!w" << tr("[%1] Write sector %2 NAKed.")
                                   .arg(deviceName())
                                   .arg(aux);
                }
                break;
            }
        case 0x52:  // Read sector
            {
                if (aux >= 1 && aux <= m_geometry.sectorCount()) {
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }
                    QByteArray data;
                    if (readSector(aux, data)) {
                        sio->port()->writeComplete();
                        sio->port()->writeDataFrame(data);
                        qDebug() << "!n" << tr("[%1] Read sector %2 (%3 bytes).")
                                       .arg(deviceName())
                                       .arg(aux)
                                       .arg(data.size());
                    } else {
                        qCritical() << "!e" << tr("[%1] Read sector %2 failed.")
                                       .arg(deviceName())
                                       .arg(aux);
                        sio->port()->writeError();
                    }
                } else {
                    sio->port()->writeCommandNak();
                    qWarning() << "!w" << tr("[%1] Read sector %2 NAKed.")
                                   .arg(deviceName())
                                   .arg(aux);
                }
                break;
            }
        case 0x53:  // Get status
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }

                QByteArray status(4, 0);
                getStatus(status);
                
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);
                qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
                break;
            }
        default:    // Unknown command
            {
                sio->port()->writeCommandNak();
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                               .arg(deviceName())
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
                break;
            }
    }
}

void SimpleDiskImage::refreshNewGeometry()
{
    m_newGeometry.initialize(m_geometry);
}

bool SimpleDiskImage::format(const DiskGeometry &geo)
{
    if ((!file.resize(0)) || (!file.resize(geo.totalSize()))) {
        qCritical() << "!e" << tr("[%1] Cannot format: %2")
                       .arg(deviceName())
                       .arg(file.errorString());
    }
    m_geometry.initialize(geo);
    m_newGeometry.initialize(geo);
    m_originalFileHeader.clear();
    m_isModified = true;
    emit statusChanged(m_deviceNo);
    return true;
}

bool SimpleDiskImage::seekToSector(quint16 sector)
{
    if (sector < 1 || sector > m_geometry.sectorCount()) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
                       .arg(deviceName())
                       .arg(sector)
		       .arg(tr("Sector number is out of bounds."));
    }
    qint64 pos = (sector - 1) * m_geometry.bytesPerSector();
    if (m_geometry.bytesPerSector() == 256) {
        if (sector <= 3) {
            pos = (sector - 1) * 128;
        } else {
            pos -= 384;
        }
    }
    if (!file.seek(pos)) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
                       .arg(deviceName())
                       .arg(sector)
                       .arg(file.errorString());
        return false;
    }
    return true;
}

bool SimpleDiskImage::readSector(quint16 sector, QByteArray &data)
{
    if (!seekToSector(sector)) {
        return false;
    }
    data = file.read(m_geometry.bytesPerSector(sector));
    if (data.size() != m_geometry.bytesPerSector(sector)) {
        qCritical() << "!e" << tr("[%1] Cannot read from sector %2: %3.")
                       .arg(deviceName())
                       .arg(sector)
                       .arg(file.errorString());
        return false;
    }
    return true;
}

bool SimpleDiskImage::writeSector(quint16 sector, const QByteArray &data)
{
    if (!seekToSector(sector)) {
        return false;
    }
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    if (file.write(data) != data.size()) {
        qCritical() << "!e" << tr("[%1] Cannot write to sector %2: %3.")
                       .arg(deviceName())
                       .arg(sector)
                       .arg(file.errorString());
        return false;
    }
    return true;
}

void SimpleDiskImage::getStatus(QByteArray &status)
{
    status[0] = m_isReadOnly * 8 |
                (m_newGeometry.bytesPerSector() == 256) * 32 |
                (m_newGeometry.bytesPerSector() == 128 && m_newGeometry.sectorsPerTrack() == 26) * 128;
    status[1] = 0xFF;
    status[2] = 3;
    status[3] = 0;
}

int SimpleDiskImage::defaultFileSystem()
{
    if (m_geometry.sectorCount() < 1) {
        return 0;
    }

    QByteArray data;
    if (!readSector(1, data)) {
        return 0;
    }

    if (m_geometry.bytesPerSector() == 512) {
        if ((quint8)data.at(32) == 0x21) {
            return 5;
        } else {
            return 0;
        }
    } else {
        if ((quint8)data.at(7) == 0x80 /*&& ((quint8)data.at(32) == 0x21 || (quint8)data.at(32) == 0x20)*/) {
            return 5;
        } else {
            if (m_geometry.sectorCount() < 368) {
                return 0;
            }
            if (!readSector(360, data)) {
                return 0;
            }
            uint v = (quint8)data.at(0);
            uint s = (quint8)data.at(1) + (quint8)data.at(2) * 256;
            uint f = (quint8)data.at(3) + (quint8)data.at(4) * 256;
            if (m_geometry.isStandardSD() && v == 1 && s == 709 && f <= s) {
                return 1;
            } else if ((m_geometry.isStandardSD() || m_geometry.isStandardDD()) && v == 2 && s == 707 && f <= s) {
                return 2;
            } else if (m_geometry.isStandardED() && v == 2 && s == 1010 && f <= s) {
                return 3;
            } else if (v == 2 && s + 12 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else if (m_geometry.bytesPerSector() == 128 && s + (v * 2 - 3) + 10 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else if (m_geometry.bytesPerSector() == 256 && s + v + 9 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else {
                return 0;
            }
        }
    }
}
