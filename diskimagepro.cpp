/*
 * diskimagepro.cpp
 *
 * Copyright 2017 josch1710
 * Copyright 2017 blind
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

extern quint8 FDC_CRC_PATTERN[];

bool SimpleDiskImage::openPro(const QString &fileName)
{
    QFile *sourceFile;

    if (m_originalImageType == FileTypes::Pro) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    // Try to open the source file
    if (!sourceFile->open(QFile::Unbuffered | QFile::ReadOnly)) {
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
    if (header[2] != 'P') {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Not a valid PRO file."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }
    if (header[3] != '3') {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Unsupported PRO file version"));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate the size
    quint16 magic = getBigIndianWord(header, 0);
    if (magic != ((sourceFile->size()-16)/(128+12))) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Unsupported PRO file size"));
        sourceFile->close();
        delete sourceFile;
        return false;
    }
    int numberOfSectors = getBigIndianWord(header, 6);
    if (numberOfSectors > 1040) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Number of sectors (%1) not supported (max 1040)").arg(numberOfSectors));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate disk geometry because fillProSectorInfo needs it
    m_geometry.initialize(0, 40, numberOfSectors / 40, 128);
    refreshNewGeometry();

    // initialize the array containing the sector headers
    for (unsigned int i = 0; i < sizeof(m_proSectorInfo) / sizeof(ProSectorInfo); i++) {
        m_proSectorInfo[i].used = false;
    }

    // Load all sectors in memory.
    // Sector Headers are kept in memory and phantom sectors also.
    for (quint16 nbSectors = 0; (nbSectors < numberOfSectors) && (!sourceFile->atEnd()); nbSectors++) {
        if (!fillProSectorInfo(fileName, sourceFile, nbSectors, nbSectors + 1)) {
            sourceFile->close();
            delete sourceFile;
            return false;
        }
    }

    // the remaining sectors are phantom sectors.
    // Load them in memory
    for (quint16 nbPhantoms = 0; (nbPhantoms < 256) && (!sourceFile->atEnd()); nbPhantoms++) {
        if (!fillProSectorInfo(fileName, sourceFile, 1040 + nbPhantoms, 0xFFFF)) {
            sourceFile->close();
            delete sourceFile;
            return false;
        }
    }

    // check for weak sectors. PRO image does not support weak sectors but if there are 6 duplicate sectors
    // with the same starting bytes, we assume that we have a weak sector.
    for (quint16 nbSectors = 0; nbSectors < numberOfSectors; nbSectors++) {
        if (m_proSectorInfo[nbSectors].used) {
            if (m_proSectorInfo[nbSectors].totalDuplicate > 0) {

                // fix sectorNumber and absoluteSector fields of duplicate sectors (slot number >= 1040)
                for (int j = 0; j < m_proSectorInfo[nbSectors].totalDuplicate; j++) {
                    quint16 phantomSlot = 1040 + (quint16)m_proSectorInfo[nbSectors].duplicateOffset[j] - 1;
                    m_proSectorInfo[phantomSlot].sectorNumber = m_proSectorInfo[nbSectors].sectorNumber;
                    m_proSectorInfo[phantomSlot].absoluteSector = m_proSectorInfo[nbSectors].absoluteSector;
                }
            }
            if (((quint8)m_proSectorInfo[nbSectors].wd1771Status & 0x10) != 0) {
                guessWeakSectorInPro(nbSectors);
            }
        }
    }

    if (m_displayTrackLayout) {
        qDebug() << "!n" << tr("Track layout for %1").arg(fileName);
        m_trackNumber = -1;
        for (int track = 0; track < 40; track++) {
            QByteArray secBuf;
            QByteArray data;
            readProTrack(track, data, 128);
            for (int sector = 0; sector < m_sectorsInTrack; sector++) {
                quint16 indexInProSector = m_trackContent[sector];
                if (secBuf.size() > 0) {
                    secBuf.append(' ');
                }
                quint8 sectorStatus = m_proSectorInfo[indexInProSector].wd1771Status;
                if (sectorStatus & 0x10) {
                    secBuf.append(tr("%1").arg(m_proSectorInfo[indexInProSector].sectorNumber, 2, 16, QChar('0')));
		            // display only the one word to keep the line as compact as possible
                    if (m_proSectorInfo[indexInProSector].weakBits != 0xFFFF) {
                        secBuf.append("(WEAK)");
                    }
                    else if ((sectorStatus & 0x20) == 0) {
                            secBuf.append("(DEL)");
                    }
                    else if ((sectorStatus & 0x08) == 0) {
                        secBuf.append("(CRC)");
                    }
                    else if (indexInProSector >= 1040) {
                        secBuf.append("(DUP)");
                    }
                }
            }
            qDebug() << "!u" << tr("track $%1: %2").arg(track, 2, 16, QChar('0')).arg(secBuf.constData());
        }
    }

    // reset some variables
    m_lastSector = 0;
    m_lastDistance = 0;
    m_lastTime = 0;
    m_trackNumber = -1;
    m_driveStatus = 0x10;
    m_isReadOnly = sourceFile->isWritable();
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = false;
    setReady(true);
    sourceFile->close();
    delete sourceFile;
    return true;
}

bool SimpleDiskImage::savePro(const QString &fileName)
{
    if (m_originalFileHeader.size() != 16) {
        m_originalFileHeader = QByteArray(16, 0);
    }

    // Compute total number of sectors
    int nbSectors = m_geometry.sectorCount();
    for (int i = 0; i < 256; i++) {
        if (m_proSectorInfo[1040 + i].used) {
            nbSectors++;
        }
    }

    m_originalFileHeader[0] = (nbSectors >> 8) & 0xFF;
    m_originalFileHeader[1] = nbSectors & 0xFF;
    m_originalFileHeader[2] = 0x50;
    m_originalFileHeader[3] = 0x33;
    m_originalFileHeader[4] = 2;
    m_originalFileHeader[6] = (m_geometry.sectorCount() >> 8) & 0xFF;
    m_originalFileHeader[7] = m_geometry.sectorCount() & 0xFF;

    // Try to open the output file
    QFile *outputFile;

    if (m_originalImageType == FileTypes::Pro) {
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
        outputFile->close();
        delete outputFile;
        return false;
    }

    // write all normal sectors
    for (uint i = 0; i < m_geometry.sectorCount(); i++) {
        QByteArray sectorHeader = QByteArray(12, 0);
        sectorHeader[0] = m_proSectorInfo[i].driveStatus;
        if (m_proSectorInfo[i].used) {
            sectorHeader[1] = m_proSectorInfo[i].wd1771Status;
        }
        else {
            sectorHeader[1] = 0xEF; // no sector
        }
        sectorHeader[2] = 0xE0;
        sectorHeader[4] = 0xF0;
        sectorHeader[5] = m_proSectorInfo[i].totalDuplicate;
        for (int j = 0; j < m_proSectorInfo[i].totalDuplicate; j++) {
            sectorHeader[7 + j] = m_proSectorInfo[i].duplicateOffset[j];
        }
        if (outputFile->write(sectorHeader) != 12) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }
        QByteArray sectorData = m_proSectorInfo[i].sectorData;
        if (sectorData.size() != 128) {
            sectorData = QByteArray(128, 0);
        }
        if (outputFile->write(sectorData) != 128) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }
    }

    // write phantom sectors if any
    for (int i = 0; i < 256; i++) {
        if (m_proSectorInfo[1040 + i].used) {
            QByteArray sectorHeader = QByteArray(12, 0);
            sectorHeader[0] = m_proSectorInfo[1040 + i].driveStatus;
            sectorHeader[1] = m_proSectorInfo[1040 + i].wd1771Status;
            sectorHeader[2] = 0xE0;
            sectorHeader[4] = 0xF0;
            sectorHeader[5] = m_proSectorInfo[1040 + i].totalDuplicate;
            for (int j = 0; j < m_proSectorInfo[1040 + i].totalDuplicate; j++) {
                sectorHeader[7 + j] = m_proSectorInfo[1040 + i].duplicateOffset[j];
            }
            if (outputFile->write(sectorHeader) != 12) {
                qCritical() << "!e" << tr("Cannot save '%1': %2")
                              .arg(fileName)
                              .arg(outputFile->errorString());
                outputFile->close();
                delete outputFile;
                return false;
            }
            QByteArray sectorData = m_proSectorInfo[1040 + i].sectorData;
            if (sectorData.size() != 128) {
                sectorData = QByteArray(128, 0);
            }
            if (outputFile->write(sectorData) != 128) {
                qCritical() << "!e" << tr("Cannot save '%1': %2")
                              .arg(fileName)
                              .arg(outputFile->errorString());
                outputFile->close();
                delete outputFile;
                return false;
            }
        }
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

bool SimpleDiskImage::saveAsPro(const QString &fileName, FileTypes::FileType destImageType)
{
    bool bareSectors = (m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz);
    if ((!bareSectors) && (m_originalImageType != FileTypes::Atx) && (m_originalImageType != FileTypes::AtxGz)) {
        qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving Pro images from the current format is not supported yet."));
        return false;
    }

    // initialize the array containing the sector headers
    for (unsigned int i = 0; i < sizeof(m_proSectorInfo) / sizeof(ProSectorInfo); i++) {
        m_proSectorInfo[i].used = false;
    }

    // the idea is to fill pro structure then call savePro
    bool warningDone = false;
    int nextPhantomSlot = 1040;
    for (uint slot = 0; slot < m_geometry.sectorCount(); slot++) {

        // read the main sector and get status and distance if
        QByteArray data;
        quint16 absoluteSector = slot + 1;
        m_conversionInProgress = true;
        m_lastDistance = 0;
        m_lastTime = 0;
        m_lastSector = absoluteSector;
        readSector(absoluteSector, data);
        qint64 mainDistance = m_lastDistance;
        quint8 mainStatus = m_wd1771Status;
        m_conversionInProgress = false;

        // fill main sector header
        m_proSectorInfo[slot].used = true;
        m_proSectorInfo[slot].firstPass = true;
        m_proSectorInfo[slot].sectorNumber = absoluteSector != 0xFFFF ? (quint8)(slot % m_geometry.sectorsPerTrack()) + 1 : (quint8)0xFF;
        m_proSectorInfo[slot].absoluteSector = absoluteSector;
        m_proSectorInfo[slot].driveStatus = m_driveStatus;
        m_proSectorInfo[slot].wd1771Status = m_wd1771Status;
        m_proSectorInfo[slot].reservedByte = 0;
        m_proSectorInfo[slot].totalDuplicate = 0;
        m_proSectorInfo[slot].sectorTiming = (quint8)(5 + (slot & 1));
        m_proSectorInfo[slot].weakBits = (quint16)0xFFFF;
        if (((m_wd1771Status & 0x10) != 0) && (!bareSectors)) {
            qint64 phantomDistance[5];
            for (int duplicate = 0; duplicate < 5; duplicate++) {

                // try to read a phantom sector
                QByteArray phantomData;
                m_conversionInProgress = true;
                m_lastTime = 0;
                readSector(absoluteSector, phantomData);
                phantomDistance[duplicate] = m_lastDistance;
                m_conversionInProgress = false;

                // check if we really have another sector
                bool sameSector = false;
                if ((mainDistance == m_lastDistance) && (mainStatus == m_wd1771Status)) {
                    if (data.size() == phantomData.size()) {
                        if (data == phantomData) {
                            sameSector = true;
                        }
                    }
                }
                if (sameSector) {
                    break;
                }
                for (int otherSector = 0; otherSector < duplicate; otherSector++) {
                    if ((phantomDistance[otherSector] == m_lastDistance) && (m_proSectorInfo[nextPhantomSlot + otherSector].wd1771Status == m_wd1771Status)) {
                        if (m_proSectorInfo[nextPhantomSlot + otherSector].sectorData.size() == phantomData.size()) {
                            if (m_proSectorInfo[nextPhantomSlot + otherSector].sectorData == phantomData) {
                                sameSector = true;
                                break;
                            }
                        }
                    }
                }
                if (sameSector) {
                    break;
                }

                // too much duplicate sectors for Pro format file
                if (nextPhantomSlot + duplicate >= 1040 + 256) {
                    if (!warningDone) {
                        warningDone = true;
                        qWarning() << "!w" << tr("The number of duplicate sectors exceeds the Pro file capacity.");
                    }
                    break;
                }

                // this is another phantom sector
                m_proSectorInfo[nextPhantomSlot + duplicate].used = true;
                m_proSectorInfo[nextPhantomSlot + duplicate].firstPass = true;
                m_proSectorInfo[nextPhantomSlot + duplicate].sectorNumber = absoluteSector != 0xFFFF ? (quint8)(slot % m_geometry.sectorsPerTrack()) + 1 : (quint8)0xFF;
                m_proSectorInfo[nextPhantomSlot + duplicate].absoluteSector = absoluteSector;
                m_proSectorInfo[nextPhantomSlot + duplicate].driveStatus = m_driveStatus;
                m_proSectorInfo[nextPhantomSlot + duplicate].wd1771Status = m_wd1771Status;
                m_proSectorInfo[nextPhantomSlot + duplicate].reservedByte = 0;
                m_proSectorInfo[nextPhantomSlot + duplicate].totalDuplicate = 0;
                m_proSectorInfo[nextPhantomSlot + duplicate].sectorTiming = (quint8)(5 + (slot & 1));
                m_proSectorInfo[nextPhantomSlot + duplicate].weakBits = (quint16)0xFFFF;
                m_proSectorInfo[nextPhantomSlot + duplicate].notEmpty = false;

                // check if sector is empty
                if (((m_proSectorInfo[nextPhantomSlot + duplicate].wd1771Status & 0x10) != 0) && (phantomData.size() > 0)) {
                    quint8 value = phantomData[0];
                    for (quint8 i = 0; i < phantomData.size(); i++) {
                        m_proSectorInfo[nextPhantomSlot + duplicate].sectorData[i] = phantomData[i];
                        if ((quint8) m_proSectorInfo[nextPhantomSlot + duplicate].sectorData[i] != 0) {
                            m_proSectorInfo[nextPhantomSlot + duplicate].notEmpty = true;
                        }
                        if ((quint8) m_proSectorInfo[nextPhantomSlot + duplicate].sectorData[i] != value) {
                            value = 1;
                        }
                    }
                    m_proSectorInfo[nextPhantomSlot + duplicate].fillByte = value;
                }

                // set duplicate info into main sector
                m_proSectorInfo[slot].duplicateOffset[m_proSectorInfo[slot].totalDuplicate] = nextPhantomSlot + duplicate - 1040 + 1;
                m_proSectorInfo[slot].totalDuplicate++;
            }
        }
        if (((quint8)m_proSectorInfo[slot].wd1771Status & 0x10) != 0) {
            guessWeakSectorInPro(slot);
        }
        nextPhantomSlot += m_proSectorInfo[slot].totalDuplicate;

        // check if sector is empty
        m_proSectorInfo[slot].notEmpty = false;
        if (((m_proSectorInfo[slot].wd1771Status & 0x10) != 0) && (data.size() > 0)) {
            quint8 value = m_proSectorInfo[slot].sectorData[0];
            for (quint8 i = 0; i < data.size(); i++) {
                m_proSectorInfo[slot].sectorData[i] = data[i];
                if ((quint8) m_proSectorInfo[slot].sectorData[i] != 0) {
                    m_proSectorInfo[slot].notEmpty = true;
                }
                if ((quint8) m_proSectorInfo[slot].sectorData[i] != value) {
                    value = 1;
                }
            }
            m_proSectorInfo[slot].fillByte = value;
        }
    }

    // save the Pro structure in a file
    m_originalFileHeader.resize(0);
    m_originalImageType = destImageType;
    return savePro(fileName);
}

bool SimpleDiskImage::createPro(int untitledName)
{
    m_geometry.initialize(false, 40, 18, 128);
    refreshNewGeometry();
    m_originalFileHeader.clear();
    m_isReadOnly = false;
    m_originalFileName = tr("Untitled image %1").arg(untitledName);
    m_originalImageType = FileTypes::Pro;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = true;

    // initialize the array containing the sector headers
    for (unsigned int i = 0; i < sizeof(m_proSectorInfo) / sizeof(ProSectorInfo); i++) {
        m_proSectorInfo[i].used = false;
    }
    for (unsigned int i = 0; i < m_geometry.sectorCount(); i++) {
        m_proSectorInfo[i].used = true;
        m_proSectorInfo[i].sectorNumber = (i % m_geometry.sectorsPerTrack()) + 1;
        m_proSectorInfo[i].absoluteSector = i + 1;
        m_proSectorInfo[i].totalDuplicate = 0;
        m_proSectorInfo[i].driveStatus = 0x10;
        m_proSectorInfo[i].wd1771Status = 0xFF; // no error
        m_proSectorInfo[i].sectorTiming = (5 + (i & 1));
        m_proSectorInfo[i].notEmpty = false;
        m_proSectorInfo[i].fillByte = 0;
        m_proSectorInfo[i].sectorData.resize(m_geometry.bytesPerSector());
        for (int j = 0; j < m_geometry.bytesPerSector(); j++) {
            m_proSectorInfo[i].sectorData[j] = 0;
        }
    }
    m_trackNumber = 0xFFFF;
    setReady(true);
    return true;
}

bool SimpleDiskImage::readHappyProSectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data)
{
    data.resize(128);
    if (trackNumber != m_trackNumber) {
        QByteArray dummy;
        readProTrack((quint16)trackNumber, dummy, 128);
    }

    // find the first index to start with if afterSectorNumber has been set.
    if (afterSectorNumber != 0) {
        // try to find the index of the specified sector in the track
        index = 0;
        for (int i = 0;  i < m_sectorsInTrack; i++) {
            quint16 indexInProSector = m_trackContent[i];
            quint8 sector = m_proSectorInfo[indexInProSector].sectorNumber;
            if (sector == afterSectorNumber) {
                index = (i + 1) % m_sectorsInTrack;
                break;
            }
        }
    }

    // find the next sector matching the requested sector number
    m_wd1771Status = 0xEF;
    quint16 indexInProSector = 0xFFFF;
    for (int i = 0; i < m_sectorsInTrack; i++) {
        int indexInTrack = index % m_sectorsInTrack;
        quint16 currentIndexInProSector = m_trackContent[indexInTrack];
        quint8 sector = m_proSectorInfo[currentIndexInProSector].sectorNumber;
        if (sector == (quint8)sectorNumber) {
            indexInProSector = currentIndexInProSector;
            m_wd1771Status = m_proSectorInfo[indexInProSector].wd1771Status;
            break;
        }
        else {
            index++;
        }
    }
    if (indexInProSector == 0xFFFF) {
        qWarning() << "!w" << tr("[%1] sector %2 ($%3) not found starting at index %4")
                      .arg(deviceName())
                      .arg(sectorNumber)
                      .arg(sectorNumber, 2, 16, QChar('0'))
                      .arg(index);
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }

    // read sector data
    for (int i = 0; i < 128; i++) {
        data[i] = m_proSectorInfo[indexInProSector].sectorData[i];
    }
    return true;
}

bool SimpleDiskImage::readHappyProSkewAlignment(bool happy1050)
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

    // read sector list of the first track
    QByteArray firstTrackData;
    readProTrack(previousTrack, firstTrackData, 256);
    int firstTrackSectorCount = m_sectorsInTrack;
    quint16 firstTrackContent[200];
    memcpy(firstTrackContent, m_trackContent, sizeof(firstTrackContent));

    // read sector list of the second track
    QByteArray secondTrackData;
    readProTrack(currentTrack, secondTrackData, 256);

    // compute sector index for the sector number
    quint8 previousSector = 0xFF - m_board.m_happyRam[0x3CA];
    bool found = false;
    int previousPosition = 0;
    for (int index = 0; index < firstTrackSectorCount; index++) {
        quint16 sectorSlot = firstTrackContent[index];
        quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
        quint8 firstSectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
        if (previousSector == firstSectorNumber) {
            found = true;
            break;
        }
        previousPosition += (25 << 3);
        if (shortSectorSize != 0) {
            previousPosition += (shortSectorSize << 3);
        }
        else {
            previousPosition += (140 << 3);
        }
    }
    if (!found) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found in track %4 ($%5)")
                      .arg(deviceName())
                      .arg(previousSector)
                      .arg(previousSector, 2, 16, QChar('0'))
                      .arg(previousTrack)
                      .arg(previousTrack, 2, 16, QChar('0'));
        return false;
    }
    quint8 currentSector = 0xFF - m_board.m_happyRam[0x3CC];
    found = false;
    int currentPosition = 0;
    for (int index = 0; index < m_sectorsInTrack; index++) {
        quint16 sectorSlot = m_trackContent[index];
        quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
        quint8 secondSectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
        if (currentSector == secondSectorNumber) {
            found = true;
            break;
        }
        currentPosition += (25 << 3);
        if (shortSectorSize != 0) {
            currentPosition += (shortSectorSize << 3);
        }
        else {
            currentPosition += (140 << 3);
        }
    }
    if (!found) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found in track %4 ($%5)")
                      .arg(deviceName())
                      .arg(currentSector)
                      .arg(currentSector, 2, 16, QChar('0'))
                      .arg(currentTrack)
                      .arg(currentTrack, 2, 16, QChar('0'));
        return false;
    }

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

bool SimpleDiskImage::writeHappyProTrack(int trackNumber, bool happy1050)
{
    // reset track data
    quint16 slot = trackNumber * m_geometry.sectorsPerTrack();
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
        quint8 duplicate = m_proSectorInfo[slot + i].totalDuplicate;
        m_proSectorInfo[slot + i].used = false;
        m_proSectorInfo[slot + i].wd1771Status = 0xEF; // no sector
        m_proSectorInfo[slot + i].totalDuplicate = 0;
        m_proSectorInfo[slot + i].notEmpty = false;
        m_proSectorInfo[slot + i].fillByte = 0;
        m_proSectorInfo[slot + i].weakBits = (quint16)0xFFFF;
        m_proSectorInfo[slot + i].sectorTiming = (5 + (i & 1));
        m_proSectorInfo[slot + i].lastSectorRead = 0;
        for (quint8 phantomIndex = 0; phantomIndex < duplicate; phantomIndex++) {
            quint16 phantomSlot = 1040 + (quint8)m_proSectorInfo[slot + i].duplicateOffset[phantomIndex] - 1;
            m_proSectorInfo[phantomSlot].used = false;
            m_proSectorInfo[phantomSlot].firstPass = false;
            m_proSectorInfo[phantomSlot].sectorData.resize(0);
            m_proSectorInfo[phantomSlot].sectorNumber = 0xFF;
            m_proSectorInfo[phantomSlot].absoluteSector = 0xFFFF;
        }
    }
    m_trackNumber = trackNumber;
    m_sectorsInTrack = 0;

    // browse track data
    int startOffset = happy1050 ? 0xD00 : 0x300;
    int offset = startOffset;
    quint8 invertedTrack = (quint8) (0xFF - trackNumber);
    while (offset < (startOffset + 0x100)) {
        quint8 code = m_board.m_happyRam[offset++];
        if (code == 0) {
            break;
        }
        if (code < 128) {
            if (((quint8)m_board.m_happyRam[offset] == invertedTrack) && ((quint8)m_board.m_happyRam[offset + 1] == (quint8)0xFF) && ((quint8)m_board.m_happyRam[offset + 2] >= 0xED)
                    && ((quint8)m_board.m_happyRam[offset + 3] == (quint8)0xFF) && ((quint8)m_board.m_happyRam[offset + 4] == (quint8)0x08)) {
                quint8 sector = 0xFF - (quint8)m_board.m_happyRam[offset + 2];
                quint8 normalSize = (quint8)m_board.m_happyRam[offset + 3] == (quint8)0xFF;

                // fill corresponding slot
                if ((sector > 0) && (sector <= m_geometry.sectorsPerTrack())) {
                    quint16 sectorSlot = (trackNumber * m_geometry.sectorsPerTrack()) + sector - 1;
                    if (m_proSectorInfo[sectorSlot].used) {
                        quint8 duplicates = m_proSectorInfo[sectorSlot].totalDuplicate;
                        if (duplicates < 5) {
                            quint16 phantomSlot = 0;
                            // find an empty phantom slot
                            for (quint16 j = 0; j < 255; j++) {
                                if (!m_proSectorInfo[1040 + j].used) {
                                    phantomSlot = 1040 + j;
                                    break;
                                }
                            }
                            if (phantomSlot == 0) {
                                qWarning() << "!w" << tr("[%1] More than 255 phantom sectors (unsupported with PRO format)")
                                              .arg(deviceName());
                                return false;
                            }
                            m_proSectorInfo[sectorSlot].duplicateOffset[duplicates] = phantomSlot - 1040 + 1;
                            m_proSectorInfo[sectorSlot].totalDuplicate = ++duplicates;
                            m_proSectorInfo[phantomSlot].used = true;
                            m_proSectorInfo[phantomSlot].sectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
                            m_proSectorInfo[phantomSlot].absoluteSector = m_proSectorInfo[sectorSlot].absoluteSector;
                            m_proSectorInfo[phantomSlot].totalDuplicate = 0;
                            m_proSectorInfo[phantomSlot].weakBits = (quint16)0xFFFF;
                            m_proSectorInfo[phantomSlot].lastSectorRead = 0;
                            sectorSlot = phantomSlot;
                        }
                        else {
                            qWarning() << "!w" << tr("[%1] More than 6 phantom sectors for a given sector number (unsupported with PRO format)")
                                          .arg(deviceName());
                            return false;
                        }
                    }
                    else {
                        m_proSectorInfo[sectorSlot].used = true;
                        m_proSectorInfo[sectorSlot].sectorNumber = sector;
                        m_proSectorInfo[sectorSlot].absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sector;
                        m_proSectorInfo[sectorSlot].totalDuplicate = 0;
                    }
                    m_proSectorInfo[sectorSlot].driveStatus = 0x10;
                    if (normalSize) {
                        m_proSectorInfo[sectorSlot].wd1771Status = 0xFF; // no error
                    }
                    else {
                        m_proSectorInfo[sectorSlot].wd1771Status = 0xF1;
                    }
                    m_proSectorInfo[sectorSlot].sectorTiming = 5;

                    // fill track content
                    m_trackContent[m_sectorsInTrack++] = sectorSlot;

                    // reserve sector data
                    m_proSectorInfo[sectorSlot].sectorData.resize(128);
                    m_proSectorInfo[sectorSlot].notEmpty = false;
                }
            }
            offset += 5;
        }
        else {
            offset++;
        }
    }
    return true;
}

bool SimpleDiskImage::writeHappyProSectors(int trackNumber, int afterSectorNumber, bool happy1050)
{
    if (trackNumber != m_trackNumber) {
        QByteArray dummy;
        readProTrack((quint16)trackNumber, dummy, 128);
    }

    bool sync = (quint8) m_board.m_happyRam[0x280] == 0;
    // find the first index to start with.
    int position = 0;
    if (sync && (afterSectorNumber != 0)) {
        // try to find the index of the specified sector in the track
        for (int i = 0;  i < m_sectorsInTrack; i++) {
            quint16 indexInProSector = m_trackContent[i];
            quint8 sector = m_proSectorInfo[indexInProSector].sectorNumber;
            if (sector == afterSectorNumber) {
                position = (i + 1) % m_sectorsInTrack;
                break;
            }
        }
    }
    int startOffset = happy1050 ? 0xC80 : 0x280;
    int startData = startOffset + 128;
    int lastIndex = (int) (quint8)m_board.m_happyRam[startOffset + 0x0F];
    bool twoPasses = (quint8)m_board.m_happyRam[startOffset + 0x69] == 0;
    int maxPass = twoPasses ? 2 : 1;
    for (int passNumber = 1; passNumber <= maxPass; passNumber++) {
        int index = 18 - passNumber;
        while (index >= lastIndex) {
            quint8 sectorNumber = 0xFF - (quint8)m_board.m_happyRam[startOffset + 0x38 + index];
            m_board.m_happyRam[startOffset + 0x14 + index] = 0xEF;
            if ((sectorNumber > 0) && (sectorNumber <= m_geometry.sectorsPerTrack())) {
                quint16 indexInProSector = 0xFFFF;
                for (int i = 0; i < m_sectorsInTrack; i++) {
                    int indexInTrack = position % m_sectorsInTrack;
                    quint16 currentIndexInProSector = m_trackContent[indexInTrack];
                    quint8 sector = m_proSectorInfo[currentIndexInProSector].sectorNumber;
                    position++;
                    if (sector == sectorNumber) {
                        indexInProSector = currentIndexInProSector;
                        quint8 writeCommand = m_board.m_happyRam[startOffset + 0x4A + index];
                        int dataOffset = startData + (index * 128);
                        quint8 dataMark = (writeCommand << 5) | 0x9F;
                        QByteArray data = m_board.m_happyRam.mid(dataOffset,128);
                        for (int j = 0; j < 128; j++) {
                            m_proSectorInfo[indexInProSector].sectorData[j] = data[j];
                        }
                        quint8 fdcStatus = m_proSectorInfo[indexInProSector].wd1771Status & dataMark; // use the data mark given in the command
                        if (writeCommand & 0x08) { // non-IBM format generates a CRC error
                            fdcStatus &= ~0x08;
                        }
                        m_proSectorInfo[indexInProSector].wd1771Status = fdcStatus;
                        m_board.m_happyRam[startOffset + 0x14 + index] = fdcStatus;
                        break;
                    }
                }
                if (indexInProSector == 0xFFFF) {
                    qWarning() << "!w" << tr("[%1] sector %2 ($%3) not found. Ignored.")
                                  .arg(deviceName())
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
            if (twoPasses) {
                index--;
            }
            index--;
        }
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

bool SimpleDiskImage::formatPro(const DiskGeometry &geo)
{
    if (geo.bytesPerSector() != 128) {
        qCritical() << "!e" << tr("Can not format PRO image: %1")
                      .arg(tr("Sector size (%1) not supported (should be 128)").arg(geo.bytesPerSector()));
        return false;
    }
    if (geo.sectorCount() > 1040) {
        qCritical() << "!e" << tr("Can not format PRO image: %1")
                      .arg(tr("Number of sectors (%1) not supported (max 1040)").arg(geo.sectorCount()));
        return false;
    }

    // initialize the array containing the sector headers
    for (unsigned int i = 0; i < sizeof(m_proSectorInfo) / sizeof(ProSectorInfo); i++) {
        m_proSectorInfo[i].used = false;
    }
    for (unsigned int i = 0; i < m_geometry.sectorCount(); i++) {
        m_proSectorInfo[i].used = true;
        m_proSectorInfo[i].sectorNumber = (i % geo.sectorsPerTrack()) + 1;
        m_proSectorInfo[i].absoluteSector = i + 1;
        m_proSectorInfo[i].totalDuplicate = 0;
        m_proSectorInfo[i].driveStatus = 0x10;
        m_proSectorInfo[i].wd1771Status = 0xFF; // no error
        m_proSectorInfo[i].sectorTiming = (5 + (i & 1));
        m_proSectorInfo[i].notEmpty = false;
        m_proSectorInfo[i].fillByte = 0;
        m_proSectorInfo[i].sectorData.resize(geo.bytesPerSector());
        for (int j = 0; j < geo.bytesPerSector(); j++) {
            m_proSectorInfo[i].sectorData[j] = 0;
        }
    }
    m_trackNumber = 0xFFFF;
    m_geometry.initialize(geo);
    m_newGeometry.initialize(geo);
    m_originalFileHeader.clear();
    m_isModified = true;
    emit statusChanged(m_deviceNo);
    return true;
}

void SimpleDiskImage::readProTrack(quint16 aux, QByteArray &data, int length)
{
    quint16 track = aux & 0x3F;
    bool useCount = (aux & 0x40) ? true : false;
    bool longHeader = (aux & 0x8000) ? true : false;
    m_wd1771Status = 0xFF;

    if (m_trackNumber != track) {

        // reset flags
        for (unsigned int i = 0; i < sizeof(m_proSectorInfo) / sizeof(ProSectorInfo); i++) {
            m_proSectorInfo[i].firstPass = false;
            m_proSectorInfo[i].inserted = false;
            m_proSectorInfo[i].paired = false;
            m_proSectorInfo[i].beforeSlot = 0xFFFF;
        }

        // get sectors (no RNF and no phantom sectors)
        m_sectorsInTrack = 0;
        quint16 trackContent[256];
        for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {

            // compute sector number for the sector index
            int sectorIndex = (((i % m_geometry.sectorsPerTrack()) + 1) << 1);
            if (i >= (m_geometry.sectorsPerTrack() >> 1)) {
                sectorIndex -= m_geometry.sectorsPerTrack() - 1;
            }
            quint8 sector = sectorIndex - 1;

            // add sector slot in track if sector has data
            quint16 absoluteSector = (track * m_geometry.sectorsPerTrack()) + sector;
            quint16 slot = absoluteSector - 1;
            if (m_proSectorInfo[slot].used && ((m_proSectorInfo[slot].wd1771Status & 0x10) == 0x10)) {
                m_proSectorInfo[slot].firstPass = true;
                if (m_proSectorInfo[slot].weakBits == 0xFFFF) {
                    // try to find a position if the data contains both an ID Address Mark and a Data Address Mark
                    m_proSectorInfo[slot].beforeSlot = findPositionInProTrack(track, slot, false);
                }
                trackContent[m_sectorsInTrack++] = slot;
            }
        }

        // check for phantom sectors.
        // Weak sectors are skipped as they need to be inserted as a single sector (already done in the previous loop)
        // Here we add all alternate phantom sectors in the track trying to spread them in the track.
        int sectorIndex = 0;
        for (int i = m_sectorsInTrack - 1; i >= 0; i--) {
            quint16 indexInProSector = trackContent[sectorIndex];
            m_proSectorInfo[indexInProSector].firstPass = false;
            quint8 totalDuplicate = m_proSectorInfo[indexInProSector].totalDuplicate;
            quint16 weakBits = m_proSectorInfo[indexInProSector].weakBits;
            if ((totalDuplicate != 0) && (weakBits == 0xFFFF)) {

                // try to find a position for all phantom sectors.
                // Some PRO images that work with SIO emulation, do not work when using 810 firmware emulation.
                // This is mainly because of duplicate sectors that are not spreaded correctly in the track.
                // The protection fails because the same sector content is read twice instead of the 2 copies.
                // The non working tiltles are:
                //   Jumpman
                //   Karateka
                //   Maniac Miner
                //   Murder on the Zinderneuf
                //   Spy vs Spy (GB)
                //   Starbase Fighter
                //
                int insertionPoint = sectorIndex;
                int increment = ((m_sectorsInTrack - totalDuplicate) / (totalDuplicate + 1)) + 1;
                // put the second sector a bit far away to make Ali Baba (US)(OS-B) work. Otherwise, the same sector is read repeatedly.
                // But, with this code, Maniac Miner does not load. Both games are exclusive.
                // Don't know how to infere the right distance and the right order. Seems impossible to manage with no more information.
                if (totalDuplicate == 1) {
                    increment = (m_sectorsInTrack + 3) >> 1;
                }
                if (increment < 1) {
                    increment = 1;
                }
                for (int j = 0; j < totalDuplicate; j++) {
                    quint16 phantomSlot = 1040 + m_proSectorInfo[indexInProSector].duplicateOffset[j] - 1;
                    m_proSectorInfo[phantomSlot].firstPass = false;
                    // try to find a position if the data contains both an ID Address Mark and a Data Address Mark
                    m_proSectorInfo[phantomSlot].beforeSlot = findPositionInProTrack(track, phantomSlot, false);
                    insertionPoint = (insertionPoint + increment) % m_sectorsInTrack;
                    for (int k = m_sectorsInTrack - 1; k >= insertionPoint; k--) {
                        trackContent[k + 1] = trackContent[k];
                    }
                    m_sectorsInTrack++;
                    trackContent[insertionPoint] = phantomSlot;
                    // swap sectors if there are only 2 duplicates.
                    // Some games relies on the fact that the order of the sectors would be different than during a sequential read.
                    // This swap allows Koronis Rift, Sands of Egypt and Living Daylights to work
                    if (totalDuplicate == 1) {
                        if (sectorIndex > insertionPoint) {
                            sectorIndex++;
                        }
                        trackContent[insertionPoint] = trackContent[sectorIndex];
                        trackContent[sectorIndex] = phantomSlot;
                    }
                }
            }
            sectorIndex++;
            while ((sectorIndex < m_sectorsInTrack - 1) && (! m_proSectorInfo[trackContent[sectorIndex]].firstPass)) {
                sectorIndex++;
            }
        }

        // for the sector without constraint, try to find a position with ID Address Mark but not Data Address Mark
        sectorIndex = 0;
        for (int i = 0; i < m_sectorsInTrack; i++) {
            quint16 slot = trackContent[i];
            if (m_proSectorInfo[slot].beforeSlot == 0xFFFF) {
                m_proSectorInfo[slot].beforeSlot = findPositionInProTrack(track, slot, true);
            }
        }

        // copy to destination list all sectors without position constraint
        sectorIndex = 0;
        for (int i = 0; i < m_sectorsInTrack; i++) {
            quint16 slot = trackContent[i];
            if (m_proSectorInfo[slot].beforeSlot == 0xFFFF) {
                m_trackContent[sectorIndex++] = slot;
                m_proSectorInfo[slot].inserted = true;
            }
        }

        // copy all other sectors respecting the constraints
        int nbInserted;
        int nbRemaining;
        do {
            nbInserted = 0;
            nbRemaining = 0;
            for (int i = 0; i < m_sectorsInTrack; i++) {
                quint16 slot = trackContent[i];
                if (! m_proSectorInfo[slot].inserted) {
                    int insertionPoint = -1;
                    for (int j = 0; j < sectorIndex; j++) {
                        quint16 nextSlot = m_trackContent[j];
                        if (m_proSectorInfo[slot].beforeSlot == nextSlot) {
                            insertionPoint = j;
                            break;
                        }
                    }
                    if (insertionPoint != -1) {
                        for (int k = sectorIndex - 1; k >= insertionPoint; k--) {
                            m_trackContent[k + 1] = m_trackContent[k];
                        }
                        m_proSectorInfo[slot].inserted = true;
                        m_trackContent[insertionPoint] = slot;
                        sectorIndex++;
                        nbInserted++;
                    }
                    else {
                        nbRemaining++;
                    }
                }
            }

            // if nothing has been inserted and there are remaining sectors, it means we have a cycle.
            // take the first remaining sector to break the cycle.
            if ((nbInserted == 0) && (nbRemaining > 0)) {
                for (int i = 0; i < m_sectorsInTrack; i++) {
                    quint16 slot = trackContent[i];
                    if (! m_proSectorInfo[slot].inserted) {
                        m_proSectorInfo[slot].inserted = true;
                        m_trackContent[sectorIndex] = slot;
                        sectorIndex++;
                        nbInserted++;
                        break;
                    }
                }
            }
        }
        while ((nbInserted > 0) && (nbRemaining > 0));

        // copy remaining sectors at the end (there should be no sector left but just in case...)
        if (sectorIndex < m_sectorsInTrack) {
            for (int i = 0; i < m_sectorsInTrack; i++) {
                quint16 slot = trackContent[i];
                if (! m_proSectorInfo[slot].inserted) {
                    m_proSectorInfo[slot].inserted = true;
                    m_trackContent[sectorIndex++] = slot;
                }
            }
            m_sectorsInTrack = sectorIndex;
        }

        // some games have a track with the exact same sector repeated 18 times (Twerps, Repton, Blade of Blackpool,...).
        // They check the speed to get the sector repeatedly. APE PRO System stores only one sector.
        // Try to emulate this protection by making 18 exact copies.
        if (m_sectorsInTrack == 1) {
            int uniqueSlot = -1;
            for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
                int sectorIndex = (track * m_geometry.sectorsPerTrack()) + i;
                if ((m_proSectorInfo[sectorIndex].used) && (m_proSectorInfo[sectorIndex].wd1771Status & 0x10)) {
                    uniqueSlot = sectorIndex;
                }
            }
            if (uniqueSlot != -1) {
                for (int i = 1; i < m_geometry.sectorsPerTrack(); i++) {
                    m_trackContent[m_sectorsInTrack++] = uniqueSlot;
                }
            }
        }
        m_trackNumber = track;
    }

    data.resize(length);
    if (m_sectorsInTrack == 0) {
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
        nbSectors = (aux & 0x4000) ? m_sectorsInTrack : (aux >> 8) & 0x1F;
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
    int currentIndexInTrack = 0;
    data[currentIndexInData++] = nbSectors;
    quint8 firstTiming = 0;
    for (int i = 0; i < nbSectors; i++) {
        quint16 indexInProSector = m_trackContent[currentIndexInTrack];
        data[currentIndexInData++] = track;
#ifdef CHIP_810
        data[currentIndexInData++] = (i % m_sectorsInTrack) << 2;
#else
        data[currentIndexInData++] = 0;
#endif
        data[currentIndexInData++] = m_proSectorInfo[indexInProSector].sectorNumber;
        data[currentIndexInData++] = 0;
        quint8 timing = m_proSectorInfo[indexInProSector].sectorTiming;
        if ((totalTiming < 0x68) && ((i % m_sectorsInTrack) == (m_sectorsInTrack - 1))) {
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
        currentIndexInTrack = (currentIndexInTrack + 1) % m_sectorsInTrack;
    }
    for (int i = currentIndexInData; i < length; i++) {
        data[currentIndexInData++] = 0;
    }
}

bool SimpleDiskImage::readProSectorStatuses(QByteArray &data)
{
    data.resize(128);
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInProTrack(nbSectors, mapping)) {
        qWarning() << "!w" << tr("[%1] sector layout does not map to track layout")
                      .arg(deviceName());
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }
    data[0] = nbSectors;
    data[0x40] = 0;
    for (int i = 0; i < nbSectors; i++) {
        int currentIndexInTrack = (int)(quint32)mapping[i];
        quint16 indexInProSector = m_trackContent[currentIndexInTrack];
        data[i + 1] = m_proSectorInfo[indexInProSector].wd1771Status;
        data[i + 0x41] = m_proSectorInfo[indexInProSector].fillByte;
        currentIndexInTrack = (currentIndexInTrack + 1) % m_sectorsInTrack;
    }
    return true;
}

bool SimpleDiskImage::readProSectorUsingIndex(quint16 aux, QByteArray &data)
{
    data.resize(128);
    quint16 index = aux & 0x1F;
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInProTrack(nbSectors, mapping)) {
        qWarning() << "!w" << tr("[%1] sector layout does not map to track layout")
                      .arg(deviceName());
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }
    int indexInTrack = (int)(quint32)mapping[index];
    quint16 indexInProSector = m_trackContent[indexInTrack];
    quint8 sectorNumber = m_proSectorInfo[indexInProSector].sectorNumber;
    if ((quint8)m_board.m_chipRam[index + 1] != sectorNumber) {
        qWarning() << "!w" << tr("[%1] sector %2 does not match sector number at index %3")
                      .arg(deviceName())
                      .arg(sectorNumber)
                      .arg(index);
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }

    // check sector status
    bool readData = true;
    if ((m_proSectorInfo[indexInProSector].wd1771Status & 0x10) == 0) {
        data.resize(128);
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        readData = false;
    }

    // read sector if there is one
    if (readData) {
        for (int i = 0; i < 128; i++) {
            data[i] = m_proSectorInfo[indexInProSector].sectorData[i];
        }
    }
    return true;
}

// Many PRO images are working well with this implementation but some titles refuse to run.
// These titles do not work either when using 810 firmware emulation.
// The non working titles are
//   Ballblazer
//   Boulder Dash
//   Living Daylights
//   Rescue on Fractalus
//   Spy vs Spy 2
//
bool SimpleDiskImage::readProSector(quint16 aux, QByteArray &data)
{
    // 1 disk rotation = 208333us
    // Compute the relative head position since the last read command (assume disk never stops spinning)
    qint64 newTimeInNanoSeconds = m_timer.nsecsElapsed();
    qint64 diffTimeInMicroSeconds = (m_lastTime == 0) ? 0 : ((newTimeInNanoSeconds - m_lastTime) / 1000);
    qint64 distance = (m_lastDistance + diffTimeInMicroSeconds) % 208333L;

    // chipFlags contains the CHIP copy flags in AUX2. If not 0, it means that the Atari is running Archiver 1.0.
    // Archiver 1.0 always sends flags in AUX2 but Super Archiver 3.02 fills flags in AUX2 only if sector is bad.
    // So we assume that we have Super Archiver 3.02 if speed is greater than 20000 in CHIP mode.
    // In both cases, the delay at the end of readProSector is disabled to have the highest speed (no delay between sectors)
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    if ((m_board.isChipOpen()) && (sio->port()->speed() > 20000)) {
        chipFlags |= 0x8000; // disable accurate timing
    }
    quint16 sector = aux & 0x3FF;

    // compute delay if head was not on the right track
    int oldTrack = (m_lastSector - 1) / m_geometry.sectorsPerTrack();
    int newTrack = (sector - 1) / m_geometry.sectorsPerTrack();
    int diffTrack = abs(newTrack - oldTrack);
    unsigned long seekDelay = (unsigned long) diffTrack * 5200L;

    // adjust relative head position when head was moving to another track
    if (diffTrack > 0) {
        distance = (distance + seekDelay) % 208333L;
    }

    // check if there are bad or phantom sectors for this sector number
    int nbPhantoms = 1;
    int phantomIndex = 1;
    quint16 slot = sector - 1;
    if (!m_proSectorInfo[slot].used) {
        m_driveStatus = 0x10;
        m_wd1771Status = 0xEF;
    }
    else {

        // get the next sector content if we have a phantom sector
        if ((m_proSectorInfo[slot].totalDuplicate != 0) && (m_proSectorInfo[slot].weakBits == 0xFFFF) && (chipFlags == 0)) {
            int dupnum = m_proSectorInfo[slot].lastSectorRead;
            nbPhantoms = m_proSectorInfo[slot].totalDuplicate;
            phantomIndex = dupnum + 1;

            m_proSectorInfo[slot].lastSectorRead = (m_proSectorInfo[slot].lastSectorRead + 1) % (m_proSectorInfo[slot].totalDuplicate + 1);
            if (dupnum != 0)  {
                slot = 1040 + (quint8)m_proSectorInfo[slot].duplicateOffset[dupnum - 1] - 1;
                if (!m_proSectorInfo[slot].used) {
                    qCritical() << "!e" << tr("[%1] Sector %2 (phantom %3) does not exist in PRO file")
                                   .arg(deviceName())
                                   .arg(sector)
                                   .arg(m_proSectorInfo[slot].duplicateOffset[dupnum]);
                    return false;
                }
            }
        }
        m_driveStatus = m_proSectorInfo[slot].driveStatus;
        m_wd1771Status = m_proSectorInfo[slot].wd1771Status;
        if ((m_wd1771Status & 0x04) == 0) {
            m_wd1771Status &= 0xFD;
        }
    }

    // compute delay for the head to be on the right sector.
    // Assume standard interleave: 1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18
    int sectorInTrack = ((sector - 1) % m_geometry.sectorsPerTrack());
    int sectorIndex = sectorInTrack >> 1;
    if (sectorInTrack & 1) {
        sectorIndex += m_geometry.sectorsPerTrack() >> 1;
    }
    qint64 headDistance = ((unsigned long) sectorIndex * 208333L) / m_geometry.sectorsPerTrack();
    qint64 fetchDelay = 0;
    if (headDistance > distance) {
        fetchDelay = (headDistance - distance) % 208333L;
    }
    else {
        fetchDelay = (headDistance + 208333L - distance) % 208333L;
    }
    distance = headDistance;

    // check sector status
    bool readData = true;
    quint16 weakOffset = 0xFFFF;
    if (m_wd1771Status != 0xFF) {
        if ((m_wd1771Status & 0x10) == 0) {
            if (!m_conversionInProgress) {
                qDebug() << "!u" << tr("[%1] Bad sector (status $%2) Grrr Grrr !")
                            .arg(deviceName())
                            .arg(m_wd1771Status, 2, 16, QChar('0'));
            }
            data.resize(128);
            readData = false;

            // no sector found. The delay is very long. Use an approximation:
            // 4 rotations to find the sector, then 44 step out, then seek to current track; repeated 2 times.
            seekDelay += ((208333L << 2) + ((44 + newTrack) * 5300)) << 1;
        }
        else if ((m_wd1771Status & 0x08) == 0) {
            weakOffset = m_proSectorInfo[slot].weakBits;
            if (weakOffset != 0xFFFF) {
                if (!m_conversionInProgress) {
                    qDebug() << "!u" << tr("[%1] Weak sector at offset %2")
                                .arg(deviceName())
                                .arg(weakOffset);
                }
            }
            else if (nbPhantoms > 1) {
                // the firmware retries a second read when a CRC error is found.
                // Fix the timing.
                fetchDelay += 208333L / nbPhantoms;
                if (!m_conversionInProgress) {
                    qDebug() << "!u" << tr("[%1] CRC error (status $%2) on sector index #%3 among %4 phantom sectors")
                                .arg(deviceName())
                                .arg(m_wd1771Status, 2, 16, QChar('0'))
                                .arg(phantomIndex)
                                .arg(nbPhantoms);
                }
            }
            else {
                // the firmware retries a second read when a CRC error is found.
                // We know that there is no other sector. Just fix the timing with another full rotation.
                fetchDelay += 208333L;
                if (!m_conversionInProgress) {
                    qDebug() << "!u" << tr("[%1] CRC error (status $%2)")
                                .arg(deviceName())
                                .arg(m_wd1771Status, 2, 16, QChar('0'));
                }
            }
        }
        else {
            if (!m_conversionInProgress) {
                qDebug() << "!u" << tr("[%1] Read status $%2")
                            .arg(deviceName())
                            .arg(m_wd1771Status, 2, 16, QChar('0'));
            }
        }
    }
    else if (nbPhantoms > 1) {
        if (!m_conversionInProgress) {
            qDebug() << "!u" << tr("[%1] Read sector index #%2 among %3 phantom sectors")
                        .arg(deviceName())
                        .arg(phantomIndex)
                        .arg(nbPhantoms);
        }
    }

    // read sector if there is one
    if (readData) {
        data = m_proSectorInfo[slot].sectorData;
        if (weakOffset != 0xFFFF) {
            for (int i = weakOffset; i < data.size(); i++) {
                data[i] = qrand() & 0xFF;
            }
        }

        // Some games have a track with the exact same sector repeated 18 times (Twerps, Repton, Blade of Blackpool,...).
        // They check the speed to get the sector repeatedly. APE PRO System stores only one sector.
        // Try to emulate this protection by disabling the accurate timing when the Atari software reads several times the same sector.
        if (m_lastSector == sector) {
            chipFlags |= 0x8000; // disable accurate timing
        }
    }

    // simulate accurate timing.
    if ((!m_conversionInProgress) && (chipFlags == 0)) {
        unsigned long delayInMicroSeconds = seekDelay + fetchDelay;
        unsigned long diffWorkInMicroSeconds = (unsigned long)((m_timer.nsecsElapsed() - newTimeInNanoSeconds) / 1000);
        if (delayInMicroSeconds > diffWorkInMicroSeconds) {
            delayInMicroSeconds -= diffWorkInMicroSeconds;
            QThread::usleep(delayInMicroSeconds);
        }
    }
    m_lastDistance = (headDistance + 1) % 208333L;
    m_lastSector = sector;
    m_lastTime = m_timer.nsecsElapsed();
    return true;
}

bool SimpleDiskImage::readProSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly)
{
	// read sector list of the first track to compare to the list given by Super Archiver
	int firstTrack = aux & 0xFF;
	QByteArray firstTrackData;
	readProTrack(firstTrack, firstTrackData, 256);
	int firstTrackSectorCount = m_sectorsInTrack;
	quint16 firstTrackContent[200];
	memcpy(firstTrackContent, m_trackContent, sizeof(firstTrackContent));

	// read sector list of the second track
	int secondTrack = (aux >> 8) & 0xFF;
	QByteArray secondTrackData;
	readProTrack(secondTrack, secondTrackData, 256);

	// overwrite input data
    m_board.m_trackData.clear();
    m_board.m_trackData.append(data);

    // Super Archive expects a sector list with timing
    if (!timingOnly) {

        // find the index in the first track of the sector list given by the Super Archiver
        quint8 nbSectorsToFind = data[3];
        for (int startIndex = 0; startIndex < firstTrackSectorCount; startIndex++) {
            bool found = true;
            for (int i = 0; i <= nbSectorsToFind; i++) {
                int index = (startIndex + i) % firstTrackSectorCount;
                quint16 firstSectorSlot = firstTrackContent[index];
                quint8 firstSectorNumber = m_proSectorInfo[firstSectorSlot].sectorNumber;
                quint8 sectorNumber = data[5 + (i % nbSectorsToFind)];
                if (sectorNumber != firstSectorNumber) {
                    found = false;
                    break;
                }
            }
            if (found) {

                // now find the byte offset in the first track of this first sector
                int firstTrackByteOffset = 0;
                int lastIndex = (startIndex + nbSectorsToFind) % firstTrackSectorCount;
                for (int index = 0; index < lastIndex; index++) {
                    quint16 sectorSlot = firstTrackContent[index];
                    quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
                    firstTrackByteOffset += 25;
                    if (shortSectorSize != 0) {
                        firstTrackByteOffset += shortSectorSize;
                    }
                    else {
                        firstTrackByteOffset += 140;
                    }
                }

                // add the seek time
                quint16 seekTimeInBytes = (quint16)((104100 + (20550 * (secondTrack - firstTrack - 1))) >> 6);;
                firstTrackByteOffset = (firstTrackByteOffset + seekTimeInBytes) % (26042 >> 3);

                // find the first sector at the same rotation angle in the second track
                int nextIndex = -1;
                int secondTrackByteOffset = 0;
                quint16 diffByteOffset = 0;
                for (int index = 0; index < m_sectorsInTrack; index++) {
                    quint16 sectorSlot = m_trackContent[index];
                    quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
                    secondTrackByteOffset += 25;
                    if (shortSectorSize != 0) {
                        secondTrackByteOffset += shortSectorSize;
                    }
                    else {
                        secondTrackByteOffset += 140;
                    }
                    if (secondTrackByteOffset >= firstTrackByteOffset) {
                        diffByteOffset = secondTrackByteOffset - firstTrackByteOffset;
                        nextIndex = index;
                        break;
                    }
                }

                // fill output buffer with sector numbers and byte offsets
                for (int index = 0; index < m_sectorsInTrack; index++) {
                    int secondTrackIndex = (index + nextIndex) % m_sectorsInTrack;
                    quint16 sectorSlot = m_trackContent[secondTrackIndex];
                    quint8 sectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
                    quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
                    secondTrackByteOffset = 25;
                    if (shortSectorSize != 0) {
                        secondTrackByteOffset += shortSectorSize;
                    }
                    else {
                        secondTrackByteOffset += 140;
                    }
                    diffByteOffset += secondTrackByteOffset;
                    m_board.m_trackData[0x08 + index] = sectorNumber;
                    m_board.m_trackData[0x28 + index] = (diffByteOffset >> 8) & 0xFF;
                    m_board.m_trackData[0x48 + index] = diffByteOffset & 0xFF;
                }

                for (int i = m_sectorsInTrack; i < 0x20; i++) {
                    m_board.m_trackData[0x08 + i] = 0;
                }
                m_board.m_trackData[0] = m_deviceNo;
                m_board.m_trackData[1] = 0x74;
                m_board.m_trackData[2] = (quint8)firstTrack;
                m_board.m_trackData[3] = (quint8)secondTrack;
                return true;
            }
        }
        return false;
    }
    else {

        // find position of the first sector
        bool found = false;
        int firstSectorPosition = 0;
        for (int index = 0; index < firstTrackSectorCount; index++) {
            quint16 sectorSlot = firstTrackContent[index];
            quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
            quint8 firstSectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
            quint8 sectorNumber = data[3];
            if (sectorNumber == firstSectorNumber) {
                found = true;
                break;
            }
            firstSectorPosition += (25 << 3);
            if (shortSectorSize != 0) {
                firstSectorPosition += (shortSectorSize << 3);
            }
            else {
                firstSectorPosition += (140 << 3);
            }
        }
        if (!found) {
            return false;
        }

        // add the time to change track and issue another read sector command: 115429 microseconds for one track difference
        quint16 seekTime = (quint16)((115429 + (20550 * (secondTrack - firstTrack - 1))) >> 3);
        // add also the time corresponding to the reading of the sector data: 154 bytes = 9856 microseconds
        firstSectorPosition = (firstSectorPosition + seekTime + (154 << 3)) % 26042;

        // now find the sector in the second track from the current rotation angle
        int secondSectorPosition = 0;
        for (int index = 0; index < m_sectorsInTrack; index++) {
            quint16 sectorSlot = m_trackContent[index];
            quint8 shortSectorSize = m_proSectorInfo[sectorSlot].shortSectorSize;
            quint8 secondSectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
            quint8 sectorNumber = data[4];
            if (sectorNumber == secondSectorNumber) {
                found = true;
                break;
            }
            secondSectorPosition += (25 << 3);
            if (shortSectorSize != 0) {
                secondSectorPosition += (shortSectorSize << 3);
            }
            else {
                secondSectorPosition += (140 << 3);
            }
        }
        if (!found) {
            return false;
        }
        secondSectorPosition = (secondSectorPosition + (154 << 3) + 1245) % 26042;

        // compute distance between the 2 sectors
        quint16 nbBits = 0;
        if (secondSectorPosition > firstSectorPosition) {
            nbBits = (secondSectorPosition - firstSectorPosition) % 26042;
        }
        else {
            nbBits = (26042 + secondSectorPosition - firstSectorPosition)  % 26042;
        }
        quint32 nbMicroSeconds = nbBits << 3;

        // store result for Read memory command. The high byte is incremented each 17 cycles * 256 = 4352 cycles
        m_board.m_trackData[0xF4] = 0xFF - ((nbMicroSeconds % 4352) / 17);
        m_board.m_trackData[0xF5] = 0xFF - (nbMicroSeconds / 4352) + 1;
        m_board.m_trackData[0] = m_deviceNo;
        m_board.m_trackData[1] = 0x74;
        m_board.m_trackData[2] = (quint8)firstTrack;
        m_board.m_trackData[3] = (quint8)secondTrack;
        return true;
    }
}

bool SimpleDiskImage::resetProTrack(quint16 aux)
{
    int trackNumber = aux & 0x3F;
    quint16 slot = trackNumber * m_geometry.sectorsPerTrack();
    for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
        quint8 duplicate = m_proSectorInfo[slot + i].totalDuplicate;
        m_proSectorInfo[slot + i].used = false;
        m_proSectorInfo[slot + i].wd1771Status = 0xEF; // no sector
        m_proSectorInfo[slot + i].totalDuplicate = 0;
        m_proSectorInfo[slot + i].notEmpty = false;
        m_proSectorInfo[slot + i].fillByte = 0;
        m_proSectorInfo[slot + i].weakBits = (quint16)0xFFFF;
        m_proSectorInfo[slot + i].sectorTiming = (5 + (i & 1));
        m_proSectorInfo[slot + i].lastSectorRead = 0;
        m_proSectorInfo[slot + i].sectorData.resize(0);
        for (quint8 phantomIndex = 0; phantomIndex < duplicate; phantomIndex++) {
            quint16 phantomSlot = 1040 + (quint8)m_proSectorInfo[slot + i].duplicateOffset[phantomIndex] - 1;
            m_proSectorInfo[phantomSlot].used = false;
            m_proSectorInfo[phantomSlot].firstPass = false;
            m_proSectorInfo[phantomSlot].sectorData.resize(0);
            m_proSectorInfo[phantomSlot].sectorNumber = 0xFF;
            m_proSectorInfo[phantomSlot].absoluteSector = (quint16)0xFFFF;
        }
    }
    return true;
}

bool SimpleDiskImage::writeProTrack(quint16 aux, const QByteArray &data)
{
    quint16 firstTrack = aux & 0x3F;
    quint8 nbTracks = aux & 0x40 ? data[0x76] : 1;
    bool useSectorList = (aux & 0x2000) != 0;
    quint8 postIDCrc = (aux & 0x8000) ? data[0x74] : 17;
    quint8 postDataCrc = (aux & 0x8000) ? data[0x73] : 9;
    quint8 preIDField = (aux & 0x8000) ? data[0x72] : 6;

    m_trackNumber = 0xFFFF; // will be computed again in readTrack
    m_sectorsInTrack = 0;
    for (quint16 track = firstTrack; track < (firstTrack + nbTracks); track++) {

        // reset track data
        quint16 slot = track * m_geometry.sectorsPerTrack();
        if (!m_isModified) {
            m_isModified = true;
            emit statusChanged(m_deviceNo);
        }
        for (int i = 0; i < m_geometry.sectorsPerTrack(); i++) {
            quint8 duplicate = m_proSectorInfo[slot + i].totalDuplicate;
            m_proSectorInfo[slot + i].used = false;
            m_proSectorInfo[slot + i].wd1771Status = 0xEF; // no sector
            m_proSectorInfo[slot + i].totalDuplicate = 0;
            m_proSectorInfo[slot + i].notEmpty = false;
            m_proSectorInfo[slot + i].fillByte = 0;
            m_proSectorInfo[slot + i].weakBits = (quint16)0xFFFF;
            m_proSectorInfo[slot + i].sectorTiming = (5 + (i & 1));
            m_proSectorInfo[slot + i].lastSectorRead = 0;
            for (quint8 phantomIndex = 0; phantomIndex < duplicate; phantomIndex++) {
                quint16 phantomSlot = 1040 + (quint8)m_proSectorInfo[slot + i].duplicateOffset[phantomIndex] - 1;
                m_proSectorInfo[phantomSlot].used = false;
                m_proSectorInfo[phantomSlot].firstPass = false;
                m_proSectorInfo[phantomSlot].sectorData.resize(0);
                m_proSectorInfo[phantomSlot].sectorNumber = 0xFF;
                m_proSectorInfo[phantomSlot].absoluteSector = 0xFFFF;
            }
        }
        m_trackNumber = track;
        m_sectorsInTrack = 0;

        // fill CHIP ram
        m_board.m_chipRam[0] = data[0] > 28 ? 28 : data[0];
        m_sectorsInTrack = m_board.m_chipRam[0];
        for (quint8 index = 1; index <= m_board.m_chipRam[0]; index++) {
            quint8 sector = data[index];
            if (!useSectorList) {
                // compute sector number for the sector index
                int sectorIndex = ((((index - 1) % m_geometry.sectorsPerTrack()) + 1) << 1) - 1;
                if (index > (m_geometry.sectorsPerTrack() >> 1)) {
                    sectorIndex -= m_geometry.sectorsPerTrack() - 1;
                }
                sector = sectorIndex;
            }
            m_board.m_chipRam[index] = sector;

            // fill corresponding slot
            if ((sector > 0) && (sector <= m_geometry.sectorsPerTrack())) {
                quint16 sectorSlot = (track * m_geometry.sectorsPerTrack()) + sector - 1;
                if (m_proSectorInfo[sectorSlot].used) {
                    quint8 duplicates = m_proSectorInfo[sectorSlot].totalDuplicate;
                    quint16 phantomSlot = 0;
                    if (duplicates < 5) {
                        // find an empty phantom slot
                        for (quint16 j = 0; j < 255; j++) {
                            if (!m_proSectorInfo[1040 + j].used) {
                                phantomSlot = 1040 + j;
                                break;
                            }
                        }
                        if (phantomSlot == 0) {
                            qWarning() << "!w" << tr("[%1] More than 255 phantom sectors (unsupported with PRO format)")
                                          .arg(deviceName());
                            return false;
                        }
                        m_proSectorInfo[sectorSlot].duplicateOffset[duplicates] = phantomSlot - 1040 + 1;
                        m_proSectorInfo[sectorSlot].totalDuplicate = ++duplicates;
                        m_proSectorInfo[phantomSlot].used = true;
                        m_proSectorInfo[phantomSlot].sectorNumber = m_proSectorInfo[sectorSlot].sectorNumber;
                        m_proSectorInfo[phantomSlot].absoluteSector = m_proSectorInfo[sectorSlot].absoluteSector;
                        m_proSectorInfo[phantomSlot].totalDuplicate = 0;
                        m_proSectorInfo[phantomSlot].weakBits = (quint16)0xFFFF;
                        m_proSectorInfo[phantomSlot].lastSectorRead = 0;
                        sectorSlot = phantomSlot;
                    }
                    else {
                        qWarning() << "!w" << tr("[%1] More than 6 phantom sectors for a given sector number (unsupported with PRO format)")
                                      .arg(deviceName());
                        return false;
                    }
                }
                else {
                    m_proSectorInfo[sectorSlot].used = true;
                    m_proSectorInfo[sectorSlot].sectorNumber = sector;
                    m_proSectorInfo[sectorSlot].absoluteSector = (track * m_geometry.sectorsPerTrack()) + sector;
                    m_proSectorInfo[sectorSlot].totalDuplicate = 0;
                }
                m_proSectorInfo[sectorSlot].driveStatus = 0x10;
                m_proSectorInfo[sectorSlot].wd1771Status = 0xFF; // no error
                m_proSectorInfo[sectorSlot].sectorTiming = (5 + (index & 1));
                quint8 dataSize = data[85 + index - 1];
                quint8 fillByte = data[57 + index - 1];
                if (dataSize != (quint8)128) {
                    m_proSectorInfo[sectorSlot].wd1771Status = 0xF7; // CRC error
                    if (dataSize < (quint8)0x80) {
                        m_proSectorInfo[sectorSlot].sectorTiming = 1 + (dataSize >> 5);
                    }
                }
                else if ((fillByte == (0xFF - DISK_ID_ADDR_MARK)) || (fillByte >= 0x04 && fillByte <= 0x08)) {
                    m_proSectorInfo[sectorSlot].wd1771Status = 0xF7; // CRC error because these fill bytes are interpreted by FDC
                }

                // fill track content
                m_trackContent[m_sectorsInTrack++] = sectorSlot;

                // fill sector data
                m_proSectorInfo[sectorSlot].sectorData.resize(128);
                if (fillByte == 0x08) { // fill with CRC
                    dataSize = ((dataSize * 3) >> 1) + 2; // CRC takes more place to write
                    if (dataSize > (quint8)128) {
                        dataSize = (quint8)128;
                    }
                    m_proSectorInfo[sectorSlot].sectorData[0] = 0x40;
                    m_proSectorInfo[sectorSlot].sectorData[1] = 0x7B;
                    int patternIndex = 0;
                    for (quint8 j = 2; j < dataSize; j++) {
                        m_proSectorInfo[sectorSlot].sectorData[j] = FDC_CRC_PATTERN[patternIndex++];
                        if (patternIndex >= 3) {
                            patternIndex = 0;
                        }
                    }
                }
                else {
                    if (dataSize > (quint8)128) {
                        dataSize = (quint8)128;
                    }
                    Crc16 crc16;
                    crc16.Reset();
                    crc16.Add((unsigned char) DISK_DATA_ADDR_MARK4);
                    for (quint16 j = 0; j < dataSize; j++) {
                        crc16.Add((unsigned char) (0xFF - fillByte));
                        m_proSectorInfo[sectorSlot].sectorData[j] = fillByte;
                    }
                    if (dataSize < (quint8)128) {
                        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
                    }
                    if (dataSize < (quint8)128) {
                        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
                    }
                }

                // if this sector is a short sector, we need to compute what will appear in the remaining bytes (up to 128)
                // The gap between sector followed by the start of the next sector will appear inside this sector (overlap).
                for (quint8 nextIndex = index; (dataSize < (quint8)128) && (nextIndex < index + 10); nextIndex++) {
                    quint8 nextSector = nextIndex < m_board.m_chipRam[0] ? data[nextIndex + 1] : data[1 + nextIndex - m_board.m_chipRam[0]];
                    quint8 nextFillByte = nextIndex < m_board.m_chipRam[0] ? data[57 + nextIndex] : data[57 + nextIndex - m_board.m_chipRam[0]];
                    if (!useSectorList) {
                        // compute sector number for the sector index
                        int sectorIndex = (((nextIndex % m_geometry.sectorsPerTrack()) + 1) << 1) - 1;
                        if (nextIndex > (m_geometry.sectorsPerTrack() >> 1)) {
                            sectorIndex -= m_geometry.sectorsPerTrack() - 1;
                        }
                        nextSector = sectorIndex;
                    }
                    dataSize = writeProSectorHeader(dataSize, sectorSlot, postDataCrc, preIDField, postIDCrc, track, nextIndex, nextSector);
                    if (dataSize < (quint8)128) {
                        Crc16 crc16;
                        crc16.Reset();
                        crc16.Add((unsigned char) DISK_DATA_ADDR_MARK4);
                        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF - DISK_DATA_ADDR_MARK4;
                        quint8 nextDataSize = nextIndex < m_board.m_chipRam[0] ? data[85 + nextIndex] : data[85 + nextIndex - m_board.m_chipRam[0]];
                        if (nextFillByte == 0x08) { // fill with CRC
                            nextDataSize = ((nextDataSize * 3) >> 1); // CRC takes more place to write
                            if (nextDataSize > (quint8)128) {
                                nextDataSize = (quint8)128;
                            }
                            if (dataSize < (quint8)128) {
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0x40;
                            }
                            if (dataSize < (quint8)128) {
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0x7B;
                            }
                            int patternIndex = 0;
                            while ((dataSize < (quint8)128) && (nextDataSize-- > 0)) {
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = FDC_CRC_PATTERN[patternIndex++];
                                if (patternIndex >= 3) {
                                    patternIndex = 0;
                                }
                            }
                        }
                        else {
                            if (nextDataSize > (quint8)128) {
                                nextDataSize = (quint8)128;
                            }
                            for (quint8 j = 0; (dataSize < (quint8)128) && (j < nextDataSize); j++) {
                                crc16.Add((unsigned char) (0xFF - nextFillByte));
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = nextFillByte;
                            }
                            if (dataSize < (quint8)128) {
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
                            }
                            if (dataSize < (quint8)128) {
                                m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
                            }
                        }
                    }
                }
                m_proSectorInfo[sectorSlot].notEmpty = false;
                quint8 value = m_proSectorInfo[sectorSlot].sectorData[0];
                for (quint8 j = 0; j < (quint8)128; j++) {
                    if ((quint8) m_proSectorInfo[sectorSlot].sectorData[j] != 0) {
                        m_proSectorInfo[sectorSlot].notEmpty = true;
                    }
                    if ((quint8) m_proSectorInfo[sectorSlot].sectorData[j] != value) {
                        value = 1;
                    }
                }
                m_proSectorInfo[sectorSlot].fillByte = value;
            }
        }
    }
    return true;
}

quint8 SimpleDiskImage::writeProSectorHeader(quint8 dataSize, quint16 sectorSlot, quint8 postDataCrc, quint8 preIDField, quint8 postIDCrc,
                                             quint8 track, quint8 index, quint8 nextSector) {
    for (quint8 j = 0; j < postDataCrc && dataSize < (quint8)128; j++) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF;
    }
#ifdef CHIP810
    // these 3 zero bytes are written by CHIP 810 but not by Super Archiver 1050.
    for (quint8 j = 0; j < 3 && dataSize < (quint8)128; j++) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0;
    }
#endif
    for (quint8 j = 0; j < preIDField && dataSize < (quint8)128; j++) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF;
    }
    Crc16 crc16;
    crc16.Reset();
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) DISK_ID_ADDR_MARK);
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (0xFF - DISK_ID_ADDR_MARK);
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) (track & 0xFF));
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)(0xFF - (track & 0xFF));
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) (index << 2));
#ifdef CHIP_810
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF - (quint8)(index << 2);
#else
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF;
#endif
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) (nextSector));
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF - nextSector;
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) 0x00);
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF;
    }
    if (dataSize < (quint8)128) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
    }
    if (dataSize < (quint8)128) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
    }
    for (quint8 j = 0; j < postIDCrc && dataSize < (quint8)128; j++) {
        m_proSectorInfo[sectorSlot].sectorData[dataSize++] = 0xFF;
    }
    return dataSize;
}

bool SimpleDiskImage::writeProTrackWithSkew(quint16 aux, const QByteArray &) {
    // skew alignment not supported
    return writeProTrack((aux & 0x3F) | 0xF000, m_board.m_trackData);
}

bool SimpleDiskImage::writeProSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy)
{
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    quint16 index = aux & 0x1F;
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInProTrack(nbSectors, mapping)) {
        qWarning() << "!w" << tr("[%1] sector layout does not map to track layout")
                      .arg(deviceName());
        return false;
    }
    int indexInTrack = (int)(quint32)mapping[index];

    // check that the sector number in the track matches the sector number in the sector list
    quint16 indexInProSector = m_trackContent[indexInTrack];
    quint8 sectorNumber = m_proSectorInfo[indexInProSector].sectorNumber;
    if ((quint8)m_board.m_chipRam[index + 1] != sectorNumber) {
        qWarning() << "!w" << tr("[%1] sector %2 does not match sector number at index %3")
                      .arg(deviceName())
                      .arg(sectorNumber)
                      .arg(index);
        return false;
    }
    quint16 slot = m_proSectorInfo[indexInProSector].absoluteSector;

    // compute sector status and sector length
    // convert CHIP flags 7,6,5 into WD1771 flags 6,5,3
    quint8 sectorLength = 128;
    m_wd1771Status = 0xFF;
    m_proSectorInfo[slot].sectorTiming = 5;
    quint8 badSectorType = chipFlags & 0x60;
    if (chipFlags & 0x80) {
        badSectorType |= 0x08;
    }
    if (badSectorType != 0) {
        m_proSectorInfo[slot].wd1771Status = 0xFF & ~badSectorType;
        if (fuzzy) {
            qWarning() << "!w" << tr("[%1] Fuzzy sector among phantom sectors (unsupported with PRO format)")
                          .arg(deviceName());
        }
        else if ((chipFlags & 0x10) == 0) {
            sectorLength = data[127];
            m_proSectorInfo[slot].sectorTiming = 2;
            qDebug() << "!u" << tr("[%1] Short sector: %2 bytes")
                        .arg(deviceName())
                        .arg(sectorLength);
        }
        else {
            qDebug() << "!u" << tr("[%1] CRC error (type $%2)")
                        .arg(deviceName())
                        .arg(badSectorType, 2, 16, QChar('0'));
        }
    }

    m_proSectorInfo[slot].driveStatus = 0x10;
    m_proSectorInfo[slot].wd1771Status = m_wd1771Status;
    m_proSectorInfo[slot].weakBits = (quint16)0xFFFF;

    // write sector
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    m_proSectorInfo[slot].notEmpty = false;
    quint8 value = data[0];
    for (quint8 i = 0; i < sectorLength; i++) {
        m_proSectorInfo[slot].sectorData[i] = data[i];
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != 0) {
            m_proSectorInfo[slot].notEmpty = true;
        }
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != value) {
            value = 1;
        }
    }
    m_proSectorInfo[slot].fillByte = value;
    m_lastSector = m_proSectorInfo[slot].absoluteSector;
    return true;
}

bool SimpleDiskImage::writeFuzzyProSector(quint16 aux, const QByteArray &data)
{
	// Fuzzy sectors are simulated by writing 6 sectors (one main and 5 duplicates) with different content at the specified offset
	quint8 weakOffset = data[m_geometry.bytesPerSector() - 2];
    quint16 chipFlags = (aux >> 8) & 0xFC;
    quint16 sector = aux & 0x3FF;

    // check if there are phantom sectors for this sector number
    quint16 slot = sector - 1;
    if (!m_proSectorInfo[slot].used) {
        qCritical() << "!e" << tr("[%1] Sector %2 does not exist in PRO file")
                       .arg(deviceName())
                       .arg(sector);
        m_driveStatus = 0x10;
        m_wd1771Status = 0xEF;
        return false;
    }

	// The fuzzy sector command never overwrites the last 3 bytes of the sector. They remain untouched and not fuzzed.
    quint8 lastBytes[3];
    if ((m_proSectorInfo[slot].wd1771Status & 0x10) == 0) {
		lastBytes[0] = data[125];
		lastBytes[1] = data[126];
		lastBytes[2] = data[127];
	}
	else {
        lastBytes[0] = m_proSectorInfo[slot].sectorData[125];
        lastBytes[1] = m_proSectorInfo[slot].sectorData[126];
        lastBytes[2] = m_proSectorInfo[slot].sectorData[127];
	}

	// is there enough duplicate sector slots to simulate a fuzzy sector
	int nbFreeDuplicateSlots = 0;
	for (int i = 1; (i < 256) && (nbFreeDuplicateSlots < 5); i++) {
		if (!m_proSectorInfo[1040 + i - 1].used) {
			nbFreeDuplicateSlots++;
		}
	}
	if (nbFreeDuplicateSlots < 5) {
		qWarning() << "!w" << tr("[%1] Sector can not be fuzzed because all duplicate sector slots are already used").arg(deviceName());
		return writeProSector(aux, data);
	}

	// change the flags of the main sector
	m_proSectorInfo[slot].totalDuplicate = 5;
    m_proSectorInfo[slot].weakBits = (quint16)weakOffset;
	m_proSectorInfo[slot].wd1771Status |= 0x10; // now we have data in the sector
	m_proSectorInfo[slot].wd1771Status &= ~0x08; // now we have a data CRC error
    m_proSectorInfo[slot].notEmpty = true;
    m_proSectorInfo[slot].fillByte = 1;

    // convert CHIP flags 7,6,5 into WD1771 flags 6,5,3
    quint8 badSectorType = chipFlags & 0x60;
    if (chipFlags & 0x80) {
        badSectorType |= 0x08;
    }
    if (badSectorType != 0) {
        m_proSectorInfo[slot].wd1771Status = 0xFF & ~badSectorType;
        if ((chipFlags & 0x10) == 0) {
            quint8 sectorLength = data[127];
            m_proSectorInfo[slot].sectorTiming = 2;
            qDebug() << "!u" << tr("[%1] Short sector: %2 bytes")
                        .arg(deviceName())
                        .arg(sectorLength);
        }
        else {
            qDebug() << "!u" << tr("[%1] CRC error (type $%2)")
                        .arg(deviceName())
                        .arg(badSectorType, 2, 16, QChar('0'));
        }
    }

	// write the main sector
    for (quint16 i = 0; i < m_geometry.bytesPerSector() - 3; i++) {
        m_proSectorInfo[slot].sectorData[i] = data[i];
    }
    for (unsigned int i = 0; i < sizeof(lastBytes); i++) {
        m_proSectorInfo[slot].sectorData[m_geometry.bytesPerSector() - 3 + i] = lastBytes[i];
	}
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }

	// write the 5 duplicate sectors with random data instead of the fuzzy bytes
	int phantomSlot = 1040;
	for (int duplicate = 0; duplicate < 5; duplicate++) {

		// find a free duplicate slot
		while (m_proSectorInfo[phantomSlot].used) {
			phantomSlot++;
            if (phantomSlot >= (1040 + 255)) {
				qCritical() << "!e" << tr("[%1] Empty duplicate sector slot not found in Pro file").arg(deviceName());
				return false;
			}
		}

		// set the flags of the duplicate sector
		m_proSectorInfo[phantomSlot].used = true;
		m_proSectorInfo[phantomSlot].totalDuplicate = 5;
        m_proSectorInfo[phantomSlot].weakBits = (quint16)weakOffset;
		m_proSectorInfo[phantomSlot].wd1771Status |= 0x10; // now we have data in the sector
		m_proSectorInfo[phantomSlot].wd1771Status &= ~0x08; // now we have a data CRC error
		m_proSectorInfo[phantomSlot].notEmpty = true;
		m_proSectorInfo[phantomSlot].fillByte = 1;

		// reference duplicate slot in main slot
		m_proSectorInfo[slot].duplicateOffset[duplicate] = phantomSlot - 1040 + 1;

		// write duplicate sector data
        for (quint16 i = 0; i < m_geometry.bytesPerSector(); i++) {
            if (i == weakOffset) {
                // make sure that the first weak byte is different in all duplicate sectors
                m_proSectorInfo[phantomSlot].sectorData[i] = data[i] + (0x03 << duplicate);
            }
            if ((i < weakOffset) || (i >= m_geometry.bytesPerSector() - 3)) {
				m_proSectorInfo[phantomSlot].sectorData[i] = m_proSectorInfo[slot].sectorData[i];
			}
			else {
				m_proSectorInfo[phantomSlot].sectorData[i] = qrand() & 0xFF;
			}
		}
	}
	return true;
}

bool SimpleDiskImage::writeProSector(quint16 aux, const QByteArray &data)
{
    // extract sector number and flags
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    quint16 sector = m_board.isChipOpen() ? aux & 0x3FF : aux;

    // check if there are phantom sectors for this sector number
    quint16 slot = sector - 1;
    if (!m_proSectorInfo[slot].used) {
        qCritical() << "!e" << tr("[%1] Sector %2 does not exist in PRO file")
                       .arg(deviceName())
                       .arg(sector);
        return false;
    }
    if (((quint8)m_proSectorInfo[slot].totalDuplicate != 0) && (chipFlags == 0)) {
        int dupnum = m_proSectorInfo[slot].lastSectorRead;
        qDebug() << "!u" << tr("[%1] Duplicate sector $%2: writing number %3").arg(deviceName()).arg(sector, 2, 16, QChar('0')).arg(dupnum);

        m_proSectorInfo[slot].lastSectorRead = (m_proSectorInfo[slot].lastSectorRead + 1) % ((quint8)m_proSectorInfo[slot].totalDuplicate + 1);
        if (dupnum != 0)  {
            slot = 1040 + (quint8)m_proSectorInfo[slot].duplicateOffset[dupnum - 1] - 1;
            if (!m_proSectorInfo[slot].used) {
                qCritical() << "!e" << tr("[%1] Sector %2 (phantom %3) does not exist in PRO file")
                               .arg(deviceName())
                               .arg(sector)
                               .arg(m_proSectorInfo[slot].duplicateOffset[dupnum]);
                return false;
            }
        }
    }

    // compute delay for the head to be on the right sector.
    // Assume standard interleave: 1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18
    int sectorInTrack = ((sector - 1) % m_geometry.sectorsPerTrack());
    int sectorIndex = sectorInTrack >> 1;
    if (sectorInTrack & 1) {
        sectorIndex += m_geometry.sectorsPerTrack() >> 1;
    }
    qint64 headDistance = ((unsigned long) sectorIndex * 208333L) / m_geometry.sectorsPerTrack();

    // compute sector status
    quint8 sectorLength = 128;
    m_proSectorInfo[slot].driveStatus = 0x10;
    m_proSectorInfo[slot].wd1771Status = 0xFF;
    m_proSectorInfo[slot].sectorTiming = 5;
    m_proSectorInfo[slot].weakBits = 0xFFFF;

    // convert CHIP flags 7,6,5 into WD1771 flags 6,5,3
    quint8 badSectorType = chipFlags & 0x60;
    if (chipFlags & 0x80) {
        badSectorType |= 0x08;
    }
    if (badSectorType != 0) {
        m_proSectorInfo[slot].wd1771Status = 0xFF & ~badSectorType;
        if ((chipFlags & 0x10) == 0) {
            sectorLength = data[127];
            m_proSectorInfo[slot].sectorTiming = 2;
            qDebug() << "!u" << tr("[%1] Short sector: %2 bytes")
                        .arg(deviceName())
                        .arg(sectorLength);
        }
        else {
            qDebug() << "!u" << tr("[%1] CRC error (type $%2)")
                        .arg(deviceName())
                        .arg(badSectorType, 2, 16, QChar('0'));
        }
    }

    // write sector
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    m_proSectorInfo[slot].notEmpty = false;
    quint8 value = data[0];
    for (quint8 i = 0; i < sectorLength; i++) {
        m_proSectorInfo[slot].sectorData[i] = data[i];
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != 0) {
            m_proSectorInfo[slot].notEmpty = true;
        }
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != value) {
            value = 1;
        }
    }
    m_proSectorInfo[slot].fillByte = value;
    m_lastDistance = (headDistance + 1) % 208333L;
    m_lastSector = sector;
    m_lastTime = QDateTime::currentMSecsSinceEpoch();
    return true;
}

// This method is responsible for transforming the weak sector offset into duplicate sectors with different data
bool SimpleDiskImage::writeProSectorExtended(int, quint8 dataType, quint8 trackNumber, quint8, quint8 sectorNumber, quint8, const QByteArray &data, bool crcError, int weakOffset)
{
    if ((sectorNumber < 1) || (sectorNumber > m_geometry.sectorsPerTrack())) {
		return true;
	}

    // find a free slot to store this new sector
    quint16 slot = (trackNumber * m_geometry.sectorsPerTrack()) + sectorNumber - 1;
    int phantomSlot = 1040;
    if (m_proSectorInfo[slot].used) {
		if ((quint8)m_proSectorInfo[slot].totalDuplicate < 5) {

            // find a free duplicate slot
            while (m_proSectorInfo[phantomSlot].used) {
                phantomSlot++;
                if (phantomSlot >= (1040 + 255)) {
                    qCritical() << "!e" << tr("[%1] Empty duplicate sector slot not found in Pro file").arg(deviceName());
                    return false;
                }
            }

            // reference duplicate slot in main slot
            m_proSectorInfo[slot].duplicateOffset[m_proSectorInfo[slot].totalDuplicate] = phantomSlot - 1040 + 1;
            m_proSectorInfo[slot].totalDuplicate++;
            slot = phantomSlot;
        }
		else {
            // only 5 duplicate sectors are allowed
			return true;
		}
    }
    else if (weakOffset != 0xFFFF) {

        // write the 5 duplicate sectors with random data instead of the fuzzy bytes
        for (int duplicate = 0; duplicate < 5; duplicate++) {

            // find a free duplicate slot
            while (m_proSectorInfo[phantomSlot].used) {
                phantomSlot++;
                if (phantomSlot >= (1040 + 255)) {
                    qCritical() << "!e" << tr("[%1] Empty duplicate sector slot not found in Pro file").arg(deviceName());
                    return false;
                }
            }

            // set the flags of the duplicate sector
            m_proSectorInfo[phantomSlot].used = true;
            m_proSectorInfo[phantomSlot].totalDuplicate = 5;
            m_proSectorInfo[phantomSlot].weakBits = (quint16)weakOffset;
            m_proSectorInfo[phantomSlot].wd1771Status |= 0x10; // now we have data in the sector
            m_proSectorInfo[phantomSlot].wd1771Status &= ~0x08; // now we have a data CRC error
            m_proSectorInfo[phantomSlot].notEmpty = true;
            m_proSectorInfo[phantomSlot].fillByte = 1;

            // reference duplicate slot in main slot
            m_proSectorInfo[slot].duplicateOffset[duplicate] = phantomSlot - 1040 + 1;

            // write duplicate sector data
            for (quint16 i = 0; i < m_geometry.bytesPerSector(); i++) {
                if (i == weakOffset) {
                    // make sure that the first weak byte is different in all duplicate sectors
                    m_proSectorInfo[phantomSlot].sectorData[i] = data[i] + (0x03 << duplicate);
                }
                else if ((i < weakOffset) || (i >= m_geometry.bytesPerSector() - 3)) {
                    m_proSectorInfo[phantomSlot].sectorData[i] = data[i];
                }
                else {
                    m_proSectorInfo[phantomSlot].sectorData[i] = qrand() & 0xFF;
                }
            }
        }
        m_proSectorInfo[slot].totalDuplicate = 5;
    }

    // compute sector status
	m_proSectorInfo[slot].used = true;
    m_proSectorInfo[slot].firstPass = true;
    m_proSectorInfo[slot].sectorNumber = sectorNumber;
    m_proSectorInfo[slot].absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sectorNumber;
    m_proSectorInfo[slot].driveStatus = 0x10;
    m_proSectorInfo[slot].wd1771Status = 0xFF;
    if ((crcError) || (weakOffset != 0xFFFF)) {
        m_proSectorInfo[slot].wd1771Status &= ~0x08;
    }
    if ((dataType & 0x01) == 0) {
        m_proSectorInfo[slot].wd1771Status &= ~0x20;
    }
    m_proSectorInfo[slot].sectorTiming = 5;
    m_proSectorInfo[slot].weakBits = (quint16)weakOffset;
    m_proSectorInfo[slot].lastSectorRead = (quint8)0;

    // write sector
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    m_proSectorInfo[slot].notEmpty = false;
    m_proSectorInfo[slot].sectorData.resize(data.size());
    quint8 value = data[0];
    for (int i = 0; i < data.size(); i++) {
        m_proSectorInfo[slot].sectorData[i] = data[i];
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != 0) {
            m_proSectorInfo[slot].notEmpty = true;
        }
        if ((quint8) m_proSectorInfo[slot].sectorData[i] != value) {
            value = 1;
        }
    }
    m_proSectorInfo[slot].fillByte = value;
    return true;
}

bool SimpleDiskImage::fillProSectorInfo(const QString &fileName, QFile *sourceFile, quint16 slot, quint16 absoluteSector)
{
    QByteArray sectorHeader = sourceFile->read(12);
    if ((sectorHeader.length() != 12) || sourceFile->atEnd()) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot read from file: %1.").arg(sourceFile->errorString()));
        return false;
    }

    // save header data into a slot
    m_proSectorInfo[slot].used = true;
    m_proSectorInfo[slot].firstPass = true;
    m_proSectorInfo[slot].sectorNumber = absoluteSector != 0xFFFF ? (quint8)(slot % m_geometry.sectorsPerTrack()) + 1 : (quint8)0xFF;
    m_proSectorInfo[slot].absoluteSector = absoluteSector;
    m_proSectorInfo[slot].driveStatus = sectorHeader[0];
    m_proSectorInfo[slot].wd1771Status = sectorHeader[1];
    m_proSectorInfo[slot].reservedByte = sectorHeader[4];
    m_proSectorInfo[slot].totalDuplicate = sectorHeader[5];
    m_proSectorInfo[slot].sectorTiming = (quint8)5;
    m_proSectorInfo[slot].shortSectorSize = (quint8)0;
    m_proSectorInfo[slot].lastSectorRead = (quint8)0;
    m_proSectorInfo[slot].weakBits = (quint16)0xFFFF;
    if (((quint8)m_proSectorInfo[slot].wd1771Status & 0x10) != 0) {
        if (m_proSectorInfo[slot].totalDuplicate > 5) {
            qCritical() << "!e" << tr("[%1] Invalid number of phantom sectors (%2) for sector %3")
                           .arg(deviceName())
                           .arg(m_proSectorInfo[slot].totalDuplicate)
                           .arg(absoluteSector);
            return false;
        }
        for (int j = 0; j < m_proSectorInfo[slot].totalDuplicate; j++) {
            m_proSectorInfo[slot].duplicateOffset[j] = (quint8) sectorHeader[7 + j];
        }
    }

    // read sector data
    int bufsize = 128;
    m_proSectorInfo[slot].sectorData = sourceFile->read(bufsize);
    if (m_proSectorInfo[slot].sectorData.length() != bufsize) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot read from file: %1.").arg(sourceFile->errorString()));
        return false;
    }

    // check if sector is empty
    m_proSectorInfo[slot].notEmpty = false;
    quint8 value = m_proSectorInfo[slot].sectorData[0];
    for (int j = 0; j < 128; j++) {
        if ((quint8) m_proSectorInfo[slot].sectorData[j] != 0) {
            m_proSectorInfo[slot].notEmpty = true;
        }
        if ((quint8) m_proSectorInfo[slot].sectorData[j] != value) {
            value = 1;
        }
    }
    m_proSectorInfo[slot].fillByte = value;
    return true;
}

void SimpleDiskImage::guessWeakSectorInPro(int slot)
{
    // check if we have a weak sector. The following (arbitrary) conditions are verified
    // - at least 4 alternate sectors
    // - all sectors have a CRC error
    // - the first changing byte is at the same offset (minus or plus one) in each sector
    // - the sectors are not empty
    m_proSectorInfo[slot].weakBits = 0xFFFF;
    if ((m_proSectorInfo[slot].totalDuplicate >= 5) && (m_proSectorInfo[slot].wd1771Status != 0xFF) && (m_proSectorInfo[slot].notEmpty)) {
        bool hasWeakBits = true;
        quint16 weakBits = 9999;
        quint16 diffOffset[6];
        for (int j = 0; (j < m_proSectorInfo[slot].totalDuplicate) && (j < 5); j++) {

            // check that the sector has an error
            quint16 phantomSlot = 1040 + (quint16)m_proSectorInfo[slot].duplicateOffset[j] - 1;
            if (!m_proSectorInfo[slot].used) {
                qCritical() << "!e" << tr("[%1] Sector %2 has an invalid phantom index %3.")
                               .arg(deviceName())
                               .arg(m_proSectorInfo[slot].absoluteSector)
                               .arg(m_proSectorInfo[slot].duplicateOffset[j]);
                hasWeakBits = false;
                break;
            }
            if ((m_proSectorInfo[phantomSlot].wd1771Status == 0xFF) || (! m_proSectorInfo[phantomSlot].notEmpty)) {
                hasWeakBits = false;
                break;
            }

            // compare bytes with main sector
            diffOffset[j] = 0;
            for (quint16 k = 0; k < m_proSectorInfo[phantomSlot].sectorData.size(); k++) {
                if (m_proSectorInfo[slot].sectorData[k] != m_proSectorInfo[phantomSlot].sectorData[k]) {
                    break;
                }
                diffOffset[j]++;
            }
            if (diffOffset[j] < weakBits) {
                weakBits = diffOffset[j];
            }
        }
        // check if changing byte is always at the same offset plus or minus one
        for (int j = 0; (j < m_proSectorInfo[slot].totalDuplicate) && (j < 5); j++) {
            if (abs(weakBits - diffOffset[j]) > 1) {
                hasWeakBits = false;
                break;
            }
        }
        if ((hasWeakBits) && (weakBits < m_proSectorInfo[slot].sectorData.size())) {
            m_proSectorInfo[slot].weakBits = weakBits;
        }
    }
}

quint16 SimpleDiskImage::findPositionInProTrack(int track, int indexInProSector, bool withoutData) {
    m_proSectorInfo[indexInProSector].shortSectorSize = 0;
    if (m_proSectorInfo[indexInProSector].weakBits == 0xFFFF) {
        quint8 invertedTrack = (0xFF - (quint8)track) & 0xFF;
        for (int l = 0; l < 128 - 8; l++) {
            if (((quint8)m_proSectorInfo[indexInProSector].sectorData[l] == 0xFF)
                    && ((quint8)m_proSectorInfo[indexInProSector].sectorData[l + 1] == (0xFF - DISK_ID_ADDR_MARK))
                    && ((quint8)m_proSectorInfo[indexInProSector].sectorData[l + 2] == invertedTrack)) {
                Crc16 crc16;
                crc16.Reset();
                for (int m = 0; m < 5; m++) {
                    crc16.Add((unsigned char) ((0xFF - m_proSectorInfo[indexInProSector].sectorData[l + 1 + m]) & 0xFF));
                }
                unsigned short readCrc = 0xFFFF - ((((unsigned short)m_proSectorInfo[indexInProSector].sectorData[l + 6]) << 8) | (((unsigned short)m_proSectorInfo[indexInProSector].sectorData[l + 7]) & 0xFF));
                //qDebug() << "!e" << tr("[%1] computed crc=%2 found crc=%3 for slot %4").arg(deviceName()).arg(readCrc, 4, 16, QChar('0')).arg(crc16.GetCrc(), 4, 16, QChar('0')).arg(indexInProSector);
                if (readCrc == crc16.GetCrc()) {

                    // check if this sector exists
                    quint8 sectorNumber = (0xFF - m_proSectorInfo[indexInProSector].sectorData[l + 4]) & 0xFF;
                    quint16 mainSlot = (track * m_geometry.sectorsPerTrack()) + (quint16)sectorNumber - 1;
                    if (m_proSectorInfo[mainSlot].used) {
                        // we have found a header with a valid CRC. Try to find a DATA address mark with at least 6 zero bytes between ID and DATA address mark
                        int nextData = 0;
                        int index = l + 8;
                        int nbFF = 0;
                        while (index < 128) {
                            if (m_proSectorInfo[indexInProSector].sectorData[index] == (char)(0xFF - 0x00)) {
                                nbFF++;
                            }
                            else if ((nbFF >= 6) && ((m_proSectorInfo[indexInProSector].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK1)) ||
                                    (m_proSectorInfo[indexInProSector].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK2)) ||
                                    (m_proSectorInfo[indexInProSector].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK3)) ||
                                    (m_proSectorInfo[indexInProSector].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK4)))) {
                                nextData = index + 1;
                                break;
                            }
                            index++;
                        }

                        // try to find a sector in main slot or in phantom slots that is not already paired
                        if (nextData == 0) {
                            if (withoutData) {
                                if (! m_proSectorInfo[mainSlot].paired) {
                                    m_proSectorInfo[mainSlot].paired = true;
                                    m_proSectorInfo[indexInProSector].shortSectorSize = l + 1;
                                    return mainSlot;
                                }
                                else {
                                    quint8 totalDuplicate = m_proSectorInfo[mainSlot].totalDuplicate;
                                    if (totalDuplicate != 0) {
                                        for (int j = 0; j < totalDuplicate; j++) {
                                            quint16 phantomSlot = 1040 + m_proSectorInfo[mainSlot].duplicateOffset[j] - 1;
                                            if (! m_proSectorInfo[phantomSlot].paired) {
                                                m_proSectorInfo[phantomSlot].paired = true;
                                                m_proSectorInfo[indexInProSector].shortSectorSize = l + 1;
                                                return phantomSlot;
                                            }
                                        }
                                    }
                                }
                                continue;
                            }
                            else {
                                continue;
                            }
                        }

                        // try to find a sector in main slot or in phantom slots that best matches the data
                        m_proSectorInfo[indexInProSector].shortSectorSize = l + 1;
                        int bestDataMatch = 0;
                        quint16 bestSlot = 0xFFFF;
                        if (! m_proSectorInfo[mainSlot].paired) {
                            bestSlot = mainSlot;
                            for (int i = 0; i < 128 - nextData; i++) {
                                if (m_proSectorInfo[indexInProSector].sectorData[nextData + i] == m_proSectorInfo[mainSlot].sectorData[i]) {
                                    if (i >= bestDataMatch) {
                                        bestDataMatch = i + 1;
                                    }
                                }
                                else {
                                    break;
                                }
                            }
                        }
                        quint8 totalDuplicate = m_proSectorInfo[mainSlot].totalDuplicate;
                        if (totalDuplicate != 0) {
                            for (int j = 0; j < totalDuplicate; j++) {
                                quint16 phantomSlot = 1040 + m_proSectorInfo[mainSlot].duplicateOffset[j] - 1;
                                if (! m_proSectorInfo[phantomSlot].paired) {
                                    for (int i = 0; i < 128 - nextData; i++) {
                                        if (m_proSectorInfo[indexInProSector].sectorData[nextData + i] == m_proSectorInfo[phantomSlot].sectorData[i]) {
                                            if (i >= bestDataMatch) {
                                                bestDataMatch = i + 1;
                                                bestSlot = phantomSlot;
                                            }
                                        }
                                        else {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if (bestSlot != 0xFFFF) {
                            m_proSectorInfo[bestSlot].paired = true;
                        }
                        return bestSlot;
                    }
                }
            }
        }
    }
    return 0xFFFF;
}

bool SimpleDiskImage::findMappingInProTrack(int nbSectors, QByteArray &mapping)
{
    if ((nbSectors == 0) || (m_sectorsInTrack == 0)) {
        return -1;
    }
    for (int sectorStartIndex = 0; sectorStartIndex < nbSectors; sectorStartIndex++) {
        for (int currentIndexInTrack = 0; currentIndexInTrack < m_sectorsInTrack; currentIndexInTrack++) {
            bool match = true;
            int indexInTrack = currentIndexInTrack;
            for (int sectorIndex = 0; sectorIndex < nbSectors; sectorIndex++) {
                int indexInRam = (sectorStartIndex + sectorIndex) % nbSectors;
                quint16 indexInProSector = m_trackContent[indexInTrack];
                if (!m_proSectorInfo[indexInProSector].used) {
                    match = false;
                    break;
                }
                quint8 sectorNumber = m_proSectorInfo[indexInProSector].sectorNumber;
                if (m_board.m_chipRam[indexInRam + 1] != sectorNumber) {
                    match = false;
                    break;
                }
                indexInTrack = (indexInTrack + 1) % m_sectorsInTrack;
            }
            if (match) {
                for (int sectorIndex = 0; sectorIndex < nbSectors; sectorIndex++) {
                    int indexInRam = (sectorStartIndex + sectorIndex) % nbSectors;
                    mapping[indexInRam] = currentIndexInTrack;
                    currentIndexInTrack = (currentIndexInTrack + 1) % m_sectorsInTrack;
                }
                return true;
            }
        }
    }
    return false;
}

int SimpleDiskImage::sectorsInCurrentProTrack()
{
    if (m_trackNumber <= 39) {
        return m_sectorsInTrack;
    }
    return 0;
}
