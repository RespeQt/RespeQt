/*
 * diskimage.cpp
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 * Copyright 2018 Eric BACHER
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "diskimage.h"
#include "zlib.h"

#include <QFileInfo>
#include "respeqtsettings.h"
#include <QDir>
#include "atarifilesystem.h"
#include "diskeditdialog.h"

#include <QtDebug>

extern quint16 ATX_SECTOR_POSITIONS_SD[];
extern quint16 ATX_SECTOR_POSITIONS_ED[];

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
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate the magic number
    quint16 magic = static_cast<quint8>(header[0]) + static_cast<quint8>(header[1]) * 256;
    if (magic != 0x0296) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Not a valid ATR file."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Decode image meta-data
    quint16 sizeLo = static_cast<quint8>(header[2]) + static_cast<quint8>(header[3]) * 256;
    quint16 secSize = static_cast<quint8>(header[4]) + static_cast<quint8>(header[5]) * 256;
    quint32 sizeHi = static_cast<quint8>(header[6]) + static_cast<quint8>(header[7]) * 256;
    quint64 size = (sizeLo + sizeHi * 65536) * 16;

    // Try to create the temporary file
    file.setFileTemplate(QDir::temp().absoluteFilePath("respeqt-temp-XXXXXX"));
    if (!file.open()) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot create temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        sourceFile->close();
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
            sourceFile->close();
            delete sourceFile;
            file.close();
            return false;
        }
        if (file.write(buffer) != buffer.length()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot write to temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
            sourceFile->close();
            delete sourceFile;
            file.close();
            return false;
        }
    }

    auto imageSize = static_cast<quint64>(file.size());

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
        sourceFile->close();
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
            if ((size - 384) % 256 != 0 && ((size + 384) / 256) % 720 != 0) {
                sizeValid = false;
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
        sourceFile->close();
        delete sourceFile;
        file.close();
        return false;
    }

    // Validate disk geometry
    DiskGeometry geometry;
    geometry.initialize(size, secSize);
    if (geometry.sectorCount() > 65535) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Too many sectors in the image (%1).").arg(geometry.sectorCount()));
        sourceFile->close();
        delete sourceFile;
        file.close();
        return false;
    }

    // Check unsupported ATR extensions
    if (header[8] || header[9] || header[9] || header[10] || header[11] || header[12] || header[13] || header[14] || header[15]) {
        qWarning() << "!u" << tr("The file '%1' has some unrecognized fields in its header.")
                      .arg(fileName);
    }

    if (!file.resize(size)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot resize temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        sourceFile->close();
        delete sourceFile;
        file.close();
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
    m_trackNumber = 0;
    setReady(true);
    sourceFile->close();
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

    file.setFileTemplate(QDir::temp().absoluteFilePath("respeqt-temp-XXXXXX"));
    if (!file.open()) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot create temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        sourceFile->close();
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
            sourceFile->close();
            delete sourceFile;
            file.close();
            return false;
        }
        if (file.write(buffer) != buffer.length()) {
            qCritical() << "!e" << tr("Cannot open '%1': %2")
                          .arg(fileName)
                          .arg(tr("Cannot write to temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
            sourceFile->close();
            delete sourceFile;
            file.close();
            return false;
        }
    }

    auto size = static_cast<quint64>(file.size());

    if ((size % 128) != 0) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Invalid image size (%1).").arg(size));
        sourceFile->close();
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
    m_trackNumber = 0;
    setReady(true);
    sourceFile->close();
    delete sourceFile;
    return true;
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
        outputFile->close();
        delete outputFile;
        return false;
    }

    // Try to write the header
    if (outputFile->write(m_originalFileHeader) != 16) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile->errorString());
        outputFile->close();
        delete outputFile;
        return false;
    }

    // Try to copy the temporary file back
    if (!file.reset()) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot rewind temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        outputFile->close();
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
                          .arg(tr("Cannot read from temporary file %1: %2").arg(file.fileName()).arg(file.errorString()));
            outputFile->close();
            delete outputFile;
            return false;
        }
        if (outputFile->write(buffer) != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
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
    outputFile->close();
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
        return false;
    }

    // Try to copy the temporary file back
    if (!file.reset()) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot rewind temporary file '%1': %2").arg(file.fileName()).arg(file.errorString()));
        outputFile->close();
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
                          .arg(tr("Cannot read from temporary file %1: %2").arg(file.fileName()).arg(file.errorString()));
            outputFile->close();
            return false;
        }
        if (outputFile->write(buffer) != bufsize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
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
    outputFile->close();
    delete outputFile;
    return true;
}

bool SimpleDiskImage::saveAsAtr(const QString &fileName, FileTypes::FileType destImageType)
{
    if ((m_originalImageType != FileTypes::Pro) && (m_originalImageType != FileTypes::ProGz) && (m_originalImageType != FileTypes::Atx) && (m_originalImageType != FileTypes::AtxGz)) {
        qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving Atr images from the current format is not supported yet."));
        return false;
    }

    if ((destImageType == FileTypes::Atr) || (destImageType == FileTypes::AtrGz)) {
        m_originalFileHeader = QByteArray(16, 0);

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
    }

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

    bool warningDone = false;
    for (quint16 sector = 1; sector <= m_geometry.sectorCount(); sector++) {
        QByteArray data;
        m_conversionInProgress = true;
        readSector(sector, data);
        m_conversionInProgress = false;
        int sectorSize = (sector <= 3) ? 128 : m_geometry.bytesPerSector();
        if (data.size() < sectorSize) {
            data.fill(0, sectorSize);
        }
        if ((m_wd1771Status != 0xFF) && (!warningDone)) {
            warningDone = true;
            qWarning() << "!w" << tr("Some sector information is lost due to destination file format limitations.");
        }

        // Try to write the header
        if (outputFile->write(data) != m_geometry.bytesPerSector()) {
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

bool SimpleDiskImage::createAtr(int untitledName)
{
    file.setFileTemplate(QDir::temp().absoluteFilePath("respeqt-temp-XXXXXX"));
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
    setReady(true);

    return true;
}

bool SimpleDiskImage::readHappyAtrSectorAtPosition(int trackNumber, int sectorNumber, int , int &index, QByteArray &data)
{
    // compute sector index for the sector number
    int startHalf = (sectorNumber & 0x01) != 0 ? 0 : (m_geometry.sectorsPerTrack() >> 1);
    int indexInHalf = ((sectorNumber - 1) >> 1);
    index = startHalf + indexInHalf;

    // read sector content
    m_wd1771Status = 0xFF;
    int absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sectorNumber;
    return readSector(absoluteSector, data);
}

bool SimpleDiskImage::readHappyAtrSkewAlignment(bool happy1050)
{
    quint8 previousTrack = 0xFF - m_board.m_happyRam[0x3C9];
    if (previousTrack > 39) {
        qWarning() << "!w" << tr("[%1] Invalid previous track number %2 ($%3) for skew alignment")
                      .arg(deviceName())
                      .arg(previousTrack)
                      .arg(previousTrack, 2, 16, QChar('0'));
        return false;
    }
    quint8 currentTrack = 0xFF - m_board.m_happyRam[0x3CB];
    if (currentTrack > 39) {
        qWarning() << "!w" << tr("[%1] Invalid current track number %2 ($%3) for skew alignment")
                      .arg(deviceName())
                      .arg(previousTrack)
                      .arg(previousTrack, 2, 16, QChar('0'));
        return false;
    }

    // compute sector index for the sector number
    quint16 *sectorPositions = m_geometry.sectorsPerTrack() == 26 ? ATX_SECTOR_POSITIONS_ED : ATX_SECTOR_POSITIONS_SD;
    quint8 previousSector = 0xFF - m_board.m_happyRam[0x3CA];
    if ((previousSector < 1) || (previousSector > m_geometry.sectorsPerTrack())) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found in track %4 ($%5)")
                      .arg(deviceName())
                      .arg(previousSector)
                      .arg(previousSector, 2, 16, QChar('0'))
                      .arg(previousTrack)
                      .arg(previousTrack, 2, 16, QChar('0'));
        return false;
    }
    int startHalf = (previousSector & 0x01) != 0 ? 0 : (m_geometry.sectorsPerTrack() >> 1);
    int indexInHalf = ((previousSector - 1) >> 1);
    int previousIndex = startHalf + indexInHalf;
    int previousPosition = sectorPositions[previousIndex];
    quint8 currentSector = 0xFF - m_board.m_happyRam[0x3CC];
    if ((currentSector < 1) || (currentSector > m_geometry.sectorsPerTrack())) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found in track %4 ($%5)")
                      .arg(deviceName())
                      .arg(currentSector)
                      .arg(currentSector, 2, 16, QChar('0'))
                      .arg(currentTrack)
                      .arg(currentTrack, 2, 16, QChar('0'));
        return false;
    }
    startHalf = (currentSector & 0x01) != 0 ? 0 : (m_geometry.sectorsPerTrack() >> 1);
    indexInHalf = ((currentSector - 1) >> 1);
    int currentIndex = startHalf + indexInHalf;
    int currentPosition = sectorPositions[currentIndex];

    // compute timing between 2 sectors
    // The Happy algorithm decrements a counter from 0x210D to measure the time between 2 sectors
    // 0x210D-0x646 is the timing between 2 reads of sector 1 in the same track on a 810
    // If not on the same track, the counter starts after the seek (this explains that the final counter is smaller when a seek is executed)
    // 0x210D-0x77D is the same measure but for 2 reads of sector 1 in 2 adjacent tracks on a 810 (including a step in/out)
    int timingNoStep = happy1050 ? 0x061B : 0x646;
    int timingWithStep = happy1050 ? 0xB61 : 0x77D;
    int trackDiff = abs(currentTrack - previousTrack);
    int seekDiff = (timingWithStep - timingNoStep) * trackDiff;
    int distance = 26042 - previousPosition + currentPosition;
    if (distance > (26042 + (seekDiff * 4))) {
        distance -= 26042;
    }
    int timing = 0x210D - timingNoStep;
    int gap = (distance * timing) / 26042;
    gap = 0x210D - gap + seekDiff;

    // fill output buffer
    m_board.m_happyRam[0x380] = (quint8)(timingNoStep & 0xFF);
    m_board.m_happyRam[0x381] = (quint8)((timingNoStep >> 8) & 0xFF);
    m_board.m_happyRam[0x382] = (quint8)(timingNoStep & 0xFF);
    m_board.m_happyRam[0x383] = (quint8)((timingNoStep >> 8) & 0xFF);
    m_board.m_happyRam[0x384] = (quint8)(timingNoStep & 0xFF);
    m_board.m_happyRam[0x385] = (quint8)((timingNoStep >> 8) & 0xFF);
    m_board.m_happyRam[0x386] = (quint8)(gap & 0xFF);
    m_board.m_happyRam[0x387] = (quint8)((gap >> 8) & 0xFF);
    m_board.m_happyRam[0x388] = 0x01;
    m_board.m_happyRam[0x389] = 0x00;
    if (happy1050) {
        m_board.m_happyRam[0x3B0] = 0x23;
        m_board.m_happyRam[0x3B2] = 0x9A;
        m_board.m_happyRam[0x3B3] = 0x8D;
    }
    else {
        m_board.m_happyRam[0x38A] = 0x08;
        m_board.m_happyRam[0x390] = 0x05;
    }
    return true;
}

bool SimpleDiskImage::writeHappyAtrTrack(int trackNumber, bool happy1050)
{
    int absoluteSector = trackNumber * m_geometry.sectorsPerTrack();
    for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
        QByteArray empty(m_geometry.bytesPerSector(), 0);
        writeAtrSector(absoluteSector + i + 1, empty);
    }
    bool notNormal = false;
    int nbSectors = 0;
    int startOffset = happy1050 ? 0xD00 : 0x300;
    int offset = startOffset;
    auto invertedTrack = (quint8)(0xFF - trackNumber);
    while (offset < (startOffset + 0x100)) {
        quint8 code = m_board.m_happyRam[offset++];
        if (code == 0) {
            break;
        }
        if (code < 128) {
            nbSectors++;
            if ((quint8)m_board.m_happyRam[offset++] != invertedTrack) {
                notNormal = true;
                break;
            }
            if ((quint8)m_board.m_happyRam[offset++] != (quint8)0xFF) {
                notNormal = true;
                break;
            }
            if ((quint8)m_board.m_happyRam[offset++] < 0xED) {
                notNormal = true;
                break;
            }
            if ((quint8)m_board.m_happyRam[offset++] != (quint8)0xFF) {
                notNormal = true;
                break;
            }
            if ((quint8)m_board.m_happyRam[offset++] != (quint8)0x08) {
                notNormal = true;
                break;
            }
        }
        else {
            offset++;
        }
    }
    if (nbSectors != m_geometry.sectorsPerTrack()) {
        notNormal = true;
    }
    if (notNormal) {
        qWarning() << "!w" << tr("[%1] Write track can not write a non standard track with this kind of image.").arg(deviceName());
        return false;
    }
    return true;
}

bool SimpleDiskImage::writeHappyAtrSectors(int trackNumber, int, bool happy1050)
{
    int startOffset = happy1050 ? 0xC80 : 0x280;
    int startData = startOffset + 128;
    int lastIndex = (int) (quint8)m_board.m_happyRam[startOffset + 0x0F];
    int index = 17;
    while (index >= lastIndex) {
        quint8 sectorNumber = 0xFF - (quint8)m_board.m_happyRam[startOffset + 0x38 + index];
        m_board.m_happyRam[startOffset + 0x14 + index] = 0xFF;
        if ((sectorNumber > 0) && (sectorNumber <= m_geometry.sectorsPerTrack())) {
            int dataOffset = startData + (index * 128);
            int absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sectorNumber;
            bool result = writeAtrSector(absoluteSector, m_board.m_happyRam.mid(dataOffset,128));
            if (! result) {
                return false;
            }
            quint8 writeCommand = m_board.m_happyRam[startOffset + 0x4A + index];
            if (writeCommand != 0x57) {
                qWarning() << "!w" << tr("[%1] command $%2 not supported for sector %3 ($%4) with this kind of image. Ignored.")
                              .arg(deviceName())
                              .arg(writeCommand)
                              .arg(sectorNumber)
                              .arg(sectorNumber, 2, 16, QChar('0'));
            }
        }
        else {
            qWarning() << "!w" << tr("[%1] write unused sector %2 ($%3). Ignored.")
                          .arg(deviceName())
                          .arg(sectorNumber)
                          .arg(sectorNumber, 2, 16, QChar('0'));
        }
        index--;
    }
    if (! happy1050) {
        for (int i = 0; i < 5; i++) {
            m_board.m_happyRam[startOffset + 0x05 + i] = 0x80;
        }
        m_board.m_happyRam[startOffset + 0x0A] = 0x00;
        m_board.m_happyRam[startOffset + 0x0B] = 0x08;
        m_board.m_happyRam[startOffset + 0x0C] = 0x08;
        m_board.m_happyRam[startOffset + 0x0D] = 0x43;
        m_board.m_happyRam[startOffset + 0x10] = 0xFF;
        m_board.m_happyRam[startOffset + 0x13] = 0xEE;
    }
    return true;
}

bool SimpleDiskImage::formatAtr(const DiskGeometry &geo)
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
    // all sectors in a XFD file have the same size (this is the difference with the ATR format).
    // For example, XFD files are used to store CP/M disk images with 256 bytes boot sectors for Indus GT with RamCharger (not possible with ATR files).
    if ((m_geometry.bytesPerSector() != 128) && (m_originalImageType != FileTypes::Xfd) && (m_originalImageType != FileTypes::XfdGz)) {
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

void SimpleDiskImage::readAtrTrack(quint16 aux, QByteArray &data, int length)
{
    m_trackNumber = aux & 0x3F;
    bool useCount = (aux & 0x40) ? true : false;
    bool longHeader = (aux & 0x8000) ? true : false;

    data.resize(length);
    if (m_geometry.sectorsPerTrack() == 0) {
        data[0] = 0;
        return;
    }
    int maxHeader = longHeader ? 42 : 63;
    if (length == 128) {
        maxHeader >>= 1;
    }
    int nbSectors = 0;
    quint16 timeoutValue = 0;
    if (useCount) {
        timeoutValue = 0x7F;
        nbSectors = (aux & 0x4000) ? m_geometry.sectorsPerTrack() : (aux >> 8) & 0x1F;
    }
    else {
        timeoutValue = (aux >> 8) & 0x7F;
        nbSectors = maxHeader;
    }
    if (nbSectors > maxHeader) {
        nbSectors = maxHeader;
    }
    quint8 totalTiming = 0;
    int currentIndexInData = 0;
    data[currentIndexInData++] = nbSectors;
    quint8 firstTiming = 0;
    for (int i = 0; i < nbSectors; i++) {
        // Assume standard interleave: 1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18
        int sectorModulo = i % m_geometry.sectorsPerTrack();
        int sectorIndex = ((sectorModulo + 1) << 1);
        if (sectorModulo >= (m_geometry.sectorsPerTrack() >> 1)) {
            sectorIndex -= m_geometry.sectorsPerTrack() - 1;
        }
        quint8 sectorNumber = sectorIndex - 1;
        data[currentIndexInData++] = m_trackNumber;
#ifdef CHIP_810
        data[currentIndexInData++] = sectorModulo << 2;
#else
        data[currentIndexInData++] = 0;
#endif
        data[currentIndexInData++] = sectorNumber;
        data[currentIndexInData++] = 0;
        quint8 timing = 5 + (sectorModulo & 1);
        if ((totalTiming < 0x68) && ((i % m_geometry.sectorsPerTrack()) == (m_geometry.sectorsPerTrack() - 1))) {
            timing = 0x68 - totalTiming;
        }
        if (longHeader) {
            data[currentIndexInData++] = timing;
            data[currentIndexInData++] = 0;
        }
        if (i == 0) {
            firstTiming = timing;
        }
        totalTiming += timing;
        if ((! useCount) && ((totalTiming - firstTiming) > timeoutValue)) {
            data[0] = (quint8)(i + 1);
            break;
        }
    }
}

bool SimpleDiskImage::readAtrSectorStatuses(QByteArray &data)
{
    data.resize(128);
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    int currentIndexInTrack = m_board.m_chipRam[1];
    if (currentIndexInTrack == -1) {
        qWarning() << "!w" << tr("[%1] readSectorStatuses return -1").arg(deviceName());
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return true;
    }
    data[0] = nbSectors;
    data[0x40] = 0;
    for (int i = 0; i < nbSectors; i++) {
        QByteArray sector;
        readSectorUsingIndex((quint16) i, sector);
        quint8 value = sector[0];
        for (int k = 0; k < sector.size(); k++) {
            if ((quint8)sector[k] != value) {
                value = 1;
                break;
            }
        }
        data[i + 1] = 0xFF;
        data[i + 0x41] = value;
        currentIndexInTrack = (currentIndexInTrack + 1) % m_geometry.sectorsPerTrack();
    }
    return true;
}

bool SimpleDiskImage::readAtrSectorUsingIndex(quint16 aux, QByteArray &data)
{
    quint16 index = aux & 0x1F;
    int indexInTrack = index % m_geometry.sectorsPerTrack();
    int absoluteSector = (m_trackNumber * m_geometry.sectorsPerTrack()) + m_board.m_chipRam[indexInTrack + 1];
    return readSector(absoluteSector, data);
}

bool SimpleDiskImage::readAtrSector(quint16 aux, QByteArray &data)
{
    quint16 sector = m_board.isChipOpen() ? aux & 0x3FF : aux;
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

bool SimpleDiskImage::readAtrSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly)
{
	// skew alignment is not useful for ATR. Return default values
    m_board.m_trackData.clear();
    m_board.m_trackData.append(data);
    auto track = (quint8)((aux >> 8) & 0xFF);
    if (!timingOnly) {
        quint16 *sectorPositions = m_geometry.sectorsPerTrack() == 26 ? ATX_SECTOR_POSITIONS_ED : ATX_SECTOR_POSITIONS_SD;
        int nbSectors = m_geometry.sectorsPerTrack();
        for (int i = 0; i < nbSectors; i++) {
            // Assume standard interleave: 1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18
            int sectorModulo = i % m_geometry.sectorsPerTrack();
            int sectorIndex = ((sectorModulo + 1) << 1);
            if (sectorModulo >= (m_geometry.sectorsPerTrack() >> 1)) {
                sectorIndex -= m_geometry.sectorsPerTrack() - 1;
            }
            quint8 sectorNumber = sectorIndex - 1;
            auto sectorPosition = (quint16)(sectorPositions[i] >> 3);
            m_board.m_trackData[0x08 + i] = sectorNumber;
            m_board.m_trackData[0x28 + i] = (sectorPosition >> 8) & 0xFF;
            m_board.m_trackData[0x48 + i] = sectorPosition & 0xFF;
        }
        for (int i = nbSectors; i < 0x20; i++) {
            m_board.m_trackData[0x08 + i] = 0;
            m_board.m_trackData[0x28 + i] = 0;
            m_board.m_trackData[0x48 + i] = 0;
        }
    }
    else {
		static quint16 timings[] = {
			0xE85D, //track $01
			0xED16, //track $02
			0xF1CE, //track $03
			0xF687, //track $04
			0xFB40, //track $05
			0xD01B, //track $06
			0xD4D0, //track $07
			0xD98D, //track $08
			0xDE46, //track $09
			0xE2FF, //track $0A
			0xE7B8, //track $0B
			0xEC72, //track $0C
			0xF12A, //track $0D
			0xF5E4  //track $0E
        };
		if (track == 0) {
			// should not happen
            m_board.m_trackData[0xF4] = m_board.m_trackData[0xF5] = 0xE8;
		}
		else {
			quint16 timing = timings[(track - 1) % 14];
            m_board.m_trackData[0xF4] = (quint8)(timing & 0xFF);
            m_board.m_trackData[0xF5] = (quint8)((timing >> 8) & 0xFF);
		}
    }
    m_board.m_trackData[0] = m_deviceNo;
    m_board.m_trackData[1] = 0x74;
    m_board.m_trackData[2] = (quint8)(aux & 0xFF);
    m_board.m_trackData[3] = track;
    return true;
}

bool SimpleDiskImage::resetAtrTrack(quint16 aux)
{
    int trackNumber = aux & 0x3F;
    int absoluteSector = trackNumber * m_geometry.sectorsPerTrack();
    QByteArray empty(m_geometry.bytesPerSector(), 0);
    for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
        if (!writeAtrSector(absoluteSector + i + 1, empty)) {
            return false;
        }
    }
    return true;
}

bool SimpleDiskImage::writeAtrTrack(quint16 aux, const QByteArray &data)
{
    m_trackNumber = aux & 0x3F;
    for (int i = 0; i < 32; i++) {
        m_board.m_chipRam[i] = data[i];
    }
    int absoluteSector = m_trackNumber * m_geometry.sectorsPerTrack();
    for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
        quint8 fillByte = 0;
        for (int j = 0; j < 28; j++) {
            if (data[j + 1] == (i + 1)) {
                fillByte = data[57 + j];
                break;
            }
        }
        QByteArray empty(m_geometry.bytesPerSector(), fillByte);
        writeAtrSector(absoluteSector + i + 1, empty);
    }
    bool notNormal = false;
    quint8 nbSector = data[0];
    if (nbSector != m_geometry.sectorsPerTrack()) {
        notNormal = true;
    }
    else {
        for (int i = 0; i < nbSector; i++) {
            if (((quint8)data[1 + i] < 1) || ((quint16)data[1 + i] > m_geometry.sectorsPerTrack())) {
                notNormal = true;
                break;
            }
            if ((quint8)data[29 + i] != 0) {
                notNormal = true;
                break;
            }
            if ((quint8)data[85 + i] != 0x80) {
                notNormal = true;
                break;
            }
        }
    }
    if (notNormal) {
        qWarning() << "!w" << tr("[%1] Write track can not write a non standard track with this kind of image.").arg(deviceName());
        return false;
    }
    return true;
}

bool SimpleDiskImage::writeAtrTrackWithSkew(quint16 aux, const QByteArray &) {
    // skew alignment not supported
    return writeAtrTrack((aux & 0x3F) | 0xF000, m_board.m_trackData);
}

bool SimpleDiskImage::writeAtrSectorUsingIndex(quint16 aux, const QByteArray &data, bool)
{
    quint16 index = aux & 0x1F;
    int indexInTrack = index % m_geometry.sectorsPerTrack();

    // write the sector
    int absoluteSector = (m_trackNumber * m_geometry.sectorsPerTrack()) + m_board.m_chipRam[indexInTrack + 1];
    return writeAtrSector(absoluteSector, data);
}

bool SimpleDiskImage::writeFuzzyAtrSector(quint16 aux, const QByteArray &data)
{
	// Fussy sectors are not supported so we write the sector as if it was a standard Write Sector command
	qWarning() << "!w" << tr("[%1] Fuzzy sectors are not supported with Atr/Xfd file format").arg(deviceName());
	return writeAtrSector(aux & 0x3FF, data);
}

bool SimpleDiskImage::writeAtrSector(quint16 aux, const QByteArray &data)
{
    quint16 sector = m_board.isChipOpen() ? aux & 0x3FF : aux;
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

bool SimpleDiskImage::writeAtrSectorExtended(int, quint8, quint8 trackNumber, quint8, quint8 sectorNumber, quint8, const QByteArray &data, bool, int)
{
    if ((sectorNumber < 1) || (sectorNumber > m_geometry.sectorsPerTrack())) {
        return true;
    }
    int absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sectorNumber;
    return writeAtrSector(absoluteSector, data);
}

int SimpleDiskImage::sectorsInCurrentAtrTrack()
{
    if (m_trackNumber <= 39) {
        return m_geometry.sectorsPerTrack();
    }
    return 0;
}
