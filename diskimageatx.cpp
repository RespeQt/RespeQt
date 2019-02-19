//
// diskimageatx class
// (c) 2018 Eric BACHER
//

#include "diskimage.h"
#include "zlib.h"

#include <QFileInfo>
#include "respeqtsettings.h"
#include <QDir>
#include "atarifilesystem.h"
#include "diskeditdialog.h"

#include <QtDebug>

extern quint8 FDC_CRC_PATTERN[];

// sector position in track for single and enhanced density
quint16 ATX_SECTOR_POSITIONS_SD[] = { 810, 2165, 3519, 4866, 6222, 7575, 8920, 10281, 11632, 12984, 14336, 15684, 17028, 20115, 21456, 22806, 24160, 25500 };
quint16 ATX_SECTOR_POSITIONS_ED[] = { 540, 1443, 2346, 3244, 4148, 5050, 5946, 6854, 7754, 8656, 9557, 10456, 11352, 13410, 14304, 15204, 16106, 17000, 17898, 18796, 19694, 20592, 21490, 22388, 23286, 24184 };

bool SimpleDiskImage::openAtx(const QString &fileName)
{
    QFile *sourceFile;

    if (m_originalImageType == FileTypes::Atx) {
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
    header = sourceFile->read(48);
    if (header.size() != 48) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate the magic number
    QByteArray magic = QByteArray(header.data(), 4);
    if (magic != "AT8X") {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Not a valid ATX file."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // read each track
    int density = (int) (quint8) header[18];
    if (m_displayTrackLayout) {
        QString densityStr;
        switch (density) {
        case 0:
            densityStr = tr("Single");
            break;
        case 1:
            densityStr = tr("Enhanced");
            break;
        case 2:
            densityStr = tr("Double");
            break;
        default:
            densityStr = tr("Unknown (%1)").arg(density);
            break;
        }
        qDebug() << "!n" << tr("Track layout for %1. Density is %2").arg(fileName).arg(densityStr);
    }
    qint64 nextPos = (qint64) getLittleIndianLong(header, 28);
    for (int track = 0; track < 40; track++) {
        m_atxTrackInfo[track].clear();

        // get information about track
        quint64 pos = nextPos;
        if (!sourceFile->seek(pos)) {
            qCritical() << "!e" << tr("[%1] Cannot seek to track header #%2: %3").arg(deviceName()).arg(track).arg(sourceFile->error());
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        QByteArray trackHeader = sourceFile->read(32);
        if (trackHeader.size() != 32) {
            qCritical() << "!e" << tr("[%1] Track #%2 header could not be read").arg(deviceName()).arg(track);
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        if (trackHeader[4] != (char) 0) {
            qCritical() << "!e" << tr("[%1] Track header #%2 has an unknown type $%3").arg(deviceName()).arg(track).arg(trackHeader[4], 2, 16, QChar('0'));
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        int sectorCount = (int) (quint8) trackHeader[10];

        // save next track pointer
        quint32 size = getLittleIndianLong(trackHeader, 0);
        nextPos += size;

        // get information about sectors
        quint32 offsetSectorList = getLittleIndianLong(trackHeader, 20);
        quint64 currentTrackOffset = pos + offsetSectorList;
        if (!sourceFile->seek(currentTrackOffset)) {
            qCritical() << "!e" << tr("[%1] Cannot seek to sector list of track $%2: %3").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sourceFile->error());
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        quint64 sectorHeaderSize = 8 + (8 * sectorCount);
        QByteArray sectorList = sourceFile->read(sectorHeaderSize);
        if (sectorList.size() != (int) sectorHeaderSize) {
            qCritical() << "!e" << tr("[%1] Sector List header of track $%2 could not be read").arg(deviceName()).arg(track, 2, 16, QChar('0'));
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        if (sectorList[4] != (char) 0x01) {
            qCritical() << "!e" << tr("[%1] Sector List header of track $%2 has an unknown type $%3").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sectorList[4], 2, 16, QChar('0'));
            sourceFile->close();
            delete sourceFile;
            return false;
        }
        quint32 flags = getLittleIndianWord(trackHeader, 16);
        bool mfm = (flags & 0x02) ? true : false;

        // read each sector
        QByteArray secBuf;
        quint32 maxData = 0;
        int nbExtended = 0;
        for (int sector = 0; sector < sectorCount; sector++) {
            int ofs = 8 + (8 * sector);
            quint8 sectorNumber = (quint8) sectorList[ofs];
            quint8 sectorStatus = (quint8) sectorList[ofs + 1];
            if (secBuf.size() > 0) {
                secBuf.append(' ');
            }
            secBuf.append(tr("%1").arg(sectorNumber, 2, 16, QChar('0')));
            // display only one word (the most important) to keep the line as compact as possible
            if (sectorStatus & 0x40) {
                nbExtended++;
                secBuf.append("(WEAK)");
            }
            else if (sectorStatus & 0x20) {
                secBuf.append("(DEL)");
            }
            else if (sectorStatus & 0x10) {
                secBuf.append("(RNF)");
            }
            else if (sectorStatus & 0x08) {
                secBuf.append("(CRC)");
            }
            else if (m_atxTrackInfo[track].count(sectorNumber) > 0) {
                secBuf.append("(DUP)");
            }
            else if ((sectorStatus & 0x06) == 0x06) {
                secBuf.append("(LONG)");
            }
            quint16 sectorPosition = getLittleIndianWord(sectorList, ofs + 2);
            AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].add(sectorNumber, sectorStatus, sectorPosition);
            quint32 startData = getLittleIndianLong(sectorList, ofs + 4);
            if (startData > maxData) {
                maxData = startData;
            }
            if ((sectorStatus & 0x10) == 0) {
                quint64 absoluteOffset = pos + startData;
                if (!sourceFile->seek(absoluteOffset)) {
                    qCritical() << "!e" << tr("[%1] Cannot seek to sector data of track $%2, sector $%3: %4").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sectorInfo->sectorNumber(), 2, 16, QChar('0')).arg(sourceFile->error());
                    sourceFile->close();
                    delete sourceFile;
                    return false;
                }
                QByteArray data = sourceFile->read(128);
                if (data.size() != 128) {
                    qCritical() << "!e" << tr("[%1] Cannot read sector data of track $%2, sector $%3: %4").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sectorInfo->sectorNumber(), 2, 16, QChar('0')).arg(sourceFile->error());
                    sourceFile->close();
                    delete sourceFile;
                    return false;
                }
                sectorInfo->setSectorData(data);
                maxData += 128;
            }
        }
        if (m_displayTrackLayout) {
            qDebug() << "!u" << tr("track $%1 (%2): %3").arg(track, 2, 16, QChar('0')).arg(mfm ? "MFM" : "FM").arg(secBuf.constData());
        }

        // read extended attributes
        for (int sector = 0; sector < nbExtended; sector++) {
            quint64 currentSectorOffset = pos + maxData + (8 * sector);
            if (!sourceFile->seek(currentSectorOffset)) {
                qCritical() << "!e" << tr("[%1] Cannot seek to extended sector data of track $%2: %3").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sourceFile->error());
                sourceFile->close();
                delete sourceFile;
                return false;
            }
            QByteArray sectorData = sourceFile->read(8);
            if (sectorData.size() != 8) {
                qCritical() << "!e" << tr("[%1] Extended sector data of track $%2 could not be read").arg(deviceName()).arg(track, 2, 16, QChar('0'));
                sourceFile->close();
                delete sourceFile;
                return false;
            }
            if (sectorData[0] == (char) 0x08) {
                quint8 sectorIndex = (quint8) sectorData[5];
                if (sectorIndex >= m_atxTrackInfo[track].size()) {
                    qCritical() << "!e" << tr("[%1] Extended sector data of track $%2 references an out of bound sector %3").arg(deviceName()).arg(track, 2, 16, QChar('0')).arg(sectorIndex);
                    sourceFile->close();
                    delete sourceFile;
                    return false;
                }
                AtxSectorInfo *sector = m_atxTrackInfo[track].at(sectorIndex);
                quint16 sectorWeakOffet = getLittleIndianWord(sectorData, 6);
                if (sectorWeakOffet < 128) {
                    sector->setSectorWeakOffset(sectorWeakOffet);
                }
            }
        }
    }

    int size = (density == 2) ? 183936 : ((density == 1) ? 133120 : 92160);
    m_geometry.initialize(size);
    refreshNewGeometry();
    m_isReadOnly = sourceFile->isWritable();
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = false;
    m_lastSector = 0;
    m_lastDistance = 0;
    m_lastTime = 0;
    m_driveStatus = 0x10;
    m_wd1771Status = 0xFF;
    setReady(true);
    sourceFile->close();
    delete sourceFile;
    return true;
}

bool SimpleDiskImage::saveAtx(const QString &fileName)
{
    if (m_originalFileHeader.size() != 48) {
        m_originalFileHeader = QByteArray(48, 0);
    }

    m_originalFileHeader[0] = 'A';
    m_originalFileHeader[1] = 'T';
    m_originalFileHeader[2] = '8';
    m_originalFileHeader[3] = 'X';
    m_originalFileHeader[4] = 1;
    m_originalFileHeader[5] = 0;
    setLittleIndianLong(m_originalFileHeader, 28, (quint32)0x30);

    // Try to open the output file
    QFile *outputFile;

    if (m_originalImageType == FileTypes::Atx) {
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
    if (outputFile->write(m_originalFileHeader) != 48) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile->errorString());
        outputFile->close();
        delete outputFile;
        return false;
    }

    // save all tracks
    for (int track = 0; track < 40; track++) {
        QByteArray trackHeader(32, 0);

        // compute size of sector list
        int nbSectors = m_atxTrackInfo[track].size();
        int nbExtended = m_atxTrackInfo[track].numberOfExtendedSectors();
        int nbSectorsWithData = m_atxTrackInfo[track].numberOfSectorsWithData();
        int sectorListSize = 8 + (8 * nbSectors);
        int dataSizeChunkSize = 8;
        int emptyChunkSize = 8;
        int extendedSize = (8 * nbExtended);
        int sizeWithoutData = 32 + sectorListSize;
        int dataSize = 8 + (m_geometry.bytesPerSector() * nbSectorsWithData);
        int sizeWithData = sizeWithoutData + dataSize + extendedSize + emptyChunkSize;

        // fill track header
        setLittleIndianLong(trackHeader, 0, (quint32)sizeWithData);
        trackHeader[8] = (quint8)track;
        trackHeader[10] = (quint8)nbSectors;
        setLittleIndianLong(trackHeader, 20, (quint32)32);

        // Try to write the track header
        if (outputFile->write(trackHeader) != 32) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }

        QByteArray sectorList(sectorListSize, 0);
        setLittleIndianLong(sectorList, 0, (quint32)sectorListSize);
        sectorList[4] = (quint8)1;
        quint32 offset = sizeWithoutData + dataSizeChunkSize;
        for (int sector = 0; sector < nbSectors; sector++) {
            AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].at(sector);
            sectorList[8 + (8 * sector)] = sectorInfo->sectorNumber();
            quint8 status = sectorInfo->sectorStatus();
            if (sectorInfo->sectorWeakOffset() != 0xFFFF) {
                status |= 0x40;
            }
            sectorList[8 + (8 * sector) + 1] = status;
            setLittleIndianWord(sectorList, 8 + (8 * sector) + 2, sectorInfo->sectorPosition());
            if ((sectorInfo->sectorStatus() & 0x10) == 0) {
                setLittleIndianLong(sectorList, 8 + (8 * sector) + 4, offset);
                offset += (quint32)m_geometry.bytesPerSector();
            }
        }

        // Try to write the sector list
        if (outputFile->write(sectorList) != sectorListSize) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }

        // Try saving the chunk containing the data size
        QByteArray dataSizeChunk(dataSizeChunkSize, 0);
        setLittleIndianLong(dataSizeChunk, 0, dataSize);
        if (outputFile->write(dataSizeChunk) != dataSizeChunk.size()) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }

        // Try saving sector data
        for (int sector = 0; sector < nbSectors; sector++) {
            AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].at(sector);
            if ((sectorInfo->sectorStatus() & 0x10) == 0) {
                QByteArray data = sectorInfo->sectorData();
                if (outputFile->write(data, m_geometry.bytesPerSector()) != m_geometry.bytesPerSector()) {
                    qCritical() << "!e" << tr("Cannot save '%1': %2")
                                  .arg(fileName)
                                  .arg(outputFile->errorString());
                    outputFile->close();
                    delete outputFile;
                    return false;
                }
            }
        }

        // Try to write the extended data
        if (nbExtended > 0) {
            for (int sector = 0; sector < nbSectors; sector++) {
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].at(sector);
                if (sectorInfo->sectorWeakOffset() != 0xFFFF) {
                    QByteArray extendedData(8, 0);
                    extendedData[0] = (quint8)8;
                    extendedData[4] = (quint8)0x10;
                    extendedData[5] = (quint8)sector;
                    setLittleIndianWord(extendedData, 6, sectorInfo->sectorWeakOffset());
                    if (outputFile->write(extendedData) != 8) {
                        qCritical() << "!e" << tr("Cannot save '%1': %2")
                                      .arg(fileName)
                                      .arg(outputFile->errorString());
                        outputFile->close();
                        delete outputFile;
                        return false;
                    }
                }
            }
        }

        // Try saving an empty chunk
        QByteArray emptyChunk(emptyChunkSize, 0);
        if (outputFile->write(emptyChunk) != emptyChunk.size()) {
            qCritical() << "!e" << tr("Cannot save '%1': %2")
                          .arg(fileName)
                          .arg(outputFile->errorString());
            outputFile->close();
            delete outputFile;
            return false;
        }
    }

    quint64 pos = outputFile->pos();
    setLittleIndianLong(m_originalFileHeader, 32, (quint32)pos);
    outputFile->seek(0);

    // Try to overwrite the header
    if (outputFile->write(m_originalFileHeader) != 48) {
        qCritical() << "!e" << tr("Cannot save '%1': %2")
                      .arg(fileName)
                      .arg(outputFile->errorString());
        outputFile->close();
        delete outputFile;
        return false;
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

bool SimpleDiskImage::saveAsAtx(const QString &fileName, FileTypes::FileType destImageType)
{
    bool bareSectors = (m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz);
    if ((!bareSectors) && (m_originalImageType != FileTypes::Pro) && (m_originalImageType != FileTypes::ProGz)) {
        qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving Atx images from the current format is not supported yet."));
        return false;
    }

    if (bareSectors) {
        for (int track = 0; track < 40; track++) {
            m_atxTrackInfo[track].clear();
            for (quint8 index = 0; index < m_geometry.sectorsPerTrack(); index++) {

                // compute sector number for the sector index
                int sectorIndex = ((index + 1) << 1);
                if (index >= (m_geometry.sectorsPerTrack() >> 1)) {
                    sectorIndex -= m_geometry.sectorsPerTrack() - 1;
                }
                quint8 sector = sectorIndex - 1;

                // read sector and store it in Atx objects
                QByteArray data;
                int absoluteSector = (m_geometry.sectorsPerTrack() * track) + sector;
                readSector(absoluteSector, data);
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].add(sector, 0, m_geometry.isStandardED() ? ATX_SECTOR_POSITIONS_ED[index] : ATX_SECTOR_POSITIONS_SD[index]);
                sectorInfo->copySectorData(data);
            }
        }
    }
    else if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        for (int track = 0; track < 40; track++) {
            m_atxTrackInfo[track].clear();

            // sort sectors in track
            QByteArray dummy(m_geometry.bytesPerSector(), 0);
            readProTrack(track, dummy, 128);
            if (m_sectorsInTrack > 0) {
                int sectorDistance = 26048 / m_sectorsInTrack;
                for (int sector = 0; sector < m_sectorsInTrack; sector++) {

                    // compute a sector position.
                    // This is not accurate and could be improved a lot by looking at m_proSectorInfo[indexInPro].shortSectorSize
                    quint16 position = sector * sectorDistance;

                    // get sector and store it in Atx objects
                    quint16 indexInPro = m_trackContent[sector];
                    quint8 status = ~m_proSectorInfo[indexInPro].wd1771Status & 0x3E;
                    quint16 weakOffset = m_proSectorInfo[indexInPro].weakBits;
                    if (weakOffset != 0xFFFF) {
                        status |= 0x40;
                    }
                    AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].add(m_proSectorInfo[indexInPro].sectorNumber, status, position);
                    sectorInfo->setSectorWeakOffset(weakOffset);
                    sectorInfo->copySectorData(m_proSectorInfo[indexInPro].sectorData);
                }
            }
        }
    }

    // save the Atx objects in a file
    m_originalFileHeader.resize(0);
    m_originalImageType = destImageType;
    return saveAtx(fileName);
}

bool SimpleDiskImage::createAtx(int untitledName)
{
    m_geometry.initialize(false, 40, 18, 128);
    refreshNewGeometry();
    m_originalFileHeader.clear();
    m_isReadOnly = false;
    m_originalFileName = tr("Untitled image %1").arg(untitledName);
    m_originalImageType = FileTypes::Atx;
    m_isModified = false;
    m_isUnmodifiable = false;
    m_isUnnamed = true;

    // initialize the array containing the sector headers
    for (int track = 0; track < 40; track++) {
        m_atxTrackInfo[track].clear();
        for (quint8 sector = 0; sector <= (sizeof(ATX_SECTOR_POSITIONS_SD) / sizeof(quint16)); sector++) {
            m_atxTrackInfo[track].add(sector + 1, 0, ATX_SECTOR_POSITIONS_SD[sector]);
        }
    }
    setReady(true);
    return true;
}

bool SimpleDiskImage::readHappyAtxSectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data)
{
    // find the first index to start with.
    if (afterSectorNumber != 0) {
        // try to find the index of the specified sector in the track
        index = 0;
        for (int i = 0; i < m_atxTrackInfo[trackNumber].size(); i++) {
            AtxSectorInfo *sectorInfo = m_atxTrackInfo[trackNumber].at(i);
            if ((sectorInfo != NULL) && (sectorInfo->sectorNumber() == afterSectorNumber)) {
                index = (i + 1) % m_atxTrackInfo[trackNumber].size();
                break;
            }
        }
    }

    // find the next sector matching the requested sector number
    m_wd1771Status = 0xEF;
    AtxSectorInfo *sectorInfo = NULL;
    for (int i = 0; i < m_atxTrackInfo[trackNumber].size(); i++) {
        int indexInTrack = index % m_atxTrackInfo[trackNumber].size();
        AtxSectorInfo *currentSectorInfo = m_atxTrackInfo[trackNumber].at(indexInTrack);
        if ((currentSectorInfo != NULL) && (currentSectorInfo->sectorNumber() == sectorNumber)) {
            sectorInfo = currentSectorInfo;
            m_wd1771Status = sectorInfo->wd1771Status();
            break;
        }
        else {
            index++;
        }
    }
    if ((sectorInfo == NULL) || ((sectorInfo->wd1771Status() & 0x10) == 0)) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found starting at index %4")
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
    QByteArray array = sectorInfo->sectorData();
    for (int i = 0; i < 128; i++) {
        data[i] = array[i];
    }
    return true;
}

bool SimpleDiskImage::readHappyAtxSkewAlignment(bool happy1050)
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
    quint8 previousSector = 0xFF - m_board.m_happyRam[0x3CA];
    AtxSectorInfo *previousSectorInfo = m_atxTrackInfo[previousTrack].find(previousSector, 0);
    if (previousSectorInfo == NULL) {
        qWarning() << "!w" << tr("[%1] Sector %2 ($%3) not found in track %4 ($%5)")
                      .arg(deviceName())
                      .arg(previousSector)
                      .arg(previousSector, 2, 16, QChar('0'))
                      .arg(previousTrack)
                      .arg(previousTrack, 2, 16, QChar('0'));
        return false;
    }
    quint8 currentSector = 0xFF - m_board.m_happyRam[0x3CC];
    AtxSectorInfo *currentSectorInfo = m_atxTrackInfo[currentTrack].find(currentSector, 0);
    if (currentSectorInfo == NULL) {
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
    int distance = 26042 - previousSectorInfo->sectorPosition() + currentSectorInfo->sectorPosition();
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
/*
    qWarning() << "!w" << tr("[%1] Track %2 previous %3 current %4 distance %5 gap $%6")
                  .arg(deviceName())
                  .arg(currentTrack)
                  .arg(previousSectorInfo->sectorPosition())
                  .arg(currentSectorInfo->sectorPosition())
                  .arg(distance)
                  .arg(gap, 4, 16, QChar('0'));
*/
    return true;
}

bool SimpleDiskImage::writeHappyAtxTrack(int trackNumber, bool happy1050)
{
    // reset track data
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    m_atxTrackInfo[trackNumber].clear();
    m_trackNumber = trackNumber;

    // browse track data
    quint16 sectorPosition = 100;
    int startOffset = happy1050 ? 0xD00 : 0x300;
    int offset = startOffset;
    quint8 invertedTrack = (quint8) (0xFF - trackNumber);
    while (offset < (startOffset + 0x100)) {
        quint8 code = m_board.m_happyRam[offset++];
        if (code == 0) {
            break;
        }
        if (code < 128) {
            if (((quint8)m_board.m_happyRam[offset] == invertedTrack) && ((quint8)m_board.m_happyRam[offset + 1] == (quint8)0xFF)
                    && ((quint8)m_board.m_happyRam[offset + 2] >= 0xED) && ((quint8)m_board.m_happyRam[offset + 2] != 0xFF) && ((quint8)m_board.m_happyRam[offset + 4] == 0x08)) {
                quint8 sector = 0xFF - (quint8)m_board.m_happyRam[offset + 2];
                quint8 normalSize = (quint8)m_board.m_happyRam[offset + 3] == (quint8)0xFF;

                // fill corresponding slot
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[trackNumber].add(sector, 0, sectorPosition << 3);
                QByteArray sectorData;
                sectorData.resize(128);
                sectorInfo->copySectorData(sectorData);
                if (normalSize) {
                    sectorInfo->setWd1771Status(0xFF);
                }
                else {
                    sectorInfo->setWd1771Status(0xF1);
                }
                sectorPosition += (quint16)(6 + 7 + 17);
                if (sectorPosition > (quint16)3600) {
                    qWarning() << "!w" << tr("[%1] Too many sectors in this track. Ignored.")
                                  .arg(deviceName());
                    return false;
                }
            }
            else if (((quint8)m_board.m_happyRam[offset] == (quint8)0xB7) && ((quint8)m_board.m_happyRam[offset + 2] >= (quint8)0x9B) && ((quint8)m_board.m_happyRam[offset + 3] == (quint8)0x4B)) {
                qWarning() << "!w" << tr("[%1] Special sync header at position %2. Ignored.")
                              .arg(deviceName())
                              .arg(sectorPosition);
                return false;
            }
            else {
                quint8 track = 0xFF - (quint8)m_board.m_happyRam[offset];
                quint8 sector = 0xFF - (quint8)m_board.m_happyRam[offset + 2];
                qWarning() << "!w" << tr("[%1] Header has out of range values: Track=$%2 Sector=$%3. Ignored.")
                              .arg(deviceName())
                              .arg(track, 2, 16, QChar('0'))
                              .arg(sector, 2, 16, QChar('0'));
            }
            offset += 5;
        }
        else {
            offset++;
            sectorPosition += (quint16)(0xFF - code) + 1;
        }
    }
    return true;
}

bool SimpleDiskImage::writeHappyAtxSectors(int trackNumber, int afterSectorNumber, bool happy1050)
{
    int startOffset = happy1050 ? 0xC80 : 0x280;
    int startData = startOffset + 128;
    bool sync = (quint8)m_board.m_happyRam[startOffset] == 0;
    // find the first index to start with.
    int position = 0;
    if (sync && (afterSectorNumber != 0)) {
        // try to find the index of the specified sector in the track
        for (int i = 0; i < m_atxTrackInfo[trackNumber].size(); i++) {
            AtxSectorInfo *sectorInfo = m_atxTrackInfo[trackNumber].at(i);
            if ((sectorInfo != NULL) && (sectorInfo->sectorNumber() == afterSectorNumber)) {
                position = sectorInfo->sectorPosition();
                break;
            }
        }
    }
    int lastIndex = (int) (quint8)m_board.m_happyRam[startOffset + 0x0F];
    bool twoPasses = (quint8)m_board.m_happyRam[startOffset + 0x69] == 0;
    int maxPass = twoPasses ? 2 : 1;
    for (int passNumber = 1; passNumber <= maxPass; passNumber++) {
        int index = 18 - passNumber;
        while (index >= lastIndex) {
            quint8 sectorNumber = 0xFF - (quint8)m_board.m_happyRam[startOffset + 0x38 + index];
            m_board.m_happyRam[startOffset + 0x14 + index] = 0xEF;
            if ((sectorNumber > 0) && (sectorNumber <= m_geometry.sectorsPerTrack())) {
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[trackNumber].find(sectorNumber, position);
                if (sectorInfo != NULL) {
                    quint8 writeCommand = m_board.m_happyRam[startOffset + 0x4A + index];
                    int dataOffset = startData + (index * 128);
                    quint8 dataMark = (writeCommand << 5) | 0x9F;
                    sectorInfo->copySectorData(m_board.m_happyRam.mid(dataOffset,128));
                    quint8 fdcStatus = sectorInfo->wd1771Status() & dataMark; // use the data mark given in the command
                    if (writeCommand & 0x08) { // non-IBM format generates a CRC error
                        fdcStatus &= ~0x08;
                    }
                    sectorInfo->setWd1771Status(fdcStatus);
                    m_board.m_happyRam[startOffset + 0x14 + index] = fdcStatus;
                    position = sectorInfo->sectorPosition();
                }
                else {
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

bool SimpleDiskImage::formatAtx(const DiskGeometry &geo)
{
    if (geo.bytesPerSector() != 128) {
        qCritical() << "!e" << tr("Can not format ATX image: %1")
                      .arg(tr("Sector size (%1) not supported (should be 128)").arg(geo.bytesPerSector()));
        return false;
    }
    if (geo.sectorCount() > 1040) {
        qCritical() << "!e" << tr("Can not format ATX image: %1")
                      .arg(tr("Number of sectors (%1) not supported (max 1040)").arg(geo.sectorCount()));
        return false;
    }

    // initialize the array containing the sector headers
    quint16 *sectorPositions = geo.sectorsPerTrack() == 26 ? ATX_SECTOR_POSITIONS_ED : ATX_SECTOR_POSITIONS_SD;
    for (int track = 0; track < 40; track++) {
        m_atxTrackInfo[track].clear();
        for (quint8 index = 0; index < geo.sectorsPerTrack(); index++) {

            // compute sector number for the sector index
            int sectorIndex = ((index + 1) << 1);
            if (index >= (m_geometry.sectorsPerTrack() >> 1)) {
                sectorIndex -= m_geometry.sectorsPerTrack() - 1;
            }
            quint8 sector = sectorIndex - 1;
            m_atxTrackInfo[track].add(sector, 0, sectorPositions[index]);
        }
    }
    m_geometry.initialize(geo);
    m_newGeometry.initialize(geo);
    m_originalFileHeader.clear();
    m_isModified = true;
    emit statusChanged(m_deviceNo);
    return true;
}

void SimpleDiskImage::readAtxTrack(quint16 aux, QByteArray &data, int length)
{
    quint16 track = aux & 0x3F;
    bool useCount = (aux & 0x40) ? true : false;
    bool longHeader = (aux & 0x8000) ? true : false;
    m_wd1771Status = 0xFF;
    m_trackNumber = track;

    int maxSectors = m_atxTrackInfo[track].size();
    data.resize(length);
    if (maxSectors == 0) {
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
        nbSectors = (aux & 0x4000) ? maxSectors : (aux >> 8) & 0x1F;
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
    quint16 lastPosition = 0;
    quint8 firstTiming = 0;
    for (int i = 0; i < nbSectors; i++) {
        AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].at(currentIndexInTrack);
        quint32 dist = 0;
        if (sectorInfo->sectorPosition() > lastPosition) {
            dist = (quint32) (sectorInfo->sectorPosition() - lastPosition);
        }
        else {
            dist = (quint32) (26042 + sectorInfo->sectorPosition() - lastPosition);
        }
        quint8 timing = (dist * (quint32)0x68) / (quint32)26042;
        if ((longHeader) && (currentIndexInData > 6)) {
            data[currentIndexInData - 2] = timing; // overwrite timing of previous sector
        }
        if (i == 0) {
            firstTiming = timing;
        }
        totalTiming += timing;
        if ((! useCount) && ((totalTiming - firstTiming) > timeoutValue)) {
            data[0] = i;
            break;
        }
        data[currentIndexInData++] = track;
#ifdef CHIP_810
        data[currentIndexInData++] = (i % maxSectors) << 2;
#else
        data[currentIndexInData++] = 0;
#endif
        data[currentIndexInData++] = sectorInfo->sectorNumber();
        data[currentIndexInData++] = 0;
        if (longHeader) {
            data[currentIndexInData++] = 5; // will be overwritten in next iteration
            data[currentIndexInData++] = 0;
        }
        currentIndexInTrack = (currentIndexInTrack + 1) % maxSectors;
        lastPosition = sectorInfo->sectorPosition();
    }
    for (int i = currentIndexInData; i < length; i++) {
        data[currentIndexInData++] = 0;
    }
}

bool SimpleDiskImage::readAtxSectorStatuses(QByteArray &data)
{
    data.resize(128);
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInAtxTrack(nbSectors, mapping)) {
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
        AtxSectorInfo *sectorInfo = m_atxTrackInfo[m_trackNumber].at(currentIndexInTrack);
        data[i + 1] = sectorInfo->wd1771Status();
        data[i + 0x41] = sectorInfo->fillByte();
        currentIndexInTrack = (currentIndexInTrack + 1) % m_atxTrackInfo[m_trackNumber].size();
    }
    return true;
}

bool SimpleDiskImage::readAtxSectorUsingIndex(quint16 aux, QByteArray &data)
{
    data.resize(128);
    quint16 index = aux & 0x1F;
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInAtxTrack(nbSectors, mapping)) {
        qWarning() << "!w" << tr("[%1] sector layout does not map to track layout")
                      .arg(deviceName());
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }
    int indexInTrack = (int)(quint32)mapping[index];
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[m_trackNumber].at(indexInTrack);
    if (sectorInfo == NULL) {
        qWarning() << "!w" << tr("[%1] no sector found at index %2 in track %3")
                      .arg(deviceName())
                      .arg(indexInTrack)
                      .arg(m_trackNumber, 2, 16, QChar('0'));
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        return false;
    }

    // check sector status
    bool readData = true;
    if ((sectorInfo->wd1771Status() & 0x10) == 0) {
        data.resize(128);
        for (int i = 0; i < 128; i++) {
            data[i] = 0;
        }
        readData = false;
    }

    // read sector if there is one
    if (readData) {
        QByteArray array = sectorInfo->sectorData();
        for (int i = 0; i < 128; i++) {
            data[i] = array[i];
        }
    }
    return true;
}

bool SimpleDiskImage::readAtxSector(quint16 aux, QByteArray &data)
{
    // chipFlags contains the CHIP copy flags in AUX2. If not 0, it means that the Atari is running Archiver 1.0.
    // Archiver 1.0 always sends flags in AUX2 but Super Archiver 3.02 fills flags in AUX2 only if sector is bad.
    // So we assume that we have Super Archiver 3.02 if speed is greater than 20000 in CHIP mode.
    // In both cases, the delay at the end of readAtxSector is disabled to have the highest speed (no delay between sectors)
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    if ((m_board.isChipOpen()) && (sio->port()->speed() > 20000)) {
        chipFlags |= 0x8000; // disable accurate timing
    }
    quint16 sector = aux & 0x3FF;

    // no accurate timing when reading all sectors to convert a file to another format
    if (m_conversionInProgress) {
        chipFlags |= 0x8000; // disable accurate timing
    }

    // no delay if CHIP mode or conversion in progress
    int newTrack = (sector - 1) / m_geometry.sectorsPerTrack();
    if ((!m_conversionInProgress) && (chipFlags == 0)) {

        // shot delay to process the read sector request
        QThread::usleep(3220);

        // compute delay if head was not on the right track
        int oldTrack = (m_lastSector - 1) / m_geometry.sectorsPerTrack();
        if (oldTrack != newTrack) {
            int diffTrack = abs(newTrack - oldTrack);
            unsigned long seekDelay = (unsigned long)(diffTrack * 5300L); // Use 810 timings.
            QThread::usleep(seekDelay);
        }
    }
    qint64 currentTimeInMicroSeconds = m_timer.nsecsElapsed();

    // get sector definition for this sector number
    qint64 fetchDelay = 0;
    qint64 currentDistanceInMicroSeconds = (m_timer.nsecsElapsed() / 1000) % 208333L;
    qint64 sectorDistanceInMicroSeconds = currentDistanceInMicroSeconds;
    int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[newTrack].find(relativeSector, (quint16)(currentDistanceInMicroSeconds >> 3));
    if (sectorInfo == NULL) {
        m_driveStatus = 0x10;
        m_wd1771Status = 0xEF;

        // no sector found. The delay is very long. Use an approximation:
        // 4 rotations to find the sector, then 44 step out, then seek to current track; repeated 2 times.
        fetchDelay = ((208333L << 2) + ((44 + newTrack) * 5300)) << 1;
    }
    else {
        m_driveStatus = sectorInfo->driveStatus();
        m_wd1771Status = sectorInfo->wd1771Status();
        if ((m_wd1771Status & 0x04) == 0) {
            m_wd1771Status &= 0xFD;
        }
        if ((m_board.isChipOpen()) && (chipFlags != 0)) {
            if ((m_wd1771Status & 0x20) == 0) {
                m_wd1771Status &= ~0x40;
            }
            else {
                m_wd1771Status |= 0x40;
            }
        }

        // compute the delay to find the sector
        sectorDistanceInMicroSeconds = ((qint64)(quint32)sectorInfo->sectorPosition()) << 3;
        if (sectorDistanceInMicroSeconds > currentDistanceInMicroSeconds) {
            fetchDelay = sectorDistanceInMicroSeconds - currentDistanceInMicroSeconds;
        }
        else {
            fetchDelay = 208333L + sectorDistanceInMicroSeconds - currentDistanceInMicroSeconds;
        }
    }

    // check sector status
    bool readData = true;
    int nbPhantoms = m_atxTrackInfo[newTrack].count(relativeSector);
    int phantomIndex = (nbPhantoms > 1) ? m_atxTrackInfo[newTrack].duplicateIndex(sectorInfo, relativeSector) : 1;
    if (m_wd1771Status != 0xFF) {
        if ((m_wd1771Status & 0x10) == 0) {
            if (!m_conversionInProgress) {
                qDebug() << "!u" << tr("[%1] Bad sector (status $%2) Grrr Grrr !")
                            .arg(deviceName())
                            .arg(m_wd1771Status, 2, 16, QChar('0'));
            }
            data.resize(128);
            readData = false;
        }
        else if ((m_wd1771Status & 0x20) == 0) {
            if (!m_conversionInProgress) {
                qDebug() << "!u" << tr("[%1] Deleted sector (status $%2)")
                            .arg(deviceName())
                            .arg(m_wd1771Status, 2, 16, QChar('0'));
            }
        }
        else if ((m_wd1771Status & 0x08) == 0) {

            // The firmware attempts a second read when a CRC error is found
            if (chipFlags == 0) {

                // check if there is another sector without the CRC error.
                qint64 startSectorDistanceInMicroSeconds = sectorDistanceInMicroSeconds + 9600;
                sectorInfo = m_atxTrackInfo[newTrack].find(relativeSector, (quint16)((((startSectorDistanceInMicroSeconds) % 208333) >> 3)));
                m_driveStatus = sectorInfo->driveStatus();
                m_wd1771Status = sectorInfo->wd1771Status();
                if ((m_wd1771Status & 0x04) == 0) {
                    m_wd1771Status &= 0xFD;
                }
                if ((m_board.isChipOpen()) && (chipFlags != 0)) {
                    if ((m_wd1771Status & 0x20) == 0) {
                        m_wd1771Status &= ~0x40;
                    }
                    else {
                        m_wd1771Status |= 0x40;
                    }
                }

                // check if we have found a different sector
                qint64 otherDistanceInMicroSeconds = ((qint64)(quint32)sectorInfo->sectorPosition()) << 3;
                if (otherDistanceInMicroSeconds != sectorDistanceInMicroSeconds) {
                    if (otherDistanceInMicroSeconds > sectorDistanceInMicroSeconds) {
                        fetchDelay += otherDistanceInMicroSeconds - sectorDistanceInMicroSeconds;
                    }
                    else {
                        fetchDelay += 208333L + otherDistanceInMicroSeconds - sectorDistanceInMicroSeconds;
                    }
                }
                else {
                    fetchDelay += 208333L;
                }
                phantomIndex = m_atxTrackInfo[newTrack].duplicateIndex(sectorInfo, relativeSector);
            }
            if (!m_conversionInProgress) {
                quint16 weakOffset = sectorInfo->sectorWeakOffset();
	            if (weakOffset != 0xFFFF) {
                    qDebug() << "!u" << tr("[%1] Weak sector at offset %2")
                                .arg(deviceName())
                                .arg(weakOffset);
                }
	            else if (nbPhantoms > 1) {
	                if ((m_wd1771Status & 0x08) == 0) {
                        qDebug() << "!u" << tr("[%1] CRC error (status $%2) on sector index #%3 among %4 phantom sectors")
                                    .arg(deviceName())
                                    .arg(m_wd1771Status, 2, 16, QChar('0'))
                                    .arg(phantomIndex)
                                    .arg(nbPhantoms);
                    }
	                else {
                        qDebug() << "!u" << tr("[%1] Read sector index #%2 among %3 phantom sectors")
                                    .arg(deviceName())
                                    .arg(phantomIndex)
                                    .arg(nbPhantoms);
                    }
                }
                else if ((m_wd1771Status & 0x08) == 0) {
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
        data = sectorInfo->sectorData();
        if (data.size() == 0) {
            data.resize(m_geometry.bytesPerSector());
        }
    }

    // simulate accurate timing.
    if ((!m_conversionInProgress) && (chipFlags == 0)) {

        // add the time for sector reading
        fetchDelay += (long)((1208 + 2) << 3);
        qint64 executionTimeInMicroSeconds = (m_timer.nsecsElapsed() - currentTimeInMicroSeconds) / 1000;
        QThread::usleep(fetchDelay - executionTimeInMicroSeconds);
    }
    m_trackNumber = newTrack;
    m_lastSector = sector;
    return true;
}

bool SimpleDiskImage::readAtxSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly)
{
	int firstTrack = aux & 0xFF;
	int secondTrack = (aux >> 8) & 0xFF;

	// overwrite input data
    m_board.m_trackData.clear();
    m_board.m_trackData.append(data);

    // Super Archive expects a sector list with timing
    if (!timingOnly) {

        // find the index in the first track of the sector list given by the Super Archiver
        int firstTrackSectorCount = m_atxTrackInfo[firstTrack].size();
        int secondTrackSectorCount = m_atxTrackInfo[secondTrack].size();
        quint8 nbSectorsToFind = data[3];
        for (int startIndex = 0; startIndex < firstTrackSectorCount; startIndex++) {
            AtxSectorInfo *firstTrackSectorInfo = NULL;
            for (int i = 0; i <= nbSectorsToFind; i++) {
                int index = (startIndex + i) % firstTrackSectorCount;
                firstTrackSectorInfo = m_atxTrackInfo[firstTrack].at(index);
                if (firstTrackSectorInfo == NULL) {
                    break;
                }
                quint8 sectorNumber = data[5 + (i % nbSectorsToFind)];
                if (sectorNumber != firstTrackSectorInfo->sectorNumber()) {
                    firstTrackSectorInfo = NULL;
                    break;
                }
            }
            if (firstTrackSectorInfo != NULL) {

                // now find the byte offset in the first track of this first sector and add the seek time
                // add the time to change track and issue another read sector command: 115319 microseconds for one track difference
                quint16 firstTrackByteOffset = ((quint16)(firstTrackSectorInfo->sectorPosition() >> 3) + 6);
                quint16 seekTimeInBytes = (quint16)((104100 + (20550 * (secondTrack - firstTrack - 1))) >> 6);
                firstTrackByteOffset = (firstTrackByteOffset + seekTimeInBytes) % (26042 >> 3);

                // find the first sector at the same rotation angle in the second track
                int nextIndex = 0;
                quint16 secondTrackByteOffset = 0;
                quint16 diffByteOffset = 0;
                for (int index = 0; index < secondTrackSectorCount; index++) {
                    AtxSectorInfo *secondTrackSectorInfo = m_atxTrackInfo[secondTrack].at(index);
                    secondTrackByteOffset = (quint16)(secondTrackSectorInfo->sectorPosition() >> 3);
                    if (secondTrackByteOffset >= (firstTrackByteOffset + 6)) {
                        diffByteOffset = secondTrackByteOffset - firstTrackByteOffset;
                        nextIndex = index;
                        break;
                    }
                }

                // fill output buffer with sector numbers and byte offsets
                for (int index = 0; index < secondTrackSectorCount; index++) {
                    int secondTrackIndex = (index + nextIndex) % secondTrackSectorCount;
                    AtxSectorInfo *secondTrackSectorInfo = m_atxTrackInfo[secondTrack].at(secondTrackIndex);
                    quint8 sectorNumber = secondTrackSectorInfo->sectorNumber();
                    int oldOffset = secondTrackByteOffset;
                    secondTrackByteOffset = (quint16)(secondTrackSectorInfo->sectorPosition() >> 3);
                    if (secondTrackByteOffset < oldOffset) {
                        diffByteOffset += (26042 >> 3) + secondTrackByteOffset - oldOffset;
                    }
                    else {
                        diffByteOffset += secondTrackByteOffset - oldOffset;
                    }
                    m_board.m_trackData[0x08 + index] = sectorNumber;
                    m_board.m_trackData[0x28 + index] = (diffByteOffset >> 8) & 0xFF;
                    m_board.m_trackData[0x48 + index] = diffByteOffset & 0xFF;
/*
qWarning() << "!w" << tr("[%1] track $%2 - $%3 $%4 $%5 - %6 | %7 - %8 | %9 $%10 $%11")
			.arg(deviceName())
            .arg(secondTrack, 2, 16, QChar('0'))
            .arg((quint8)(m_board.m_trackData[0x08 + index] & 0xFF), 2, 16, QChar('0'))
            .arg((quint8)(m_board.m_trackData[0x28 + index] & 0xFF), 2, 16, QChar('0'))
            .arg((quint8)(m_board.m_trackData[0x48 + index] & 0xFF), 2, 16, QChar('0'))
            .arg(diffByteOffset)
            .arg(firstTrackByteOffset)
            .arg(secondTrackByteOffset)
            .arg(startIndex)
            .arg(data[5], 2, 16, QChar('0'))
            .arg(data[6], 2, 16, QChar('0'));
*/
                }

                for (int i = secondTrackSectorCount; i < 0x20; i++) {
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
		//Archon (1983)(Electronic Arts)(US)[a].atx with SA 3.02 & 3.03
		//track $01
		//m_trackData[0x08] 04 06 08 0A 0C 0E 10 12 01 03 05 07 09 0B 0D 0F 11 02
		//m_trackData[0x28] 00 00 01 02 02 03 04 05 06 06 07 08 08 09 0A 0B 0B 0C
		//m_trackData[0x48] 4A F6 A3 50 FB A8 55 01 4D F9 A5 52 FE AA 57 03 AF 5C
		//track $02
		//m_trackData[0x08] 04 05 08 0A 0C 0E 10 12 06 01 03 05 07 09 0B 0D 0F 11 02
		//m_trackData[0x28] 00 00 01 02 02 03 04 04 05 06 06 07 08 08 09 0A 0A 0B 0C
		//m_trackData[0x48] 37 E1 8C 36 E0 8B 36 DF 88 4D F7 A2 4C F6 A1 4D F6 A1 4C
		//track $03
		//m_trackData[0x08] 0A 0C 0E 10 12 01 03 05 07 09 0B 0D 0F 11 02 04 06 08
		//m_trackData[0x28] 00 00 01 02 03 04 04 05 06 06 07 08 09 09 0A 0B 0B 0C
		//m_trackData[0x48] 50 FC A9 56 01 4D F8 A5 52 FE AA 57 03 AF 5C 08 B4 61
		//track $04
		//m_trackData[0x08] 04 06 08 0A 0C 0E 10 12 01 03 05 07 09 0B 0D 0F 11 02
		//m_trackData[0x28] 00 00 01 02 02 03 04 05 06 06 07 08 08 09 0A 0B 0B 0C
		//m_trackData[0x48] 4A F6 A3 50 FB A8 55 01 4D F9 A5 52 FE AA 57 03 AF 5C
        //track $05
        //m_trackData[0x08] 04 06 08 0A 0C 0E 10 12 01 03 05 07 09 0B 0D 0F 11 02
        //m_trackData[0x28] 00 00 01 02 02 03 04 05 06 06 07 08 08 09 0A 0B 0B 0C
        //m_trackData[0x48] 4A F6 A3 50 FB A8 55 01 4D F9 A5 52 FE AA 57 03 AF 5C
        //track $06
        //m_trackData[0x08] 04 06 08 0A 0C 0E 10 12 01 03 05 07 09 0B 0D 0F 11 02
        //m_trackData[0x28] 00 00 01 02 02 03 04 05 06 06 07 08 08 09 0A 0B 0B 0C
        //m_trackData[0x48] 4A F6 A3 50 FB A8 55 01 4D F9 A5 52 FE AA 57 03 AF 5C
    }
    else {

        // find position of the first sector
        AtxSectorInfo *firstTrackSectorInfo = m_atxTrackInfo[firstTrack].find(data[3], 0);
        if (firstTrackSectorInfo == NULL) {
            return false;
        }
        quint16 firstSectorPosition = firstTrackSectorInfo->sectorPosition();

        // add the time to change track and issue another read sector command: 115429 microseconds for one track difference
        quint16 seekTime = (quint16)((115429 + (20550 * (secondTrack - firstTrack - 1))) >> 3);
        // add also the time corresponding to the reading of the sector data: 154 bytes = 9856 microseconds
        firstSectorPosition = (firstSectorPosition + seekTime + (154 << 3)) % 26042;

        // now find the sector in the second track from the current rotation angle
        AtxSectorInfo *secondTrackSectorInfo = m_atxTrackInfo[secondTrack].find(data[4], firstSectorPosition);
        if (secondTrackSectorInfo == NULL) {
            return false;
        }
        quint16 secondSectorPosition = (secondTrackSectorInfo->sectorPosition() + (154 << 3) + 1244) % 26042;

        // compute distance between the 2 sectors
        quint16 nbBits = 0;
        if (secondSectorPosition > firstSectorPosition) {
            nbBits = (secondSectorPosition - firstSectorPosition) % 26042;
        }
        else {
            nbBits = (26042 + secondSectorPosition - firstSectorPosition)  % 26042;
        }
        if (nbBits < 25) {
            nbBits += 26042;
        }
        quint32 nbMicroSeconds = ((quint32)nbBits) << 3;

        // store result for Read memory command. The high byte is incremented each 17 cycles * 256 = 4352 cycles
        m_board.m_trackData[0xF4] = 0xFF - ((nbMicroSeconds % 4352) / 17);
        m_board.m_trackData[0xF5] = 0xFF - (nbMicroSeconds / 4352);
        m_board.m_trackData[0] = m_deviceNo;
        m_board.m_trackData[1] = 0x74;
        m_board.m_trackData[2] = (quint8)firstTrack;
        m_board.m_trackData[3] = (quint8)secondTrack;
/*
static quint16 timings[] = {
    0xE85D, //track $01
    0xED16, //track $02
    0xF1CE, //track $03
    0xF687, //track $04
    0xFB40, //track $05
    0xD01B, //track $06 $14 $22
    0xD4D0, //track $07
    0xD98D, //track $08
    0xDE46, //track $09
    0xE2FF, //track $0A
    0xE7B8, //track $0B
    0xEC72, //track $0C
    0xF12A, //track $0D
    0xF5E4  //track $0E
};
quint16 timing = timings[(secondTrack - 1) % 14];
qWarning() << "!w" << tr("[%1] track $%2 low=$%3 high=$%4 timer=%5 altirra low=$%6 altirra high=$%7 first=%8 second=%9")
            .arg(deviceName())
            .arg(secondTrack, 2, 16, QChar('0'))
            .arg((quint8)(m_board.m_trackData[0xF4] & 0xFF), 2, 16, QChar('0'))
            .arg((quint8)(m_board.m_trackData[0xF5] & 0xFF), 2, 16, QChar('0'))
            .arg(nbMicroSeconds)
            .arg((quint8)(timing & 0xFF), 2, 16, QChar('0'))
            .arg((quint8)((timing >> 8) & 0xFF), 2, 16, QChar('0'))
            .arg(firstSectorPosition)
            .arg(secondSectorPosition);
*/
        return true;
		//Archon (1983)(Electronic Arts)(US)[a].atx with SA 3.12
        //$01-$00 $5D $E8 = 23 * 4352 + 162 * 17 = 100096 + 2754 = 102850  Format: $DC5C
        //$02-$00 $16 $ED = 18 * 4352 + 233 * 17 =  78336 + 3961 =  82297  Format: $E115
        //$03-$00 $CE $F1 = 14 * 4352 +  49 * 17 =  60928 +  833 =  61761  Format: $E5CE
        //$04-$00 $87 $F6 =  9 * 4352 + 120 * 17 =  39168 + 2040 =  41208  Format: $EA86
        //$05-$00 $40 $FB =  4 * 4352 + 191 * 17 =  17408 + 3247 =  20655  Format: $EF3F
        //$06-$00 $1B $D0 = 47 * 4352 + 228 * 17 = 204544 + 3876 = 208420  Format: $F3F8
        //$07-$00 $D4 $D4 = 43 * 4352 +  43 * 17 = 187136 +  731 = 187867  Format: $C8D2
        //$08-$00 $8D $D9 = 38 * 4352 + 114 * 17 = 165376 + 1938 = 167314  Format: $CD8A
        //$09-$00 $46 $DE = 33 * 4352 + 185 * 17 = 143616 + 3145 = 146761  Format: $D243
        //$0A-$00 $FF $E2 = 29 * 4352 +   0 * 17 = 126208 +    0 = 126208  Format: $D6FC
        //$0B-$00 $B8 $E7 = 24 * 4352 +  71 * 17 = 104448 + 1207 = 105655  Format: $DBB5
        //$0C-$00 $72 $EC = 19 * 4352 + 141 * 17 =  82688 + 2397 =  85085  Format: $E06E
        //$0D-$00 $2A $F1 = 14 * 4352 + 213 * 17 =  60928 + 3621 =  64549  Format: $E526
        //$0E-$00 $E4 $F5 = 10 * 4352 +  27 * 17 =  43520 +  459 =  43979  Format: $E9DF
        //$0F-$0E $5D $E8 = 23 * 4352 + 162 * 17 = 100096 + 2754 = 102850  Format: $DC5C
        //$10-$0E $16 $ED = 18 * 4352 + 233 * 17 =  78336 + 3961 =  82297  Format: $E115
        //$11-$0E $CE $F1 = 14 * 4352 +  49 * 17 =  60928 +  833 =  61761  Format: $E5CE
        //$12-$0E $88 $F6 =  9 * 4352 + 119 * 17 =  39168 + 2023 =  41191  Format: $EA86
        //$13-$0E $40 $FB =  4 * 4352 + 191 * 17 =  17408 + 3247 =  20655  Format: $EF3F
        //$14-$0E $1A $D0 = 47 * 4352 + 229 * 17 = 204544 + 3893 = 208437  Format: $F3F8
        //$15-$0E $D3 $D4 = 43 * 4352 +  44 * 17 = 187136 +  748 = 187884  Format: $C8D2
        //$16-$0E $8C $D9 = 38 * 4352 + 115 * 17 = 165376 + 1955 = 167331  Format: $CD8A
        //$17-$0E $45 $DE = 33 * 4352 + 186 * 17 = 143616 + 3162 = 146778  Format: $D243
        //$18-$0E $FD $E2 = 29 * 4352 +   2 * 17 = 126208 +   34 = 126242  Format: $D6FC
        //$19-$0E $B7 $E7 = 24 * 4352 +  72 * 17 = 104448 + 1224 = 105672  Format: $DBB5
        //$1A-$0E $70 $EC = 19 * 4352 + 143 * 17 =  82688 + 2431 =  85119  Format: $E06E
    }
}

bool SimpleDiskImage::resetAtxTrack(quint16 aux)
{
    int trackNumber = aux & 0x3F;
    m_atxTrackInfo[trackNumber].clear();
    return true;
}

bool SimpleDiskImage::writeAtxTrack(quint16 aux, const QByteArray &data)
{
    quint16 firstTrack = aux & 0x3F;
    quint8 nbTracks = aux & 0x40 ? data[0x76] : 1;
    bool useSectorList = (aux & 0x2000) != 0;
    quint8 postIDCrc = (aux & 0x8000) ? data[0x74] : 17;
    quint8 postDataCrc = (aux & 0x8000) ? data[0x73] : 9;
    quint8 preIDField = (aux & 0x8000) ? data[0x72] : 6;

    for (quint16 track = firstTrack; track < (firstTrack + nbTracks); track++) {

        // reset track data
        if (!m_isModified) {
            m_isModified = true;
            emit statusChanged(m_deviceNo);
        }
        m_atxTrackInfo[track].clear();
        m_trackNumber = track;

        // fill CHIP ram
        m_board.m_chipRam[0] = data[0] > 28 ? 28 : data[0];
        quint16 sectorPosition = 100;
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
                quint8 sectorStatus = 0;
                quint8 dataSize = data[85 + index - 1];
                quint8 fillByte = data[57 + index - 1];
                if (dataSize != (quint8)128) {
                    sectorStatus = 0x08; // CRC error
                }
                else if ((fillByte == (0xFF - DISK_ID_ADDR_MARK)) || (fillByte >= 0x04 && fillByte <= 0x08)) {
                    sectorStatus = 0x08; // CRC error because these fill bytes are interpreted by FDC
                }
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[track].add(sector, sectorStatus, sectorPosition << 3);
                QByteArray sectorData;
                sectorData.resize(128);
                if (fillByte == 0x08) { // fill with CRC
                    dataSize = ((dataSize * 3) >> 1) + 2; // CRC takes more place to write
                    if (dataSize > (quint8)128) {
                        dataSize = (quint8)128;
                    }
                    sectorData[0] = 0x40;
                    sectorData[1] = 0x7B;
                    int patternIndex = 0;
                    for (quint8 j = 2; j < dataSize; j++) {
                        sectorData[j] = FDC_CRC_PATTERN[patternIndex++];
                        if (patternIndex >= 3) {
                            patternIndex = 0;
                        }
                    }
                    sectorPosition += (quint16)postDataCrc + (quint16)3 + (quint16)preIDField + (quint16)7 + (quint16)postIDCrc + (quint16)1 + (quint16)dataSize + (quint16)2;
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
                        sectorData[j] = fillByte;
                    }
                    sectorPosition += (quint16)postDataCrc + (quint16)3 + (quint16)preIDField + (quint16)7 + (quint16)postIDCrc + (quint16)1 + (quint16)dataSize + (quint16)2;
                    if (dataSize < (quint8)128) {
                        sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
                    }
                    if (dataSize < (quint8)128) {
                        sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
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
                    dataSize = writeAtxSectorHeader(dataSize, sectorData, postDataCrc, preIDField, postIDCrc, track, nextIndex, nextSector);
                    if (dataSize < (quint8)128) {
                        Crc16 crc16;
                        crc16.Reset();
                        crc16.Add((unsigned char) DISK_DATA_ADDR_MARK4);
                        sectorData[dataSize++] = 0xFF - DISK_DATA_ADDR_MARK4;
                        quint8 nextDataSize = nextIndex < m_board.m_chipRam[0] ? data[85 + nextIndex] : data[85 + nextIndex - m_board.m_chipRam[0]];
                        if (nextFillByte == 0x08) { // fill with CRC
                            nextDataSize = ((nextDataSize * 3) >> 1); // CRC takes more place to write
                            if (nextDataSize > (quint8)128) {
                                nextDataSize = (quint8)128;
                            }
                            if (dataSize < (quint8)128) {
                                sectorData[dataSize++] = 0x40;
                            }
                            if (dataSize < (quint8)128) {
                                sectorData[dataSize++] = 0x7B;
                            }
                            int patternIndex = 0;
                            while ((dataSize < (quint8)128) && (nextDataSize-- > 0)) {
                                sectorData[dataSize++] = FDC_CRC_PATTERN[patternIndex++];
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
                                sectorData[dataSize++] = nextFillByte;
                            }
                            if (dataSize < (quint8)128) {
                                sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
                            }
                            if (dataSize < (quint8)128) {
                                sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
                            }
                        }
                    }
                }
                sectorInfo->copySectorData(sectorData);
            }
            if (sectorPosition > (quint16)3600) {
                break; // two many sectors in this track
            }
        }
    }
    return true;
}

quint8 SimpleDiskImage::writeAtxSectorHeader(quint8 dataSize, QByteArray &sectorData, quint8 postDataCrc, quint8 preIDField, quint8 postIDCrc,
                                             quint8 track, quint8 index, quint8 nextSector) {
    for (quint8 j = 0; j < postDataCrc && dataSize < (quint8)128; j++) {
        sectorData[dataSize++] = 0xFF;
    }
#ifdef CHIP_810
    // these 3 zero bytes are written by CHIP 810 but not by Super Archiver 1050.
    for (quint8 j = 0; j < 3 && dataSize < (quint8)128; j++) {
        sectorData[dataSize++] = 0;
    }
#endif
    for (quint8 j = 0; j < preIDField && dataSize < (quint8)128; j++) {
        sectorData[dataSize++] = 0xFF;
    }
    Crc16 crc16;
    crc16.Reset();
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) DISK_ID_ADDR_MARK);
        sectorData[dataSize++] = (0xFF - DISK_ID_ADDR_MARK);
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) (track & 0xFF));
        sectorData[dataSize++] = (quint8)(0xFF - (track & 0xFF));
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) (index << 2));
#ifdef CHIP_810
        sectorData[dataSize++] = 0xFF - (quint8)(index << 2);
#else
        sectorData[dataSize++] = 0xFF;
#endif
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) nextSector);
        sectorData[dataSize++] = 0xFF - nextSector;
    }
    if (dataSize < (quint8)128) {
        crc16.Add((unsigned char) 0);
        sectorData[dataSize++] = 0xFF;
    }
    if (dataSize < (quint8)128) {
        sectorData[dataSize++] = (quint8)(((0xFFFF - crc16.GetCrc()) >> 8) & 0xFF);
    }
    if (dataSize < (quint8)128) {
        sectorData[dataSize++] = (quint8)((0xFFFF - crc16.GetCrc()) & 0xFF);
    }
    for (quint8 j = 0; j < postIDCrc && dataSize < (quint8)128; j++) {
        sectorData[dataSize++] = 0xFF;
    }
    return dataSize;
}

bool SimpleDiskImage::writeAtxTrackWithSkew(quint16 aux, const QByteArray &/*data*/) {
//    int firstTrack = data[3];
//    int secondTrack = aux & 0x3F;
    return writeTrack((aux & 0x3F) | 0xF000, m_board.m_trackData);
}

bool SimpleDiskImage::writeAtxSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy)
{
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    quint16 index = aux & 0x1F;
    int nbSectors = m_board.m_chipRam[0];
    if (nbSectors > 31) {
        nbSectors = 31;
    }
    QByteArray mapping(nbSectors, 0);
    if (! findMappingInAtxTrack(nbSectors, mapping)) {
        qWarning() << "!w" << tr("[%1] sector layout does not map to track layout")
                      .arg(deviceName());
        return false;
    }
    int indexInTrack = (int)(quint32)mapping[index];

    // check that the sector number in the track matches the sector number in the sector list
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[m_trackNumber].at(indexInTrack);
    if (sectorInfo == NULL) {
        qWarning() << "!w" << tr("[%1] no sector found at index %2 in track %3")
                      .arg(deviceName())
                      .arg(indexInTrack)
                      .arg(m_trackNumber, 2, 16, QChar('0'));
        return false;
    }
    quint8 sectorNumber = sectorInfo->sectorNumber();
    if ((quint8)m_board.m_chipRam[index + 1] != sectorNumber) {
        qWarning() << "!w" << tr("[%1] sector %2 does not match sector number at index %3 in track %4")
                      .arg(deviceName())
                      .arg(sectorNumber)
                      .arg(index)
                      .arg(m_trackNumber, 2, 16, QChar('0'));
        return false;
    }

    // compute sector status and sector length
    // convert CHIP flags 7,6,5 into WD1771 flags 5,3
    // bit 6 is ORed with bit 5 to reduce to 1 bit the DAM type as ATX format dore not support 2 bits
    quint8 sectorLength = 128;
    m_driveStatus = 0x10;
    m_wd1771Status = 0xFF;
    quint8 badSectorType = ((chipFlags & 0x40) >> 1) | (chipFlags & 0x20);
    if (chipFlags & 0x80) {
        badSectorType |= 0x08;
    }
    if (badSectorType != 0) {
        m_wd1771Status = 0xFF & ~badSectorType;
        if (fuzzy) {
            qDebug() << "!u" << tr("[%1] Fuzzy sector starting at byte %2")
                        .arg(deviceName())
                        .arg((quint16)data[126]);
        }
        else if ((chipFlags & 0x10) == 0) {
            sectorLength = (quint16)data[127];
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
    sectorInfo->setWd1771Status(m_wd1771Status);
    if (fuzzy) {
        sectorInfo->setSectorWeakOffset((quint16)data[126]);
    }

    // write sector
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    for (quint8 i = 0; i < sectorLength; i++) {
        sectorInfo->copySectorData(data);
    }
    m_lastSector = (m_trackNumber * m_geometry.sectorsPerTrack()) + sectorInfo->sectorNumber();
    return true;
}

bool SimpleDiskImage::writeFuzzyAtxSector(quint16 aux, const QByteArray &data)
{
	// Fuzzy sectors are supported but from the fuzzed bytes always extend to the end of the sector.
	quint8 weakOffset = data[126];
    quint16 chipFlags = (aux >> 8) & 0xFC;
    quint16 sector = aux & 0x3FF;

// TODO 1: check if AUX2 contains usual CHIP flags (bad sector type,...) !
// TODO 2: check if data[127] is the last buzzy byte index or is the last byte to write (fuzzy short sector)

    // 1 disk rotation = 208333us
    // Compute the relative head position since the last read command (assume disk never stops spinning)
    qint64 newTime = QDateTime::currentMSecsSinceEpoch();
    qint64 diffTime = (m_lastTime == 0) ? 0 : ((newTime - m_lastTime) * 1000);
    qint64 distance = (m_lastDistance + diffTime) % 208333L;

    // compute delay if head was not on the right track
    int oldTrack = (m_lastSector - 1) / m_geometry.sectorsPerTrack();
    int newTrack = (sector - 1) / m_geometry.sectorsPerTrack();
    int diffTrack = abs(newTrack - oldTrack);

    // adjust relative head position when head was moving on another track
    if (diffTrack > 0) {
        unsigned long seekDelay = (unsigned long) diffTrack * 5200L;
        distance = (distance + seekDelay) % 208333L;
    }

    // get sector definition for this sector number
    int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[newTrack].find(relativeSector, (quint16)(distance >> 3));
    if (sectorInfo == NULL) {
        m_driveStatus = 0x10;
        m_wd1771Status = 0xEF;
        qCritical() << "!e" << tr("[%1] Sector %2 does not exist in ATX file")
                       .arg(deviceName())
                       .arg(sector);
		return false;
    }
    else {

        // compute sector status
        quint8 sectorLength = m_geometry.bytesPerSector();
        m_driveStatus = 0x10;
        m_wd1771Status = 0xFF;

        // convert CHIP flags 7,6,5 into WD1771 flags 5,3
        // bit 6 is ORed with bit 5 to reduce to 1 bit the DAM type as ATX format does not support 2 bits
        quint8 badSectorType = ((chipFlags & 0x40) >> 1) | (chipFlags & 0x20);
        if (chipFlags & 0x80) {
            badSectorType |= 0x08;
        }
        if (badSectorType != 0) {
            m_wd1771Status = 0xFF & ~badSectorType;
            if ((chipFlags & 0x10) == 0) {
                sectorLength = (quint16)data[127];
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
        sectorInfo->setWd1771Status(m_wd1771Status);
    }

	// set weak bytes and copy data
	sectorInfo->setSectorWeakOffset(weakOffset);
	sectorInfo->copySectorData(data);
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }

    // change the status to reflect the CRC error generated by these weak bits
    sectorInfo->setWd1771Status(((sectorInfo->wd1771Status() & ~0x08) | 0x10) & 0xFF);
    m_lastDistance = sectorInfo->sectorPosition();
	return true;
}

bool SimpleDiskImage::writeAtxSector(quint16 aux, const QByteArray &data)
{
    // 1 disk rotation = 208333us
    // Compute the relative head position since the last read command (assume disk never stops spinning)
    qint64 newTime = QDateTime::currentMSecsSinceEpoch();
    qint64 diffTime = (m_lastTime == 0) ? 0 : ((newTime - m_lastTime) * 1000);
    qint64 distance = (m_lastDistance + diffTime) % 208333L;

    // extract sector number and flags
    quint16 chipFlags = m_board.isChipOpen() ? (aux >> 8) & 0xFC : 0;
    quint16 sector = m_board.isChipOpen() ? aux & 0x3FF : aux;

    // compute delay if head was not on the right track
    int oldTrack = (m_lastSector - 1) / m_geometry.sectorsPerTrack();
    int newTrack = (sector - 1) / m_geometry.sectorsPerTrack();
    int diffTrack = abs(newTrack - oldTrack);

    // adjust relative head position when head was moving on another track
    if (diffTrack > 0) {
        unsigned long seekDelay = (unsigned long) diffTrack * 5200L;
        distance = (distance + seekDelay) % 208333L;
    }

    // get sector definition for this sector number
    qint64 headDistance = distance;
    int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
    quint16 sectorLength = 0;
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[newTrack].find(relativeSector, (quint16)(distance >> 3));
    if (sectorInfo == NULL) {
        m_driveStatus = 0x10;
        m_wd1771Status = 0xEF;
        qCritical() << "!e" << tr("[%1] Sector %2 does not exist in ATX file")
                       .arg(deviceName())
                       .arg(sector);
    }
    else {

        // get the sector position.
        headDistance = ((qint64)(quint32)sectorInfo->sectorPosition()) << 3;

        // compute sector status
        sectorLength = m_geometry.bytesPerSector();
        m_driveStatus = 0x10;
        m_wd1771Status = 0xFF;

        // convert CHIP flags 7,6,5 into WD1771 flags 5,3
        // bit 6 is ORed with bit 5 to reduce to 1 bit the DAM type as ATX format does not support 2 bits
        quint8 badSectorType = ((chipFlags & 0x40) >> 1) | (chipFlags & 0x20);
        if (chipFlags & 0x80) {
            badSectorType |= 0x08;
        }
        if (badSectorType != 0) {
            m_wd1771Status = 0xFF & ~badSectorType;
            if ((chipFlags & 0x10) == 0) {
                sectorLength = (quint16)data[127];
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
        sectorInfo->setWd1771Status(m_wd1771Status);
    }
    m_lastDistance = (headDistance + 15000) % 208333L;
    m_trackNumber = newTrack;
    m_lastSector = sector;
    m_lastTime = QDateTime::currentMSecsSinceEpoch();

    // write sector
    if (sectorLength > 0) {
        if (!m_isModified) {
            m_isModified = true;
            emit statusChanged(m_deviceNo);
        }
        sectorInfo->copySectorData(data);
        return true;
    }
    return false;
}

bool SimpleDiskImage::writeAtxSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8, quint8 sectorNumber, quint8, const QByteArray &data, bool crcError, int weakOffset)
{
    if ((sectorNumber < 1) || (sectorNumber > m_geometry.sectorsPerTrack())) {
        return true;
    }
    quint8 sectorStatus = (crcError) ? 0x08 : 0;
    if ((dataType & 0x01) == 0) {
        sectorStatus |= 0x20;
    }
    if (weakOffset != 0xFFFF) {
        sectorStatus |= 0x40;
    }
    AtxSectorInfo *sectorInfo = m_atxTrackInfo[trackNumber].add(sectorNumber, sectorStatus, (quint16)bitNumber);

    // write sector
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    sectorInfo->setSectorWeakOffset(weakOffset);
    sectorInfo->copySectorData(data);
    return true;
}

bool SimpleDiskImage::findMappingInAtxTrack(int nbSectors, QByteArray &mapping)
{
    int sectorsInTrack = m_atxTrackInfo[m_trackNumber].size();
    if ((nbSectors == 0) || (sectorsInTrack == 0)) {
        return false;
    }
    for (int sectorStartIndex = 0; sectorStartIndex < nbSectors; sectorStartIndex++) {
        for (int currentIndexInTrack = 0; currentIndexInTrack < sectorsInTrack; currentIndexInTrack++) {
            bool match = true;
            int indexInTrack = currentIndexInTrack;
            for (int sectorIndex = 0; sectorIndex < nbSectors; sectorIndex++) {
                int indexInRam = (sectorStartIndex + sectorIndex) % nbSectors;
                AtxSectorInfo *sectorInfo = m_atxTrackInfo[m_trackNumber].at(indexInTrack);
                if (sectorInfo == NULL) {
                    match = false;
                    break;
                }
                quint8 sectorNumber = sectorInfo->sectorNumber();
                if (m_board.m_chipRam[indexInRam + 1] != sectorNumber) {
                    match = false;
                    break;
                }
                indexInTrack = (indexInTrack + 1) % sectorsInTrack;
            }
            if (match) {
                for (int sectorIndex = 0; sectorIndex < nbSectors; sectorIndex++) {
                    int indexInRam = (sectorStartIndex + sectorIndex) % nbSectors;
                    mapping[indexInRam] = currentIndexInTrack;
                    currentIndexInTrack = (currentIndexInTrack + 1) % sectorsInTrack;
                }
                return true;
            }
        }
    }
    return false;
}

QByteArray AtxSectorInfo::sectorData()
{
	if (m_sectorWeakOffset != 0xFFFF) {
		for (int i = m_sectorWeakOffset; i < m_sectorData.size(); i++) {
			m_sectorData[i] = qrand() % 0xFF;
		}
	}
	return m_sectorData;
}

quint8 AtxSectorInfo::byteAt(int pos)
{
	if (pos >= m_sectorData.size()) {
		return 0;
	}
	if ((m_sectorWeakOffset != 0xFFFF) && (pos >= m_sectorWeakOffset)) {
		return qrand() & 0xFF;
	}
	return m_sectorData[pos];
}

quint8 AtxSectorInfo::rawByteAt(int pos)
{
	// same as byteAt but does not interpret weak bits
	if (pos >= m_sectorData.size()) {
		return 0;
	}
	return m_sectorData[pos];
}

quint8 AtxSectorInfo::fillByte()
{
	if (m_sectorWeakOffset != 0xFFFF) {
		return 1;
	}
	quint8 value = m_sectorData[0];
	for (int i = 0; i < m_sectorData.size(); i++) {
		if ((quint8)m_sectorData[i] != value) {
			return 1;
		}
	}
	return value;
}

void AtxSectorInfo::copySectorData(const QByteArray &sectorData)
{
	m_sectorData.clear();
	for (int i = 0; i < sectorData.size(); i++) {
		m_sectorData[i] = sectorData[i];
	}
}

void AtxSectorInfo::setSectorWeakOffset(quint16 sectorWeakOffet)
{
	m_sectorWeakOffset = sectorWeakOffet;
	if (sectorWeakOffet != 0xFFFF) {
		m_sectorStatus |= 0x40;
	}
	else {
		m_sectorStatus &= ~0x40;
	}
}

int AtxSectorInfo::dataMarkOffset(int headerOffset, int shift)
{
    // skip the header size
    int index = headerOffset + 6;
    // after the sector header, we should find at least 6 $00 bytes and then a DATA address mark.
    // We only check for 5 $00 because the remaining bytes may not be byte aligned.
    int nbConsecutive00 = 0;
    while (index < size() - 1) {
        if (byteAt(index) == (quint8)(0xFF - 0x00)) {
            nbConsecutive00++;
        }
        else if (nbConsecutive00 >= 5) {
            int data = (((unsigned char)byteAt(index) << shift) | ((unsigned char)byteAt(index + 1) >> (8 - shift))) & 0xFF;
            if ((data == (quint8)(0xFF - DISK_DATA_ADDR_MARK1)) ||
                    (data == (quint8)(0xFF - DISK_DATA_ADDR_MARK2)) ||
                    (data == (quint8)(0xFF - DISK_DATA_ADDR_MARK3)) ||
                    (data == (quint8)(0xFF - DISK_DATA_ADDR_MARK4))) {
                return index;
            }
        }
        else {
            nbConsecutive00 = 0;
        }
        index++;
    }
    return -1;
}

AtxSectorInfo* AtxTrackInfo::add(quint8 sectorNumber, quint8 sectorStatus, quint16 sectorPosition)
{
	AtxSectorInfo *sector = new AtxSectorInfo(sectorNumber, sectorStatus, sectorPosition);
	m_sectors.append(sector);
	return sector;
}

int AtxTrackInfo::count(quint8 sectorNumber)
{
	int nb = 0;
	if (! m_sectors.isEmpty()) {
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
			if (sector->sectorNumber() == sectorNumber) {
				nb++;
			}
		}
	}
	return nb;
}

int AtxTrackInfo::numberOfExtendedSectors()
{
	int nb = 0;
	if (! m_sectors.isEmpty()) {
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
			if (sector->sectorWeakOffset() != 0xFFFF) {
				nb++;
			}
		}
	}
	return nb;
}

int AtxTrackInfo::numberOfSectorsWithData()
{
	int nb = 0;
	if (! m_sectors.isEmpty()) {
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
			if ((sector->sectorStatus() & 0x10) == 0) {
				nb++;
			}
		}
	}
	return nb;
}

void AtxTrackInfo::clear()
{
	if (! m_sectors.isEmpty()) {
		qDeleteAll(m_sectors.begin(), m_sectors.end());
	}
	m_sectors.clear();
}

AtxSectorInfo* AtxTrackInfo::find(quint8 sectorNumber, quint16 distance)
{
	if (! m_sectors.isEmpty()) {
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
            if ((sector->sectorNumber() == sectorNumber) && (sector->sectorPosition() >= distance) /*&& ((sector->sectorStatus() & 0x30) == 0)*/) {
				return sector;
			}
		}
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
			if (sector->sectorNumber() == sectorNumber) {
				return sector;
			}
		}
	}
	return NULL;
}

int AtxTrackInfo::duplicateIndex(AtxSectorInfo *sectorInfo, int sectorNumber)
{
	if (! m_sectors.isEmpty()) {
		int index = 0;
		for (QList<AtxSectorInfo*>::iterator it = m_sectors.begin(); it != m_sectors.end(); ++it) {
			AtxSectorInfo *sector = *it;
			if (sector->sectorNumber() == sectorNumber) {
				index++;
			}
			if (sector == sectorInfo) {
				return index;
			}
		}
	}
	return 1;
}

int AtxTrackInfo::shortSectorSize(int track, int sectorIndex, int *bitShift) {
    if ((sectorIndex < 0) || (sectorIndex >= size())) {
        return 0;
    }
    AtxSectorInfo *sector = at(sectorIndex++);
    // Try to find the offset of the ID address mark in the sector data.
    // The main problem is that the next sector header may or may not be byte aligned with the current one.
    // It means we have to find a header considering all shift positions
    AtxSectorInfo *nextSector = at(sectorIndex < size() ? sectorIndex : 0);
    // if the next sector is too far, no need to check for short sector.
    int sectorPos = (int)(quint32)sector->sectorPosition();
    int nextSectorPos = (int)(quint32)nextSector->sectorPosition();
    int gap = nextSectorPos - sectorPos;
    if (gap > (145 << 3)) {
        return 0;
    }
    unsigned char invertedTrack = (0xFF - (unsigned char)track) & 0xFF;
    for (int index = 0; index < sector->size() - 9; index++) {
        for (int shift = 0; shift < 8; shift++) {
            // shift the sector data to get the real header values
            unsigned char sectorHeader[8];
            for (unsigned int headerByte = 0; headerByte < sizeof(sectorHeader); headerByte++) {
                sectorHeader[headerByte] = (((unsigned char)sector->byteAt(index + headerByte) << shift) | ((unsigned char)sector->byteAt(index + headerByte + 1) >> (8 - shift))) & 0xFF;
            }
            // the header must start with $00 $FE <track> but data is inverted so we check for $FF $01 <invertedTrack>
            if ((sectorHeader[0] == 0xFF) && (sectorHeader[1] == 0x01) && (sectorHeader[2] == invertedTrack)) {
                quint8 sectorNumber = (0xFF - sectorHeader[4]) & 0xFF;
                if (sectorNumber == nextSector->sectorNumber()) {
                    Crc16 crc16;
                    crc16.Reset();
                    for (int m = 0; m < 5; m++) {
                        crc16.Add((unsigned char) ((0xFF - sectorHeader[m + 1]) & 0xFF));
                    }
                    unsigned short readCrc = 0xFFFF - ((((unsigned short)sectorHeader[6]) << 8) | (((unsigned short)sectorHeader[7]) & 0xFF));
                    if (readCrc == crc16.GetCrc()) {
                        *bitShift = shift;
                        return index + 1;
                    }
                }
            }
        }
    }
    return 0;
}

int SimpleDiskImage::sectorsInCurrentAtxTrack()
{
    if (m_trackNumber <= 39) {
        return m_atxTrackInfo[m_trackNumber].size();
    }
    return 0;
}
