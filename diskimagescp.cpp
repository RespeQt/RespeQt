//
// diskimagescp class
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

bool SimpleDiskImage::openScp(const QString &fileName)
{
    QFile *sourceFile;

    if (m_originalImageType == FileTypes::Scp) {
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
    QByteArray header = sourceFile->read(16);
    if (header.size() != 16) {
        qCritical() << "!e" << tr("[%1] Cannot open '%2': %3")
		              .arg(deviceName())
                      .arg(fileName)
                      .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate the magic word
    QByteArray scp = QByteArray(header.data(), 3);
    if (scp != "SCP") {
        qCritical() << "!e" << tr("[%1] Cannot open '%2': %3").arg(deviceName()).arg(fileName).arg(tr("Not a valid SCP file."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }

    // Validate the bit cell time and the head number
    if (header[9]) {
        qCritical() << "!e" << tr("[%1] Cannot open '%2': %3").arg(deviceName()).arg(fileName).arg(tr("Unsupported bit cell time width."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }
    if (header[10] > (char)1) {
        qCritical() << "!e" << tr("[%1] Cannot open '%2': %3").arg(deviceName()).arg(fileName).arg(tr("The file does not contain disk side 0."));
        sourceFile->close();
        delete sourceFile;
        return false;
    }
	int nbRevolutions = (int)header[5];

	// mark all tracks as empty
    for (int track = 0; track < 40; track++) {
        m_scpTrackInfo[track].setTrackOffset(0);
    }

	// validate the offset table
	QByteArray offsets = sourceFile->read(0x2A0);
    if (offsets.size() != 0x2A0) {
        qCritical() << "!e" << tr("[%1] Cannot open '%2': %3")
		              .arg(deviceName())
                      .arg(fileName)
                      .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        sourceFile->close();
        delete sourceFile;
        return false;
    }
	for (int track = 0; track < 166; track++) {
		// read the entire track offset
		quint32 trackOffset = getLittleIndianLong(offsets, track * 4);
		if (trackOffset == 0) {
			continue;
		}
        if (!sourceFile->seek(trackOffset)) {
            qCritical() << "!e" << tr("[%1] Cannot seek to track header #%2: %3").arg(deviceName()).arg(track).arg(sourceFile->error());
            sourceFile->close();
            delete sourceFile;
            return false;
        }
		int trackHeaderSize = 4 + (12 * nbRevolutions);
		QByteArray trackHeader = sourceFile->read(trackHeaderSize);
		if (trackHeader.size() != trackHeaderSize) {
			qCritical() << "!e" << tr("[%1] Cannot open '%2': %3")
			              .arg(deviceName())
						  .arg(fileName)
						  .arg(tr("Cannot read the track #%1 header: %2.").arg(track).arg(sourceFile->errorString()));
			sourceFile->close();
			delete sourceFile;
			return false;
		}

		// Validate the magic word
		QByteArray trk = QByteArray(trackHeader.data(), 3);
		if (trk != "TRK") {
			qCritical() << "!e" << tr("[%1] Cannot open '%2': %3").arg(deviceName()).arg(fileName).arg(tr("Not a valid SCP file."));
			sourceFile->close();
			delete sourceFile;
			return false;
		}

		// store the offsets in memory to speed up access when a track is read
		int trackNumber = (int) trackHeader[3];
		if ((trackNumber >= 0) && (trackNumber < 40)) {
			m_scpTrackInfo[trackNumber].clearRevolutions();
			m_scpTrackInfo[trackNumber].setTrackOffset(trackOffset);
			int infoOffset = 4;
			for (int i = 0; i < nbRevolutions; i++) {
				quint32 duration = getLittleIndianLong(trackHeader, infoOffset + 0);
				quint32 bitcells = getLittleIndianLong(trackHeader, infoOffset + 4);
				quint32 offset = getLittleIndianLong(trackHeader, infoOffset + 8);
				m_scpTrackInfo[trackNumber].addRevolution(duration, bitcells, offset);
				infoOffset += 12;
			}
		}
	}

	// check that all tracks have been loaded
    for (int track = 0; track < 40; track++) {
        if (m_scpTrackInfo[track].getTrackOffset() == 0) {
            qCritical() << "!e" << tr("[%1] Cannot seek to track header #%2: %3").arg(deviceName()).arg(track).arg(tr("No data for track."));
            sourceFile->close();
            delete sourceFile;
            return false;
		}
	}

    // check density
    int density = 0;
    // TODO
	bool mfm = density > 0;

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
        QByteArray secBuf;
        for (int track = 0; track < 40; track++) {
            if (m_scpTrackInfo[track].loadTrack(m_originalFileName)) {
                int nbSectors = m_scpTrackInfo[track].size();
                for (int sector = 0; sector < nbSectors; sector++) {
                    AtxSectorInfo *sectorInfo = m_scpTrackInfo[track].at(sector);
                    quint8 sectorNumber = sectorInfo->sectorNumber();
                    quint8 sectorStatus = sectorInfo->sectorStatus();
                    if (secBuf.size() > 0) {
                        secBuf.append(' ');
                    }
                    secBuf.append(tr("%1").arg(sectorNumber, 2, 16, QChar('0')));
                    // display only one word (the most important) to keep the line as compact as possible
                    if (sectorStatus & 0x40) {
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
                    else if (m_scpTrackInfo[track].count(sectorNumber) > 0) {
                        secBuf.append("(DUP)");
                    }
                    else if ((sectorStatus & 0x06) == 0x06) {
                        secBuf.append("(LONG)");
                    }
                }
            }
            else {
                secBuf.resize(0);
                secBuf.append(tr("Error loading track data from %1").arg(m_originalFileName));
            }
            qDebug() << "!u" << tr("track $%1 (%2): %3").arg(track, 2, 16, QChar('0')).arg(mfm ? "MFM" : "FM").arg(secBuf.constData());
        }
    }

    // assume single density protected disk (should check for track 0 density)
    int size = (density == 2) ? 183936 : ((density == 1) ? 133120 : 92160);
    m_geometry.initialize(size);
    refreshNewGeometry();
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

/*
				for(auto it2 = data_buf.begin(), it2End = data_buf.end(); it2 != it2End; ++it2) {
					const auto& offset_be = *it2;

					// need to byte-swap
					uint32_t offset = ((offset_be << 8) & 0xff00) + ((offset_be >> 8) & 0xff);

					if (offset) {
						time += offset;
						raw_track.mTransitions.push_back(time);
					} else
						time += 0x10000;
				}

void process_track_fm(const RawTrack& rawTrack, int logicalTrack) {
	if (rawTrack.mTransitions.size() < 2)
		return;

	// Atari disk timing produces 250,000 clocks per second at 288 RPM. We must compute the
	// effective sample rate given the actual disk rate.
	const double cells_per_rev = 250000.0 / (288.0 / 60.0);
	double scks_per_cell = rawTrack.mSamplesPerRev / cells_per_rev * g_clockPeriodAdjust;

	//printf("%.2f samples per cell\n", scks_per_cell);

	const uint32_t *samp = rawTrack.mTransitions.data();
	size_t samps_left = rawTrack.mTransitions.size() - 1;
	int time_basis = 0;
	int time_left = 0;

	int cell_len = (int)(scks_per_cell + 0.5);
	int cell_range = cell_len / 3;
	int cell_timer = 0;
	int cell_fine_adjust = 0;

	uint8_t shift_even = 0;
	uint8_t shift_odd = 0;

	std::vector<SectorParser> sectorParsers;
	uint8_t spew_data[16];
	int spew_index = 0;
	uint32_t spew_last_time = samp[0];

	for(;;) {
		while (time_left <= 0) {
			if (!samps_left)
				goto done;

			int delta = samp[1] - samp[0];

			if (g_verbosity >= 4)
				printf(" %02X %02X | %3d | %d\n", shift_even, shift_odd, delta, samp[0]);

			time_left += delta;
			time_basis = samp[1];
			++samp;
			--samps_left;
		}

		//printf("next_trans = %d, cell_timer = %d, %d transitions left\n", time_left, cell_timer, samps_left);

		// if the shift register is empty, restart shift timing at next transition
		if (!(shift_even | shift_odd)) {
			time_left = 0;
			cell_timer = cell_len;
			shift_even = 0;
			shift_odd = 1;
		} else {
			// compare time to next transition against cell length
			int trans_delta = time_left - cell_timer;

			if (trans_delta < -cell_range) {
				if (g_verbosity >= 4)
					printf(" %02X %02X | delta = %+3d | ignore\n", shift_even, shift_odd, trans_delta);
				// ignore the transition
				cell_timer -= time_left;
				continue;
			}

			std::swap(shift_even, shift_odd);
			shift_odd += shift_odd;
			
			if (trans_delta <= cell_range) {
				++shift_odd;

				if (g_verbosity >= 4)
					printf(" %02X %02X | delta = %+3d | 1\n", shift_even, shift_odd, trans_delta);

				// we have a transition in range -- clock in a 1 bit
				cell_timer = cell_len;
				time_left = 0;

				// adjust clocking by phase error
				if (trans_delta < -5)
					cell_timer -= 3;
				else if (trans_delta < -3)
					cell_timer -= 2;
				else if (trans_delta < 1)
					--cell_timer;
				else if (trans_delta > 1)
					++cell_timer;
				else if (trans_delta > 3)
					cell_timer += 2;
				else if (trans_delta > 5)
					cell_timer += 3;
			} else {
				if (g_verbosity >= 4)
					printf(" %02X %02X | delta = %+3d | 0\n", shift_even, shift_odd, trans_delta);

				// we don't have a transition in range -- clock in a 0 bit
				time_left -= cell_timer;
				cell_timer = cell_len + (cell_fine_adjust / 256);
			}

			if (g_verbosity >= 3) {
				spew_data[spew_index] = shift_odd;
				if (++spew_index == 16) {
					spew_index = 0;
					printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %.2f\n"
						, spew_data[0]
						, spew_data[1]
						, spew_data[2]
						, spew_data[3]
						, spew_data[4]
						, spew_data[5]
						, spew_data[6]
						, spew_data[7]
						, spew_data[8]
						, spew_data[9]
						, spew_data[10]
						, spew_data[11]
						, spew_data[12]
						, spew_data[13]
						, spew_data[14]
						, spew_data[15]
						, (double)(time_basis - time_left - spew_last_time) / scks_per_cell
						);

					spew_last_time = time_basis - time_left;
				}
			}
			
			const uint32_t vsn_time = time_basis - time_left;
			for(auto it = sectorParsers.begin(); it != sectorParsers.end();) {
				if (it->Parse(vsn_time, shift_even, shift_odd))
					++it;
				else
					it = sectorParsers.erase(it);
			}

			if (shift_even == 0xC7 && shift_odd == 0xFE) {
				sectorParsers.emplace_back();
				sectorParsers.back().Init(logicalTrack, &rawTrack.mIndexTimes, (float)scks_per_cell, &g_disk.mTracks[rawTrack.mSide][rawTrack.mTrack], vsn_time);
			}
		}
	}

done:
	;
}
*/

bool SimpleDiskImage::saveScp(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving SCP images is not supported yet."));
    return false;
}

bool SimpleDiskImage::createScp(int /*untitledName*/)
{
    setReady(true);
    return true;
}

bool SimpleDiskImage::readHappyScpSectorAtPosition(int /*trackNumber*/, int /*sectorNumber*/, int /*afterSectorNumber*/, int &/*index*/, QByteArray &/*data*/)
{
    return true;
}

bool SimpleDiskImage::readHappyScpSkewAlignment(bool /*happy1050*/)
{
    return true;
}

bool SimpleDiskImage::writeHappyScpTrack(int /*trackNumber*/, bool /*happy1050*/)
{
    m_isModified = true;
    emit statusChanged(m_deviceNo);
    return true;
}

bool SimpleDiskImage::writeHappyScpSectors(int /*trackNumber*/, int /*afterSectorNumber*/, bool /*happy1050*/)
{
    return true;
}

bool SimpleDiskImage::formatScp(const DiskGeometry &/*geo*/)
{
    m_isModified = true;
    emit statusChanged(m_deviceNo);
    return true;
}

void SimpleDiskImage::readScpTrack(quint16 /*aux*/, QByteArray &/*data*/, int /*length*/)
{
}

bool SimpleDiskImage::readScpSectorStatuses(QByteArray &/*data*/)
{
    return true;
}

bool SimpleDiskImage::readScpSectorUsingIndex(quint16 /*aux*/, QByteArray &/*data*/)
{
    return true;
}

bool SimpleDiskImage::readScpSector(quint16 /*aux*/, QByteArray &/*data*/)
{
    return true;
}

bool SimpleDiskImage::readScpSkewAlignment(quint16 /*aux*/, QByteArray &data, bool /*timingOnly*/)
{
    m_board.m_trackData.clear();
    m_board.m_trackData.append(data);
    return true;
}

bool SimpleDiskImage::resetScpTrack(quint16 /*aux*/)
{
    return true;
}

bool SimpleDiskImage::writeScpTrack(quint16 /*aux*/, const QByteArray &/*data*/)
{
    return true;
}

bool SimpleDiskImage::writeScpTrackWithSkew(quint16 /*aux*/, const QByteArray &/*data*/) {
    return true;
}

bool SimpleDiskImage::writeScpSectorUsingIndex(quint16 /*aux*/, const QByteArray &/*data*/, bool /*fuzzy*/)
{
    return true;
}

bool SimpleDiskImage::writeFuzzyScpSector(quint16 /*aux*/, const QByteArray &/*data*/)
{
	return true;
}

bool SimpleDiskImage::writeScpSector(quint16 /*aux*/, const QByteArray &/*data*/)
{
    return false;
}

bool SimpleDiskImage::writeScpSectorExtended(int /*bitNumber*/, quint8 /*dataType*/, quint8 /*trackNumber*/, quint8, quint8 /*sectorNumber*/, quint8, const QByteArray &/*data*/, bool /*crcError*/, int /*weakOffset*/)
{
    return true;
}

ScpTrackRevolution* ScpTrackInfo::addRevolution(quint32 duration, quint32 bitcells, quint32 offset)
{
    auto track = new ScpTrackRevolution(duration, bitcells, offset);
	m_trackRevolutions.append(track);
	return track;
}

void ScpTrackInfo::clearRevolutions()
{
	if (! m_trackRevolutions.isEmpty()) {
		qDeleteAll(m_trackRevolutions.begin(), m_trackRevolutions.end());
	}
	m_trackRevolutions.clear();
	m_trackOffset = 0;
}

void ScpTrackInfo::clear()
{
	if (! m_sectors.isEmpty()) {
		qDeleteAll(m_sectors.begin(), m_sectors.end());
	}
	m_sectors.clear();
}

int ScpTrackInfo::count(quint8 /*sectorNumber*/)
{
	// TODO
	return 0;
}

bool ScpTrackInfo::loadTrack(QString &/*originalFileName*/)
{
	// TODO
	return false;
}

int SimpleDiskImage::sectorsInCurrentScpTrack()
{
    // TODO
    return 0;
}
