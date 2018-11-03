//
// FirmwareDiskImage class
// (c) 2018 Eric BACHER
//

#include "FirmwareDiskImage.hpp"

// set STANDALONE to 1 to be able to test SIO commands without SIO2PC/SIO2USB device
#define STANDALONE 0

extern quint8 HAPPY_SIGNATURE[9];

FirmwareDiskImage::FirmwareDiskImage(SioWorker *worker, eHARDWARE eHardware, QString &hardwareName, Rom *rom, int iRpm, bool powerOnWithDiskInserted)
    : SimpleDiskImage(worker)
{
    m_hardwareName = hardwareName;
    m_powerOnWithDiskInserted = powerOnWithDiskInserted;
    switch (eHardware) {
    case HARDWARE_810:
    case HARDWARE_810_WITH_THE_CHIP:
        m_atariDrive = new Atari810(rom, eHardware, iRpm);
        break;
    case HARDWARE_810_WITH_HAPPY:
        m_atariDrive = new Atari810Happy(rom, eHardware, iRpm);
        break;
    case HARDWARE_1050:
    case HARDWARE_1050_WITH_THE_ARCHIVER:
        m_atariDrive = new Atari1050(rom, eHardware, iRpm);
        break;
    case HARDWARE_1050_WITH_HAPPY:
        m_atariDrive = new Atari1050Happy(rom, eHardware, iRpm);
        break;
    case HARDWARE_1050_WITH_SPEEDY:
        m_atariDrive = new Atari1050Speedy(rom, eHardware, iRpm);
        break;
    case HARDWARE_1050_WITH_TURBO:
        m_atariDrive = new Atari1050Turbo(rom, eHardware, iRpm);
        break;
    case HARDWARE_1050_WITH_DUPLICATOR:
        m_atariDrive = new Atari1050Duplicator(rom, eHardware, iRpm);
        break;
    default:
        break;
    }
    m_lastTime = 0;
    for (int i = 0; i < 40; i++) {
        m_trackInformationDisplayed[i] = false;
    }
    m_initializing = true;
	m_traceOn = false;
	m_traceFile = NULL;
}

FirmwareDiskImage::~FirmwareDiskImage()
{
    setTrace(false);
    m_atariDrive->TurnOn(false);
    delete m_atariDrive;
}

void FirmwareDiskImage::setDiskImager(FirmwareDiskImage *diskImager)
{
    m_atariDrive->SetDiskImager(diskImager);
}

bool FirmwareDiskImage::IsDisplayDriveHead(void)
{
    return m_displayDriveHead;
}

void FirmwareDiskImage::SetDisplayDriveHead(bool active)
{
    m_displayDriveHead = active;
}

bool FirmwareDiskImage::IsDisplayFdcCommands(void)
{
    return m_displayFdcCommands;
}

void FirmwareDiskImage::SetDisplayFdcCommands(bool active)
{
    m_displayFdcCommands = active;
}

bool FirmwareDiskImage::IsDisplayIndexPulse(void)
{
    return m_displayIndexPulse;
}

void FirmwareDiskImage::SetDisplayIndexPulse(bool active)
{
    m_displayIndexPulse = active;
}

bool FirmwareDiskImage::IsDisplayMotorOnOff(void)
{
    return m_displayMotorOnOff;
}

void FirmwareDiskImage::SetDisplayMotorOnOff(bool active)
{
    m_displayMotorOnOff = active;
}

bool FirmwareDiskImage::IsDisplayIDAddressMarks(void)
{
    return m_displayIDAddressMarks;
}

void FirmwareDiskImage::SetDisplayIDAddressMarks(bool active)
{
    m_displayIDAddressMarks = active;
}

bool FirmwareDiskImage::IsDisplayTrackInformation(void)
{
    return m_displayTrackInformation;
}

void FirmwareDiskImage::SetDisplayTrackInformation(bool active)
{
    m_displayTrackInformation = active;
}

bool FirmwareDiskImage::IsDisplayCpuInstructions(void)
{
    return m_displayCpuInstructions;
}

void FirmwareDiskImage::SetDisplayCpuInstructions(bool active)
{
    m_displayCpuInstructions = active;
}

QString FirmwareDiskImage::GetTraceFilename(void)
{
    return m_traceFilename;
}

void FirmwareDiskImage::SetTraceFilename(QString filename)
{
	// close the previous file before changing
	setTrace(false);
    m_traceFilename = filename;
}

bool FirmwareDiskImage::open(const QString &fileName, FileTypes::FileType type)
{
    setDiskImager(this);
    if (! m_powerOnWithDiskInserted) {
        m_atariDrive->PowerOn();
    }
    bool ok = SimpleDiskImage::open(fileName, type);
    if (ok) {
        if (m_powerOnWithDiskInserted) {
            m_atariDrive->PowerOn();
        }
        if (m_chipOpen) {
            sendChipCode();
        }
#if STANDALONE
        handleCommand(0x24, 0x01);
        /*
        handleCommand(0x52, 0x01);
        handleCommand(0x57, 0x01);
        handleCommand(0x52, 0x01);
        handleCommand(0x46, 0xA800);
        for (quint16 i = 1; i <= 18; i++) {
            handleCommand(0x52, i);
        }
        */
#endif
    }
    m_initializing = false;
    return ok;
}

void FirmwareDiskImage::close()
{
    SimpleDiskImage::close();
}

bool FirmwareDiskImage::save()
{
    if (m_isModified) {
        m_atariDrive->FlushTrack();
    }
    return SimpleDiskImage::save();
}

bool FirmwareDiskImage::saveAs(const QString &fileName)
{
    if (m_isModified) {
        m_atariDrive->FlushTrack();
    }
    return SimpleDiskImage::saveAs(fileName);
}

void FirmwareDiskImage::setLeverOpen(bool open)
{
    SimpleDiskImage::setLeverOpen(open);
    m_atariDrive->SetReady(! open);
    // wait for 1050 to detect disk density when disk is inserted
    if ((! open) && (m_atariDrive->GetFdc()->GetFdcType() == FDC_2793)) {
        for (int i = 0; i < 4000000; i++) {
            m_atariDrive->Step();
        }
    }
}

void FirmwareDiskImage::setReady(bool bReady)
{
    bool wasReady = isReady();
    SimpleDiskImage::setReady(bReady);
    if (wasReady != bReady) {
        m_atariDrive->SetReady(bReady);
    }
}

void FirmwareDiskImage::setChipMode(bool enable)
{
    bool oldChipMode = m_chipOpen;
    eHARDWARE hardware = m_atariDrive->GetHardware();
	bool open = enable && ((hardware == HARDWARE_810_WITH_THE_CHIP) || (hardware == HARDWARE_1050_WITH_THE_ARCHIVER));
    SimpleDiskImage::setChipMode(open);
    // Once the CHIP is open, let it open until the user sends a SIO Chip close command
    if ((! m_initializing) && (! oldChipMode) && (open)) {
        sendChipCode();
        emit statusChanged(m_deviceNo);
    }
}

unsigned char FirmwareDiskImage::computeChksum(unsigned char *commandBuf, int len)
{
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += *commandBuf++;
        if (sum >= 256) {
            sum++;
            sum &= 0xff;
        }
    }
    return (unsigned char) sum;
}

QString FirmwareDiskImage::description()
{
    QString desc = QString(m_geometry.humanReadable()).append(" - ").append(hardwareName());
    if (m_chipOpen) {
        eHARDWARE hardware = m_atariDrive->GetHardware();
        if ((hardware == HARDWARE_810_WITH_THE_CHIP) || (hardware == HARDWARE_1050_WITH_THE_ARCHIVER)) {
            desc = desc.append(" - ").append(tr("CHIP mode"));
        }
    }
    return desc;
}

bool FirmwareDiskImage::setTrace(bool on)
{
	if (m_traceOn != on) {
		m_traceOn = on;
		if (on) {
			m_traceFile = new QFile(m_traceFilename);
			if (! m_traceFile->open(QFile::Append | QFile::Text)) {
                qWarning() << "!w" << tr("[%1] Can not write to file %2").arg(deviceName()).arg(m_traceFilename);
                delete m_traceFile;
				m_traceFile = NULL;
                m_traceOn = false;
			}
			else {
				m_traceStream = new QTextStream(m_traceFile);
                *m_traceStream << "\n";
			}
		}
		else if (m_traceFile != NULL) {
			m_traceFile->close();
			delete m_traceFile;
			m_traceFile = NULL;
			m_traceStream = NULL;
		}
		m_atariDrive->SetTrace(on);
	}
    return m_traceOn;
}

bool FirmwareDiskImage::writeErrorInsteadOfNak()
{
    qWarning() << "!w" << tr("[%1] Sending [ERROR] instead of [NAK] to Atari").arg(deviceName());
    return sio->port()->writeError();
}

bool FirmwareDiskImage::writeStatus(unsigned char status)
{
    qWarning() << "!w" << tr("[%1] Sending [$%2] to Atari").arg(deviceName()).arg((unsigned short) status, 2, 16, QChar('0'));
    return sio->port()->writeRawFrame(QByteArray(1, status));
}

bool FirmwareDiskImage::writeRawFrame(QByteArray data)
{
    if (m_dumpDataFrame) {
        int speed = m_atariDrive->GetTransmitSpeedInBitsPerSeconds();
        qDebug() << "!u" << tr("[%1] Sending %2 bytes to Atari at speed %3 bit/s").arg(deviceName()).arg(data.length() - 1).arg(speed);
        dumpBuffer((unsigned char *) data.data(), data.length() - 1);
    }
    return sio->port()->writeRawFrame(data);
}

QString FirmwareDiskImage::commandDescription(quint8 command, quint16 aux)
{
    int sector = aux & 0x3FF;
    int track = (sector - 1) / m_geometry.sectorsPerTrack();
    int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
    eHARDWARE hardware = m_atariDrive->GetHardware();
	if (hardware == HARDWARE_810) {
        switch (command) {
		case 0x20:	return tr("[%1] Upload and Execute Code (Revision E only) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
        case 0x4F:	return tr("[%1] Write Sector %2 ($%3) (with verify) (undocumented command)").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0'));
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
		case 0x53:	return tr("[%1] Get Status").arg(deviceName());
		case 0x54:	return tr("[%1] Get RAM Buffer (Revision E only) at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
		case 0xFF:	return tr("[%1] Execute Code (Revision E only) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_810_WITH_THE_CHIP) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x42:	return tr("[%1] Write Sector using Index with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x43:	return tr("[%1] Read All Sector Statuses").arg(deviceName());
		case 0x44:	return tr("[%1] Read Sector using Index with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x46:	return tr("[%1] Write Track with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x47:	return tr("[%1] Read Track with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x4C:	return tr("[%1] Set RAM Buffer").arg(deviceName());
        case 0x4D:	return tr("[%1] Upload and Execute Code with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x4E:	return tr("[%1] Set Shutdown Delay with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x4F:
			if (aux == 0)
					return tr("[%1] Close CHIP").arg(deviceName());
			else	return tr("[%1] Open CHIP with code %2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
		case 0x54:	return tr("[%1] Get RAM Buffer").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x5A:	return tr("[%1] Set Trace On/Off with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_810_WITH_HAPPY) {
		if (m_atariDrive->GetRom()->GetLength() < 5000) { // Happy rev 5
			switch (command) {
			case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
            case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            case 0x52:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] Read memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
                else	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            case 0x53:	return tr("[%1] Get Status").arg(deviceName());
			case 0x57:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] Write memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
                else	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			}
		}
		else { // Happy rev 7
			switch (command) {
			case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
			case 0x36:	return tr("[%1] Clear Sector Flags (Broadcast) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			case 0x37:	return tr("[%1] Get Sector Flags (Broadcast) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			case 0x38:	return tr("[%1] Send Sector (Broadcast) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			case 0x48:
				if ((aux & 0xFF) == 0x01)
						return tr("[%1] Set Idle Timeout to $%2").arg(deviceName()).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
				else if ((aux & 0xFF) == 0x02)
						return tr("[%1] Set Alternate Device ID to $%2").arg(deviceName()).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
				else if ((aux & 0xFF) == 0x03)
						return tr("[%1] Reinitialize Drive").arg(deviceName());
				else	return tr("[%1] Configure Drive with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            case 0x51:	return tr("[%1] Turn off drive motor").arg(deviceName());
			case 0x52:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] Read memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
                else	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            case 0x53:	return tr("[%1] Get Status").arg(deviceName());
			case 0x57:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] Write memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
                else	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
            case 0x70:	return tr("[%1] High Speed Put Sector $%2 (no verify)").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			case 0x72:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] High Speed Read memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
				else	return tr("[%1] High Speed Read Sector $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			case 0x77:
                if ((aux >= 0x800) && (aux <= 0x1380))
						return tr("[%1] High Speed Write memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
				else	return tr("[%1] High Speed Write Sector $%2 (with verify)").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			}
		}
	}
	else if (hardware == HARDWARE_1050) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x23:	return tr("[%1] Run Speed Diagnostic with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x24:	return tr("[%1] Run Diagnostic with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x3F:	return tr("[%1] Poll Speed").arg(deviceName());
		case 0x42:	return tr("[%1] Write Sector using Index with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x43:	return tr("[%1] Read All Sector Statuses").arg(deviceName());
		case 0x44:	return tr("[%1] Read Sector using Index with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x46:	return tr("[%1] Write Track with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x47:	return tr("[%1] Read Track (128 bytes) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x4C:	return tr("[%1] Set RAM Buffer").arg(deviceName());
		case 0x4E:	return tr("[%1] Get PERCOM Block").arg(deviceName());
		case 0x4F:
			if (aux < 0x1000)
					return tr("[%1] Set PERCOM Block").arg(deviceName());
			else	return tr("[%1] Open CHIP with code %2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
		case 0x54:	return tr("[%1] Get RAM Buffer").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6 with AUX=$%7").arg(deviceName()).arg(sector).arg(sector, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector).arg(aux, 4, 16, QChar('0'));
        case 0x5A:	return tr("[%1] Set Trace On/Off with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x62:	return tr("[%1] Write Fuzzy Sector using Index with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x66:	return tr("[%1] Format with Custom Sector Skewing with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x67:	return tr("[%1] Read Track (256 bytes) with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x71:	return tr("[%1] Write Fuzzy Sector $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
		case 0x73:	return tr("[%1] Set Speed with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
        case 0x74:	return tr("[%1] Read Memory").arg(deviceName());
        case 0x75:	return tr("[%1] Upload and Execute Code with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_1050_WITH_HAPPY) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x23:	return tr("[%1] Run Speed Diagnostic with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x24:	return tr("[%1] Run Diagnostic with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x3F:	return tr("[%1] Poll Speed").arg(deviceName());
		case 0x48:
			if ((aux & 0xFF) == 0x01)
					return tr("[%1] Set Idle Timeout to $%2").arg(deviceName()).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			else if ((aux & 0xFF) == 0x02)
					return tr("[%1] Set Alternate Device ID to $%2").arg(deviceName()).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
			else if ((aux & 0xFF) == 0x03)
					return tr("[%1] Reinitialize Drive").arg(deviceName());
			else	return tr("[%1] Configure Drive with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x50:
			if (aux & 0x8000)
					return tr("[%1] Write memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
            else	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x51:	return tr("[%1] Turn off drive motor").arg(deviceName());
		case 0x52:
			if (aux & 0x8000)
					return tr("[%1] Read memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
            else	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
		case 0x57:
			if (aux & 0x8000)
					return tr("[%1] Write memory at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
            else	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x70:
			if (aux & 0x8000)
					return tr("[%1] High Speed Write memory at $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			else	return tr("[%1] High Speed Put Sector $%2 (no verify)").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
		case 0x72:
			if (aux & 0x8000)
					return tr("[%1] High Speed Read memory at $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			else	return tr("[%1] High Speed Read Sector $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
		case 0x77:
			if (aux & 0x8000)
					return tr("[%1] High Speed Write memory at $%2").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
			else	return tr("[%1] High Speed Write Sector $%2 (with verify)").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
		   
		default: return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_1050_WITH_SPEEDY) {
		switch (command) {
		case 0x20:	return tr("[%1] Format Disk Asynchronously with AUX1=$%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x3F:	return tr("[%1] Poll Speed").arg(deviceName());
		case 0x41:	return tr("[%1] Add or Delete a Command").arg(deviceName());
		case 0x44:	return tr("[%1] Configure Drive and Display with AUX1=$%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x4B:	return tr("[%1] Configure Slow/Fast Speed with AUX1=$%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x4C:	return tr("[%1] Jump to Address $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x4D:	return tr("[%1] Jump to Address $%2 with Acknowledge").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x4E:	return tr("[%1] Get PERCOM Block").arg(deviceName());
		case 0x4F:	return tr("[%1] Set PERCOM Block").arg(deviceName());
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x51:	return tr("[%1] Flush Write").arg(deviceName());
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x60:	return tr("[%1] Write Track at Address $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x62:	return tr("[%1] Read Track at Address $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x68:	return tr("[%1] Get SIO Routine size").arg(deviceName());
		case 0x69:	return tr("[%1] Get SIO Routine relocated at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_1050_WITH_TURBO) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x23:	return tr("[%1] Service Put").arg(deviceName());
		case 0x24:	return tr("[%1] Service Get").arg(deviceName());
		case 0x4E:	return tr("[%1] Get PERCOM Block").arg(deviceName());
		case 0x4F:	return tr("[%1] Set PERCOM Block").arg(deviceName());
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	else if (hardware == HARDWARE_1050_WITH_DUPLICATOR) {
		switch (command) {
		case 0x21:	return tr("[%1] Format Single Density Disk").arg(deviceName());
		case 0x22:	return tr("[%1] Format Enhanced Density Disk").arg(deviceName());
		case 0x3F:	return tr("[%1] Poll Speed").arg(deviceName());
		case 0x4E:	return tr("[%1] Get PERCOM Block").arg(deviceName());
		case 0x4F:	return tr("[%1] Set PERCOM Block").arg(deviceName());
        case 0x50:	return tr("[%1] Put Sector %2 ($%3) (no verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x52:	return tr("[%1] Read Sector %2 ($%3) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x53:	return tr("[%1] Get Status").arg(deviceName());
        case 0x57:	return tr("[%1] Write Sector %2 ($%3) (with verify) in track %4 ($%5) sector #%6").arg(deviceName()).arg(aux).arg(aux, 3, 16, QChar('0')).arg(track).arg(track, 2, 16, QChar('0')).arg(relativeSector);
        case 0x61:	return tr("[%1] Analyze Track $%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
        case 0x62:	return tr("[%1] Set Drive buffer Mode: ").arg(deviceName()).arg((aux & 0xFF) == 0 ? tr("Buffered") : tr("Unbuffered"));
		case 0x63:	return tr("[%1] Custom Track Format with $%2 bytes").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
        case 0x64:	return tr("[%1] Set Density Sensing: ").arg(deviceName()).arg((aux & 0xFF) == 0 ? tr("Enabled") : tr("Disabled"));
		case 0x66:	return tr("[%1] Format with Custom Sector Skewing with AUX1=$%2 and AUX2=$%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x67:	return tr("[%1] Seek to Track $%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x68:	return tr("[%1] Change Drive Hold on Time to %2 tenths of seconds").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x6A:	return tr("[%1] Change Drive RPM: %2").arg(deviceName()).arg((aux & 0xFF) == 0 ? 288 : 272);
		case 0x6D:	return tr("[%1] Upload Sector Pattern and Read Track $%2").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0'));
		case 0x6E:	return tr("[%1] Upload Sector Pattern and Write Buffer to Disk").arg(deviceName());
		case 0x72:	return tr("[%1] Run Uploaded Program at Address $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x73:	return tr("[%1] Set Load Address at $%2").arg(deviceName()).arg(aux, 4, 16, QChar('0'));
		case 0x74:	return tr("[%1] Get Sector Data").arg(deviceName());
		case 0x75:	return tr("[%1] Upload Data to Drive").arg(deviceName());
		case 0x76:	return tr("[%1] Upload Sector Data in Buffer $%2 with Status $%3").arg(deviceName()).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		case 0x77:	return tr("[%1] Write Sector $%2 with Deleted Data").arg(deviceName()).arg(aux, 3, 16, QChar('0'));
		default:	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
		}
	}
	return tr("[%1] Command $%2 with AUX1=$%3 and AUX2=$%4").arg(deviceName()).arg((command & 0xFF), 2, 16, QChar('0')).arg((aux & 0xFF), 2, 16, QChar('0')).arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
}

void FirmwareDiskImage::dumpFDCCommand(const char *commandName, unsigned int command, unsigned int track, unsigned int sector, unsigned int data, int currentRotation, int maxRotation)
{
    if ((m_displayFdcCommands) || ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL))) {
		QString str = tr("[%1] [Rotation=%2/%3] FDC command %4 (Cmd=$%5 Trk=$%6 Sec=$%7 Data=$%8)")
                    .arg(deviceName())
                    .arg(currentRotation)
                    .arg(maxRotation)
                    .arg(commandName)
                    .arg(command, 2, 16, QChar('0'))
                    .arg(track, 2, 16, QChar('0'))
                    .arg(sector, 2, 16, QChar('0'))
                    .arg(data, 2, 16, QChar('0'));
		if (m_displayFdcCommands) {
			qDebug() << "!u" << str;
		}
		if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
            *m_traceStream << str << "\n";
		}
    }
}

void FirmwareDiskImage::dumpIndexPulse(bool indexPulse, int currentRotation, int maxRotation)
{
    if ((m_displayIndexPulse) || ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL))) {
		QString str;
        if (indexPulse) {
            str = tr("[%1] [Rotation=%2/%3] Index Pulse").arg(deviceName()).arg(currentRotation).arg(maxRotation);
        }
        else {
            str = tr("[%1] [Rotation=%2/%3] End of Index Pulse").arg(deviceName()).arg(currentRotation).arg(maxRotation);
        }
		if (m_displayIndexPulse) {
			qDebug() << "!u" << str;
		}
		if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
            *m_traceStream << str << "\n";
		}
	}
}

void FirmwareDiskImage::dumpMotorOnOff(bool motorOn, int currentRotation, int maxRotation)
{
    if ((m_displayMotorOnOff) || ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL))) {
		QString str;
        if (motorOn) {
            str = tr("[%1] [Rotation=%2/%3] Motor is turned ON").arg(deviceName()).arg(currentRotation).arg(maxRotation);
        }
        else {
            str = tr("[%1] [Rotation=%2/%3] Motor is turned off").arg(deviceName()).arg(currentRotation).arg(maxRotation);
        }
		if (m_displayMotorOnOff) {
            qDebug() << "!u" << str;
		}
		if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
            *m_traceStream << str << "\n";
        }
	}
}

void FirmwareDiskImage::dumpIDAddressMarks(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, unsigned short crc, int currentRotation, int maxRotation)
{
    if ((m_displayIDAddressMarks) || ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL))) {
        QString str = tr("[%1] [Rotation=%2/%3] ID address mark found (Trk=$%4 Side=$%5 Sec=$%6 Len=$%7 Crc=$%8)")
                    .arg(deviceName())
                    .arg(currentRotation)
                    .arg(maxRotation)
                    .arg(track, 2, 16, QChar('0'))
                    .arg(side, 2, 16, QChar('0'))
                    .arg(sector, 2, 16, QChar('0'))
                    .arg(size, 2, 16, QChar('0'))
                    .arg(crc, 4, 16, QChar('0'));
		if (m_displayIDAddressMarks) {
            qDebug() << "!u" << str;
		}
		if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
            *m_traceStream << str << "\n";
        }
	}
}

void FirmwareDiskImage::dumpDataAddressMark(unsigned int track, unsigned int side, unsigned int sector, unsigned int size, int currentRotation, int maxRotation)
{
    if ((m_displayIDAddressMarks) || ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL))) {
        QString str = tr("[%1] [Rotation=%2/%3] Data address mark found (Trk=$%4 Side=$%5 Sec=$%6 Len=$%7) in read command")
                    .arg(deviceName())
                    .arg(currentRotation)
                    .arg(maxRotation)
                    .arg(track, 2, 16, QChar('0'))
                    .arg(side, 2, 16, QChar('0'))
                    .arg(sector, 2, 16, QChar('0'))
                    .arg(size, 2, 16, QChar('0'));
		if (m_displayIDAddressMarks) {
            qDebug() << "!u" << str;
		}
		if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
            *m_traceStream << str << "\n";
        }
	}
}

void FirmwareDiskImage::dumpReadSector(unsigned int track, unsigned int, unsigned int sector, unsigned int, unsigned char status, int currentRotation)
{
    m_lastReadSectorTrack = track;
    m_lastReadSectorNumber = sector;
    m_lastReadSectorStatus = status;
    m_lastReadSectorRotation = currentRotation;
}

void FirmwareDiskImage::displayReadSectorStatus(unsigned int trackNumber, Track *track, unsigned int sectorNumber, unsigned char status, int currentRotation, int maxRotation)
{
    if (! m_displayIDAddressMarks) {
        bool display = status != 0xFF ? true : false;
        int nbPhantoms = 0;
        int phantomIndex = 0;
        // check if there are duplicate sectors
        int bitNumber = track->FindNextIdAddressMark(0);
        Crc16 crc;
        while (bitNumber != -1) {
            crc.Reset();
            unsigned char clock;
            unsigned char header[7];
            unsigned char weak;
            int headerBitNumber = bitNumber;
            for (unsigned int i = 0; i < sizeof(header); i++) {
                bitNumber = track->ReadRawByte(bitNumber, &clock, &header[i], &weak);
                if (i < sizeof(header) - 2) {
                    crc.Add(header[i]);
                }
            }
            unsigned short headerCrc = (((unsigned short)header[5]) << 8) | (((unsigned short)header[6]) & 0xFF);
            if ((header[1] == (unsigned char)trackNumber) && (headerCrc == crc.GetCrc()) && (header[3] == (unsigned char)sectorNumber)) {
                nbPhantoms++;
                if (headerBitNumber == currentRotation) {
                    phantomIndex = nbPhantoms;
                }
            }
            bitNumber = track->FindNextIdAddressMark(bitNumber + 8);
        }
        if (nbPhantoms > 1) {
            display = true;
        }
        if (display) {
            if (status != 0xFF) {
                if ((status & 0x10) == 0) {
                    qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Bad sector (status $%4) Grrr Grrr !")
                                .arg(deviceName())
                                .arg(currentRotation)
                                .arg(maxRotation)
                                .arg(status, 2, 16, QChar('0'));
                }
                else if ((status & 0x20) == 0) {
                    qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Deleted sector (status $%4)")
                                .arg(deviceName())
                                .arg(currentRotation)
                                .arg(maxRotation)
                                .arg(status, 2, 16, QChar('0'));
                }
                else if ((status & 0x08) == 0) {
                    quint16 weakOffset = 0xFFFF; // sectorInfo->sectorWeakOffset();
                    if (weakOffset != 0xFFFF) {
                        qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Weak sector at offset %4")
                                    .arg(deviceName())
                                    .arg(currentRotation)
                                    .arg(maxRotation)
                                    .arg(weakOffset);
                    }
                    else if (nbPhantoms > 1) {
                        if ((status & 0x08) == 0) {
                            qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] CRC error (status $%4) on sector index #%5 among %6 phantom sectors")
                                        .arg(deviceName())
                                        .arg(currentRotation)
                                        .arg(maxRotation)
                                        .arg(status, 2, 16, QChar('0'))
                                        .arg(phantomIndex)
                                        .arg(nbPhantoms);
                        }
                        else {
                            qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Read sector index #%4 among %5 phantom sectors")
                                        .arg(deviceName())
                                        .arg(currentRotation)
                                        .arg(maxRotation)
                                        .arg(phantomIndex)
                                        .arg(nbPhantoms);
                        }
                    }
                    else if ((status & 0x08) == 0) {
                        qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] CRC error (status $%4)")
                                    .arg(deviceName())
                                    .arg(currentRotation)
                                    .arg(maxRotation)
                                    .arg(status, 2, 16, QChar('0'));
                    }
                }
                else {
                    qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Read status $%4")
                                .arg(deviceName())
                                .arg(currentRotation)
                                .arg(maxRotation)
                                .arg(status, 2, 16, QChar('0'));
                }
            }
            else if (nbPhantoms > 1) {
                qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Read sector index #%4 among %5 phantom sectors")
                            .arg(deviceName())
                            .arg(currentRotation)
                            .arg(maxRotation)
                            .arg(phantomIndex)
                            .arg(nbPhantoms);
            }
        }
    }
}

void FirmwareDiskImage::dumpCPUInstructions(const char *buf, int currentRotation, int maxRotation)
{
    if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
        *m_traceStream << tr("[%1] [Rotation=%2/%3] [Bank=%4] %5")
                    .arg(deviceName())
                    .arg(currentRotation)
                    .arg(maxRotation)
                    .arg(m_atariDrive->GetBankNumber())
                    .arg(buf) << "\n";
	}
}

void FirmwareDiskImage::dumpCommandToFile(QString &command)
{
    if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
        *m_traceStream << command << "\n";
    }
}

//  old style debug API...
void FirmwareDiskImage::debugMessage(const char *msg, ...)
{
    static char buf[512];
    va_list args;
    va_start(args, msg);
    vsprintf(buf, msg, args);
    qDebug() << "!u" << tr("[%1] %2").arg(deviceName()).arg(buf);
}

void FirmwareDiskImage::dumpDataToFile(QByteArray &data)
{
    if ((m_displayCpuInstructions) && (m_traceOn) && (m_traceStream != NULL)) {
		int len = data.size();
        *m_traceStream << tr("[%1] Receiving %2 bytes from Atari").arg(deviceName()).arg(len) << "\n";
		for (int i = 0; i < ((len + 15) >> 4); i++) {
			char line[80];
			int ofs = i << 4;
            fillBuffer(line, (unsigned char *)data.data(), len, ofs, false);
            *m_traceStream << tr("[%1] %2").arg(deviceName()).arg(line) << "\n";
		}
	}
}

void FirmwareDiskImage::disassembleBuffer(unsigned char *data, int len, unsigned short address)
{
	// CRC16 is not a good hash but this is only for very few "upload and execute code" commands so this is OK
	// This CRC is used to implement these commands at SIO level in diskimage.cpp.
	// This is useful to make Fuzzy sectors with Super Archiver when hardware emulation level.
	Crc16 crc;
	crc.Reset();
    for (int i = 0; i < len; i++) {
        crc.Add(data[i]);
	}
    qDebug() << "!n" << tr("[%1] Disassembly of %2 bytes at $%3 with CRC $%4")
                .arg(deviceName())
                .arg(len)
                .arg(address, 4, 16, QChar('0'))
                .arg(crc.GetCrc(), 4, 16, QChar('0'));
    char buf[256];
	int offset = 0;
    if ((address == 0x0800) && (hasHappySignature())) {
        while (offset < 9) {
            qDebug() << "!u" << tr("[%1] §%2:%3 %4 %5 ; Happy signature")
                        .arg(deviceName())
                        .arg((address + (unsigned short)offset), 4, 16, QChar('0'))
                        .arg(data[offset], 2, 16, QChar('0'))
                        .arg(data[offset + 1], 2, 16, QChar('0'))
                        .arg(data[offset + 2], 2, 16, QChar('0'));
            offset += 3;
        }
        int command = 0x50;
        while (offset < 32) {
            qDebug() << "!u" << tr("[%1] §%2:%3 %4 %5 JMP $%6 ; Command $%7")
                        .arg(deviceName())
                        .arg((address + (unsigned short)offset), 4, 16, QChar('0'))
                        .arg(data[offset], 2, 16, QChar('0'))
                        .arg(data[offset + 1], 2, 16, QChar('0'))
                        .arg(data[offset + 2], 2, 16, QChar('0'))
                        .arg((data[offset + 1] + (data[offset + 2] << 8)), 4, 16, QChar('0'))
                        .arg(command, 2, 16, QChar('0'));
            offset += 3;
            command++;
        }
    }
	while (offset < len) {
		int lenOpCode = m_atariDrive->BuildInstruction(buf, &data[offset], len - offset, address + (unsigned short)offset);
        if (lenOpCode == -1) {
			break;
		}
		else {
	        qDebug() << "!u" << tr("[%1] §%2")
	                    .arg(deviceName())
	                    .arg(buf);
		}
        offset += lenOpCode;
	}
	// TODO save remaining bytes to disassemble them with the next upload command if address is consecutive
}

void FirmwareDiskImage::displayRawTrack(int trackNumber, Track *track)
{
    qDebug() << "!u" << tr("[%1] Start of track $%2 raw dump")
                .arg(deviceName())
                .arg(trackNumber, 2, 16, QChar('0'));
    unsigned char weak;
    Crc16 crc;
    int previousBitNumber = 0;
    int bitNumber = track->FindNextIdAddressMark(0);
    while (bitNumber != -1) {
        crc.Reset();
        unsigned char clock;
        unsigned char header[7];
        int headerBitNumber = bitNumber;
        for (unsigned int i = 0; i < sizeof(header); i++) {
            headerBitNumber = track->ReadRawByte(headerBitNumber, &clock, &header[i], &weak);
            if (i < sizeof(header) - 2) {
                crc.Add(header[i]);
            }
        }
        unsigned short headerCrc = (((unsigned short)header[5]) << 8) | (((unsigned short)header[6]) & 0xFF);
        if ((header[1] == (unsigned char)trackNumber) && (headerCrc == crc.GetCrc())) {
            int nbBytes = (bitNumber - previousBitNumber) >> 3;
			if (nbBytes > 0) {
                unsigned char buf[nbBytes << 1];
                qDebug() << "!u" << tr("[%1] dump at position %2 up until %3")
                            .arg(deviceName())
                            .arg(previousBitNumber)
							.arg(bitNumber);
                for (int i = 0; i < nbBytes; i++) {
                    previousBitNumber = track->ReadRawByte(previousBitNumber, &buf[i << 1], &buf[(i << 1) + 1], &weak);
                }
                dumpBuffer(buf, nbBytes << 1);
            }
            crc.Reset();
            int maxBitIndex = headerBitNumber + (30 << 3); //track->AddBytesToIndex(headerBitNumber, 30);
            int dataBitNumber = track->FindNextDataAddressMark(headerBitNumber);
            bool crcError = true;
            int weakOffset = 0xFFFF;
            if (dataBitNumber > maxBitIndex) {
                dataBitNumber = -1;
            }
            else {
                int nbDataBytes = 128 << (header[4] & 0x03);
                unsigned char data[nbDataBytes + 2]; // data
                unsigned char dataMark;
                dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &dataMark, &weak);
                crc.Add(dataMark);
                for (unsigned int i = 0; i < sizeof(data); i++) {
                    dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &data[i], &weak);
                    if ((weak != 0) && (weakOffset == 0xFFFF)) {
                        weakOffset = i;
                    }
                    if (i < sizeof(data) - 2) {
                        data[i] = 0xFF - crc.Add(data[i]);
                    }
                }
                unsigned short dataCrc = (((unsigned short)data[sizeof(data) - 2]) << 8) | (((unsigned short)data[sizeof(data) - 1]) & 0xFF);
                crcError = (dataCrc != crc.GetCrc());
            }
            qDebug() << "!u" << tr("[%1] header at %2 bits (%3 bytes): $%4 $%5 $%6 $%7 $%8 $%9 $%10 data at %11 bits (%12 bytes) crc %13 weak %14")
                        .arg(deviceName())
                        .arg(bitNumber)
                        .arg((bitNumber >> 3))
                        .arg(header[0], 2, 16, QChar('0'))
                        .arg(header[1], 2, 16, QChar('0'))
                        .arg(header[2], 2, 16, QChar('0'))
                        .arg(header[3], 2, 16, QChar('0'))
                        .arg(header[4], 2, 16, QChar('0'))
                        .arg(header[5], 2, 16, QChar('0'))
                        .arg(header[6], 2, 16, QChar('0'))
                        .arg(dataBitNumber)
                        .arg(dataBitNumber >> 3)
                        .arg(crcError ? "ERROR" : "Ok")
                        .arg((weakOffset == 0xFFFF) ? "No" : tr("at %1 byte").arg(weakOffset));
        }
        previousBitNumber = bitNumber;
        bitNumber = track->FindNextIdAddressMark(bitNumber + 8);
    }
    bitNumber = track->GetReadDataBitsPerRotation();
    int nbBytes = (bitNumber - previousBitNumber) >> 3;
	if (nbBytes > 0) {
		unsigned char buf[nbBytes << 1];
        qDebug() << "!u" << tr("[%1] dump at position %2 up until %3")
					.arg(deviceName())
					.arg(previousBitNumber)
					.arg(bitNumber);
		for (int i = 0; i < nbBytes; i++) {
            previousBitNumber = track->ReadRawByte(previousBitNumber, &buf[i << 1], &buf[(i << 1) + 1], &weak);
		}
		dumpBuffer(buf, nbBytes << 1);
	}
    qDebug() << "!u" << tr("[%1] End of track $%2 raw dump")
                .arg(deviceName())
                .arg(trackNumber, 2, 16, QChar('0'));
}

QString FirmwareDiskImage::convertWd1771Status(quint8 status, quint16 weakBits, quint8 shortSize, int bitShift)
{
    QString description("");
    if (status == 0xFF) {
        description.append(tr("Normal"));
    }
    else if (status == 0xE7) {
        description.append(tr("ID CRC error"));
    }
    else if (status == 0xEF) {
        description.append(tr("Record not found"));
    }
    else if (status == 0xF7) {
        description.append(tr("Data CRC error"));
    }
    else if (status == 0xF9) {
        description.append(tr("Long sector"));
    }
    else if (status == 0xF1) {
        description.append(tr("Long sector + Data CRC error"));
    }
    else if (status == 0xDF) {
        description.append(tr("Deleted sector"));
    }
    else if (status == 0xD7) {
        description.append(tr("Deleted sector + Data CRC error"));
    }
    else if (status == 0xD9) {
        description.append(tr("Deleted long sector"));
    }
    else if (status == 0xD1) {
        description.append(tr("Deleted long sector + Data CRC error"));
    }
    else {
        if ((status & 0x20) == 0) {
            description.append(tr("Deleted sector"));
        }
        if ((status & 0x10) == 0) {
            if (description.size() > 0) {
                description.append(" + ");
            }
            description.append(tr("Record not found"));
        }
        if ((status & 0x08) == 0) {
            if (description.size() > 0) {
                description.append(" + ");
            }
            description.append(tr("CRC error"));
        }
        if ((status & 0x04) == 0) {
            if (description.size() > 0) {
                description.append(" + ");
            }
            description.append(tr("Lost data"));
        }
        if ((status & 0x02) == 0) {
            if (description.size() > 0) {
                description.append(" + ");
            }
            description.append(tr("DRQ"));
        }
    }
    if (weakBits != 0xFFFF) {
        if (description.size() > 0) {
            description.append(" + ");
        }
        description.append(tr("Weak bits at byte %1").arg(weakBits));
    }
    if (shortSize != 0) {
        if (description.size() > 0) {
            description.append(" + ");
        }
        if (bitShift != 0) {
        	description.append(tr("Next header at byte %1 with bit shift %2").arg(shortSize).arg(bitShift));
        }
        else {
        	description.append(tr("Next header at byte %1").arg(shortSize));
        }
    }
    return description;
}

int FirmwareDiskImage::getExpectedDataFrameSize(quint8 command, quint16 aux)
{
    eHARDWARE hardware = m_atariDrive->GetHardware();
    if (hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
        if (command == 0x46) {
            return 128;
        }
        if (command == 0x4C) {
            return 128;
        }
        if ((command == 0x4F) && (aux < 0x1000)) {
            return 12;
        }
        if (command == 0x75) {
            return 256;
        }
    }
    else if (hardware == HARDWARE_1050_WITH_SPEEDY) {
        if (command == 0x41) {
            return 3;
        }
        if (command == 0x4F) {
            return 12;
        }
        if (command == 0x60) {
            return m_geometry.bytesPerSector() * m_geometry.sectorsPerTrack();
        }
    }
    else if (hardware == HARDWARE_1050_WITH_TURBO) {
        if (command == 0x23) {
            return 128;
        }
        if (command == 0x4F) {
            return 12;
        }
    }
    else if (hardware == HARDWARE_1050_WITH_DUPLICATOR) {
        if (command == 0x63) {
            return 1;
        }
        if (command == 0x4F) {
            return 12;
        }
        if (command == 0x6D) {
            return 128;
        }
        if (command == 0x6E) {
            return 128;
        }
        if (command == 0x75) {
            return 256;
        }
    }
    return m_geometry.bytesPerSector(aux);
}

void FirmwareDiskImage::sendChipCode()
{
    int code;
    if (m_atariDrive->GetHardware() == HARDWARE_810_WITH_THE_CHIP) {
        code = (unsigned short)m_atariDrive->GetRom()->ReadRegister(0) | ((unsigned short)m_atariDrive->GetRom()->ReadRegister(1) << 8);
    }
    else if (m_atariDrive->GetHardware() == HARDWARE_1050_WITH_THE_ARCHIVER) {
        code = 0xABCD;
        unsigned char cmd[5];
        cmd[0] = (unsigned char) 0x31;
        cmd[1] = (unsigned char) 0x3F;
        cmd[2] = (unsigned char) 0x00;
        cmd[3] = (unsigned char) 0x00;
        cmd[4] = computeChksum(cmd, 4);
        unsigned char status = m_atariDrive->ReceiveSioCommandFrame(cmd);
        if (status == STATUS_ACK) {
            status = m_atariDrive->ExecuteSioCommandFrame();
        }
        m_atariDrive->RunForCycles(1000L);
        if (status != STATUS_COMPLETE) {
            qWarning() << "!w" << tr("[%1] Error while polling speed before opening the Chip with code %2")
                        .arg(deviceName())
                        .arg(code, 2, 16, QChar('0'));
            return;
        }
    }
    else {
        return;
    }
    unsigned char cmd[5];
    cmd[0] = (unsigned char) 0x31;
    cmd[1] = (unsigned char) 0x4F;
    cmd[2] = (unsigned char) (code & 0xFF);
    cmd[3] = (unsigned char) ((code >> 8) & 0xFF);
    cmd[4] = computeChksum(cmd, 4);
    unsigned char status = m_atariDrive->ReceiveSioCommandFrame(cmd);
    if (status == STATUS_ACK) {
        status = m_atariDrive->ExecuteSioCommandFrame();
        if (status == STATUS_COMPLETE) {
            qDebug() << "!n" << tr("[%1] Chip successfully open with code %2")
                        .arg(deviceName())
                        .arg(code, 2, 16, QChar('0'));
            return;
        }
    }
    qWarning() << "!w" << tr("[%1] Error while opening the Chip with code %2")
                .arg(deviceName())
                .arg(code, 2, 16, QChar('0'));
    m_atariDrive->RunForCycles(1000L);
}

int FirmwareDiskImage::getUploadCodeStartAddress(quint8 command, quint16 aux, QByteArray &data)
{
    eHARDWARE hardware = m_atariDrive->GetHardware();
    if (hardware == HARDWARE_810) { // revision E
        if (command == 0x20) {
            return 0x0080;
        }
        else if (command == 0xFF) {
            return 0x0180;
        }
    }
    else if (hardware == HARDWARE_810_WITH_THE_CHIP) {
        if (command == 0x4D) {
            return 0x0080;
        }
    }
    else if (hardware == HARDWARE_810_WITH_HAPPY) {
        if ((command == 0x57) && (aux >= 0x800) && (aux < 0x1380)) {
            if (aux == 0x0800) {
                if (m_happyRam.size() == 0) {
                    m_happyRam.reserve(sizeof(HAPPY_SIGNATURE));
                }
                for (unsigned int i = 0; i < sizeof(HAPPY_SIGNATURE); i++) {
                    m_happyRam[aux - 0x800 + i] = data[i];
                }
            }
            if (hasHappySignature()) {
                return (int) aux;
            }
		}
        if (m_atariDrive->GetRom()->GetLength() > 5000) { // Happy rev 7 only
            if ((command == 0x77) && (aux >= 0x800) && (aux < 0x1380)) {
                if (aux == 0x0800) {
                    if (m_happyRam.size() == 0) {
                        m_happyRam.reserve(sizeof(HAPPY_SIGNATURE));
                    }
                    for (unsigned int i = 0; i < sizeof(HAPPY_SIGNATURE); i++) {
                        m_happyRam[aux - 0x800 + i] = data[i];
                    }
                }
                if (hasHappySignature()) {
                    return (int) aux;
                }
			}
        }
    }
    else if (hardware == HARDWARE_1050_WITH_THE_ARCHIVER) {
        if (command == 0x75) {
            return 0x1000;
        }
	}
    else if (hardware == HARDWARE_1050_WITH_HAPPY) {
        if (((command == 0x50) || (command == 0x57) || (command == 0x70) || (command == 0x77)) && (aux >= 0x8000) && (aux <= 0x97FF)) {
            return (int) aux;
        }
    }
	return -1;
}

#if STANDALONE
void FirmwareDiskImage::handleCommand(quint8 command, quint16 aux)
{
    setDiskImager(this);
    setTrace(false);
    unsigned char cmd[5];
    cmd[0] = (unsigned char) 0x31;
    cmd[1] = (unsigned char) (command & 0xFF);
    cmd[2] = (unsigned char) (aux & 0xFF);
    cmd[3] = (unsigned char) ((aux >> 8) & 0xFF);
    cmd[4] = computeChksum(cmd, 4);
    unsigned char status = m_atariDrive->ReceiveSioCommandFrame(cmd);
	QString commandDesc = commandDescription(command, aux);
	qDebug() << "!n" << commandDesc;
    if (status == STATUS_ACK) {
        // execute the command
        bool displayCpu = (m_displayCpuInstructions) && (m_atariDrive->GetRom()->IsCommandActive(command, aux));
        displayCpu = setTrace(displayCpu);
        if (displayCpu) {
            dumpCommandToFile(commandDesc);
        }
        status = m_atariDrive->ExecuteSioCommandFrame();
        if (status == WAIT_FOR_DATA) {
            QByteArray data(128, 0);
            data[0] = 23;
            data[1] = 17; data[2] = 15; data[3] = 13; data[4] = 11; data[5] = 9; data[6] = 7; data[7] = 5; data[8] = 3; data[9] = 1;
            data[10] = 18; data[11] = 16; data[12] = 14; data[13] = 12; data[14] = 10; data[15] = 8; data[16] = 6; data[17] = 4; data[18] = 2;
            data[19] = 18; data[20] = 16; data[21] = 14; data[22] = 12; data[23] = 10;
            data[57] = 0x97; data[58] = 0x95; data[59] = 0x93; data[60] = 0x91; data[61] = 0x89; data[62] = 0x87; data[63] = 0x85; data[64] = 0x83; data[65] = 0x81;
            data[66] = 0x98; data[67] = 0x96; data[68] = 0x94; data[69] = 0x92; data[70] = 0x90; data[71] = 0x88; data[72] = 0x86; data[73] = 0x84; data[74] = 0x82;
            data[75] = 0xA8; data[76] = 0xA6; data[77] = 0xA4; data[78] = 0xA2; data[79] = 0xA0;
            for (int i = 85; i < 85 + 24; i++) {
                data[i] = 80;
            }
            data[85 + 8] = 128;
            data[113] = 11; data[114] = 6; data[115] = 9; data[116] = 17;
            data[118] = 1;
            data.append(computeChksum((unsigned char *) data.data(), data.size()));
			setTrace(displayCpu);
            status = m_atariDrive->ReceiveSioDataFrame((unsigned char *) data.data(), data.size());
			setTrace(false);
            if (status == 0x43) {
                qDebug() << "!u" << tr("[%1] Sending [COMPLETE] to Atari").arg(deviceName());
            }
            else {
                qWarning() << "!w" << tr("[%1] Sending [$%2] to Atari").arg(deviceName()).arg((unsigned short) status, 2, 16, QChar('0'));
            }
        }
        else {
            if (status == 0x43) {
                qDebug() << "!u" << tr("[%1] Sending [COMPLETE] to Atari").arg(deviceName());
            }
            else {
                qWarning() << "!w" << tr("[%1] Sending [$%2] to Atari").arg(deviceName()).arg((unsigned short) status, 2, 16, QChar('0'));
            }
            unsigned char data[520];
            data[0] = 0xAA;
            int dataSize = m_atariDrive->CopySioDataFrame(data);
            if (dataSize > 0) {
                if (m_dumpDataFrame) {
                    int speed = m_atariDrive->GetTransmitSpeedInBitsPerSeconds();
                    qDebug() << "!u" << tr("[%1] Sending %2 bytes to Atari at speed %3 bit/s").arg(deviceName()).arg(dataSize - 1).arg(speed);
                    dumpBuffer((unsigned char *) data, dataSize - 1);
                }
            }
        }
    }
    else {
        qWarning() << "!w" << tr("[%1] Sending [$%2] to Atari").arg(deviceName()).arg((unsigned short) status, 2, 16, QChar('0'));
    }
    setTrace(false);
}
#else
void FirmwareDiskImage::handleCommand(quint8 command, quint16 aux)
{
    setDiskImager(this);
    setTrace(false);
    // rebuild the command with drive D1: so the firmware is always D1: even if disk slot is swapped by the user.
    // Add the checksum to feed the firmware with the full command frame
    unsigned char cmd[5];
    cmd[0] = (unsigned char) 0x31;
    cmd[1] = (unsigned char) (command & 0xFF);
    cmd[2] = (unsigned char) (aux & 0xFF);
    cmd[3] = (unsigned char) ((aux >> 8) & 0xFF);
    cmd[4] = computeChksum(cmd, 4);
	QString commandDesc = commandDescription(command, aux);
	qDebug() << "!n" << commandDesc;
    // before executing this new command, we have to make the drive run so the disk rotation is (approximately)
    // on the right sector, corresponding to the time elapsed since the previous command
    qint64 newTimeInNanoSeconds = m_timer.nsecsElapsed();
    qint64 diffTimeInMicroSeconds = ((m_lastTime == 0) || (m_lastTime > newTimeInNanoSeconds)) ? 0 : ((newTimeInNanoSeconds - m_lastTime) / 1000);
    m_atariDrive->RunForCycles((long)diffTimeInMicroSeconds);
    qint64 currentDistanceInMicroSeconds = (m_timer.nsecsElapsed() / 1000) % 208333L;
    m_atariDrive->RunUntilPosition((long)currentDistanceInMicroSeconds);
    // let the firmware receive this command
    unsigned char status = m_atariDrive->ReceiveSioCommandFrame(cmd);
    // if the command was not handled by the firmware (status is 0), it means that the firmware could not return in its main loop.
    // We must run the firmware a little bit more. But abort after 10 retries if the firmware is really stuck.
    int retries = 10;
    while ((status == 0) && (retries > 0)) {
        // run again to let the firmware come back in its main loop because the previous command
        // has been stopped immediately after the last byte sent by the firmware.
        for (int i = 0; i < 1000; i++) {
            m_atariDrive->Step();
        }
        m_atariDrive->RunUntilPosition((long)currentDistanceInMicroSeconds);
        status = m_atariDrive->ReceiveSioCommandFrame(cmd);
        retries--;
    }
    if (retries != 10) {
        if (retries <= 0) {
            // prevent being stuck in recursive handleCommand if firmware is still stuck after PowerOn
            if (m_initializing) {
                qCritical() << "!e" << tr("[%1] Firmware does not respond even after reboot ! Ignoring command")
                              .arg(deviceName());
            }
            else {
                qWarning() << "!w" << tr("[%1] Firmware does not respond. Rebooting drive now !")
                              .arg(deviceName());
                m_initializing = true;
                // restart the firmware and open Chip if option is on
                m_atariDrive->PowerOn();
                if (m_chipOpen) {
                    sendChipCode();
                }
                // retry the command with m_initializing still set to true to avoid stack overflow if firmware is definitely stuck
                handleCommand(command, aux);
            }
            m_initializing = false;
            return;
        }
        else {
            qWarning() << "!w" << tr("[%1] Command accepted after %2 retries")
                        .arg(deviceName())
                        .arg(10 - retries);
        }
    }
    // check the status
    if (status == STATUS_ACK) {
        if (!writeCommandAck()) {
            return;
        }
        // set high speed for data with Happy 810
        // FIXME: IT DOES NOT WORK ! Can not receive sectors from Happy software Rev.5. Please help !
        if ((m_atariDrive->GetHardware() == HARDWARE_810_WITH_HAPPY) && (command == 0x54)) {
            QThread::usleep(100);
            sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
        }
        // execute the command
        bool displayCpu = (m_displayCpuInstructions) && (m_atariDrive->GetRom()->IsCommandActive(command, aux));
        displayCpu = setTrace(displayCpu);
        if (displayCpu) {
            dumpCommandToFile(commandDesc);
        }
        status = m_atariDrive->ExecuteSioCommandFrame();
        if (status == WAIT_FOR_DATA) {
            // the firmware is expecting more data to come
            QByteArray data = readDataFrame(getExpectedDataFrameSize(command, aux));
            if (data.size() == 0) {
                writeDataNak();
                return;
            }
            // the DATA ACK must be sent very quickly.
            // If we loose time in the emulation to check if the firmware accepts the data, the computer may have already aborted the operation.
            // So we assume the data will be accepted by the firmware and send a DATA ACK right now.
            if (!writeDataAck()) {
                return;
            }
			if (displayCpu) {
				dumpDataToFile(data);
			}
            // add the checksum to feed the firmware with the full data frame
            data.append(computeChksum((unsigned char *) data.data(), data.size()));
            status = m_atariDrive->ReceiveSioDataFrame((unsigned char *) data.data(), data.size());
            // if the command is an "upload code" (archiver or happy), disassemble the code if the option is active
            if (m_disassembleUploadedCode) {
                int address = getUploadCodeStartAddress(command, aux, data);
                if (address != -1) {
                    disassembleBuffer((unsigned char *) data.data(), data.size() - 1, (unsigned short) address);
                }
            }
            if (status == STATUS_COMPLETE) {
                writeComplete();
            }
            else if (status == STATUS_ERROR) {
                writeError();
            }
            else if (status == STATUS_NAK) {
                // the firmware emulation rejected the data but we already sent back a DATA ACK !
                // We must send an ERROR status (instead of the NAK) so the computer knows there is a problem in the drive.
                // This is not an accurate emulation but it should occur very rarely.
                writeErrorInsteadOfNak();
            }
            else if (status != NO_STATUS) {
                // any unknown status...
                writeStatus(status);
            }
        }
        else if ((status == STATUS_COMPLETE) || (status == STATUS_ERROR)) {
            if (command == 0x52) {
                displayReadSectorStatus(m_lastReadSectorTrack, m_atariDrive->GetTrack(), m_lastReadSectorNumber, m_lastReadSectorStatus, m_lastReadSectorRotation, m_atariDrive->GetTrack()->GetReadDataBitsPerRotation());
            }
            // check the speed used by the firmware
            int speed = m_atariDrive->GetTransmitSpeedInBitsPerSeconds();
            if (speed > 20000) {
                sio->port()->setSpeed(findNearestSpeed(speed) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
            }
            // If the CHIP 810 or Super Archiver 1050 is open, accurate timing is disabled to get the maximum speed in Archiver software
            bool isArchiver = false;
            if ((m_atariDrive->GetHardware() == HARDWARE_810_WITH_THE_CHIP) || (m_atariDrive->GetHardware() == HARDWARE_1050_WITH_THE_ARCHIVER)) {
                // Chip can be open by configuration or by SIO command. But it can not be closed by configuration.
                // So we monitor SIO commands to reflect Chip status (SIO Close command exists only in CHIP 810)
                if ((m_atariDrive->GetHardware() == HARDWARE_810_WITH_THE_CHIP) && (command == 0x3F) && (aux == 0x0000) && (status == STATUS_COMPLETE)) {
                    m_chipOpen = false;
                    emit statusChanged(m_deviceNo);
                }
                else if ((command == 0x3F) && (aux != 0x0000) && (status == STATUS_COMPLETE)) {
                    m_chipOpen = true;
                    emit statusChanged(m_deviceNo);
                }
                isArchiver = m_chipOpen;
            }
            bool isHappy = (m_atariDrive->GetHardware() == HARDWARE_810_WITH_HAPPY) || (m_atariDrive->GetHardware() == HARDWARE_1050_WITH_HAPPY);
            if ((! isArchiver) && (! isHappy)) {
                // compute how much time the firmware has spent in command execution.
                unsigned long executionTimeInMicroSeconds = m_atariDrive->GetExecutionTimeInMicroseconds();
                unsigned long diffWorkInMicroSeconds = (unsigned long)((m_timer.nsecsElapsed() - newTimeInNanoSeconds) / 1000L);
                // wait to simulate accurate timing if the protected software on the Atari relies on the timing.
                if ((unsigned long)executionTimeInMicroSeconds > diffWorkInMicroSeconds) {
                    unsigned long delayInMicroSeconds = executionTimeInMicroSeconds - diffWorkInMicroSeconds;
                    // This added value is empirical. A lower value prevents A.E. from working and a higher value prevents Archon (US) from working.
                    if (command == 0x52) {
                        if ((m_atariDrive->GetHardware() == HARDWARE_810) || (m_atariDrive->GetHardware() == HARDWARE_810_WITH_THE_CHIP) || (m_atariDrive->GetHardware() == HARDWARE_1050_WITH_THE_ARCHIVER)) {
                            delayInMicroSeconds += 3000;
                        }
                        else {
                            delayInMicroSeconds += 6000;
                        }
                    }
                    QThread::usleep(delayInMicroSeconds);
                }
            }
            // the command has been executed. Send execution status
            if (status == STATUS_COMPLETE) {
                if (!writeComplete()) {
                    return;
                }
            }
            else {
                if (!writeError()) {
                    return;
                }
            }
            // Archiver needs a little delay between COMPLETE and DATA
            if (isArchiver) {
                QThread::usleep(150);
            }
            // check if there is data to send back to computer
            unsigned char data[520];
            int dataSize = m_atariDrive->CopySioDataFrame(data);
            if (dataSize > 0) {
                // Super Archiver 1050 software polls the speed and the firmware and answers $0A corresponding to 52400.
                // Replace 52400 with the current high speed set by the user in its preferences; Super Archiver software supports several speeds.
                if ((m_atariDrive->GetHardware() == HARDWARE_1050_WITH_THE_ARCHIVER) && (command == 0x3F) && (status == STATUS_COMPLETE) && (dataSize == 2) && (data[0] == 0x0A)) {
                    data[0] = data[1] = sio->port()->speedByte();
                }
                // same thing with Speedy
                if ((m_atariDrive->GetHardware() == HARDWARE_1050_WITH_SPEEDY) && (command == 0x3F) && (status == STATUS_COMPLETE) && (dataSize == 2)) {
                    data[0] = data[1] = sio->port()->speedByte();
                }
                // note that the firmware has already computed and added the checksum
                writeRawFrame(QByteArray((const char *) data, dataSize));
            }
            // if speed has been changed for data only (Happy 810/1050), normal speed is not restored here because it is too soon.
            // Instead, the readCommandFrame() method restores the normal speed when the COMMAND LOW event is detected
        }
        else if (status != NO_STATUS) {
            // any unknown status...
            writeStatus(status);
        }
    }
    else if (status == STATUS_NAK) {
        writeCommandNak();
    }
    else if (status != NO_STATUS) {
        // any unknown status...
        writeStatus(status);
    }
    m_lastTime = m_timer.nsecsElapsed();
    setTrace(false);
}
#endif

// return false if single density. true otherwise (considered as double density by WD2793)
bool FirmwareDiskImage::ReadTrack(int trackNumber, Track *track)
{
    bool onHalfTrack = (track->GetTrackType() == TRACK_HALF) && (trackNumber & 1);
    if (track->GetTrackType() == TRACK_HALF) {
        trackNumber >>= 1;
    }
    if (onHalfTrack) {
        track->Clear();
        if (m_displayDriveHead) {
            qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Head is between track $%4 and $%5")
                        .arg(deviceName())
                        .arg(track->GetRotationDataBit())
                        .arg(track->GetReadDataBitsPerRotation())
                        .arg(trackNumber, 2, 16, QChar('0'))
                        .arg(trackNumber + 1, 2, 16, QChar('0'));
        }
        track->ResetDirty();
        return ! m_geometry.isStandardSD();
    }
    if (m_displayDriveHead) {
        qDebug() << "!u" << tr("[%1] [Rotation=%2/%3] Head is on track $%4")
                    .arg(deviceName())
                    .arg(track->GetRotationDataBit())
                    .arg(track->GetReadDataBitsPerRotation())
                    .arg(trackNumber, 2, 16, QChar('0'));
    }
    track->Clear();
    if (m_originalImageType == FileTypes::Pro || m_originalImageType == FileTypes::ProGz) {
        FillTrackFromProImage(trackNumber, track);
    }
    else if (m_originalImageType == FileTypes::Atx || m_originalImageType == FileTypes::AtxGz) {
        FillTrackFromAtxImage(trackNumber, track);
    }
    else if (m_originalImageType == FileTypes::Scp || m_originalImageType == FileTypes::ScpGz) {
        FillTrackFromScpImage(trackNumber, track);
    }
    else {
        if (! isOpen()) {
            track->ResetDirty();
            return false;
        }
        FillTrackFromAtrImage(trackNumber, track);
    }
    track->ResetDirty();
    return ! m_geometry.isStandardSD();
}

void FirmwareDiskImage::FillTrackFromAtrImage(int trackNumber, Track *track)
{
    Crc16 crc;
    int bitNumber = 0;
    track->SetMaxDataBitsPerRotation();
    int sectorsPerTrack = m_geometry.sectorsPerTrack();
    int firstSectorNumber = sectorsPerTrack * trackNumber;
    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 64);
    bitNumber = track->WriteRawByte(bitNumber, DISK_INDEX_ADDR_MARK_CLOCK, DISK_INDEX_ADDR_MARK);
    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 11);
    for (int index = 0; index < sectorsPerTrack; index++) {
        QByteArray data;
        // Assume standard interleave: 1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18
        int sectorIndex = (((index % m_geometry.sectorsPerTrack()) + 1) << 1);
        if (index >= (m_geometry.sectorsPerTrack() >> 1)) {
            sectorIndex -= m_geometry.sectorsPerTrack() - 1;
        }
        int sector = sectorIndex - 1;
        int absoluteSector = firstSectorNumber + sector;
        int bytesPerSector = m_geometry.bytesPerSector(absoluteSector);
        readSector(absoluteSector, data);
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 6);
        crc.Reset();
        bitNumber = track->WriteRawByte(bitNumber, DISK_ID_ADDR_MARK_CLOCK, crc.Add(DISK_ID_ADDR_MARK));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)trackNumber));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add(0x00));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)(sector)));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add(0x00));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)((crc.GetCrc() >> 8) & 0xFF));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)(crc.GetCrc() & 0xFF));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 17);
        crc.Reset();
        bitNumber = track->WriteRawByte(bitNumber, DISK_DATA_ADDR_MARK_CLOCK, crc.Add(DISK_DATA_ADDR_MARK4));
        for (int i = 0; i < bytesPerSector; i++) {
            bitNumber = track->WriteRawByte(bitNumber, DISK_DATA_ADDR_MARK_CLOCK, crc.Add(0xFF - data[i]));
        }
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)((crc.GetCrc() >> 8) & 0xFF));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)(crc.GetCrc() & 0xFF));
        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 12);
    }
    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 64);
    track->SetReadDataBitsPerRotation(bitNumber);
}

void FirmwareDiskImage::FillTrackFromProImage(int trackNumber, Track *track)
{
    Crc16 crc;
    int bitNumber = 0;
    bool noResize = false;
    // compute the sector order.
    QByteArray dummy(128, 0);
    SimpleDiskImage::readProTrack(trackNumber, dummy, 128);

    // check if the track contains a protection (other than timing).
    // If there is a protection and displayTrackInformation option is on, then try to explain the layout of the track.
    bool displayInformation = false;
    if ((! m_initializing) && (m_displayTrackInformation) && (m_trackInformationDisplayed[trackNumber] == false)) {
        if (m_sectorsInTrack != m_geometry.sectorsPerTrack()) {
            displayInformation = true;
        }
        else {
            bool sectorNumbers[m_sectorsInTrack];
            for (int i = 0; i < m_sectorsInTrack; i++) {
                sectorNumbers[i] = false;
            }
            for (int i = 0; i < m_sectorsInTrack; i++) {
                quint16 indexInPro = m_trackContent[i];
                if (m_proSectorInfo[indexInPro].wd1771Status != 0xFF) {
                    displayInformation = true;
                    break;
                }
                else {
                    quint8 sectorNumber = m_proSectorInfo[indexInPro].sectorNumber;
                    if ((sectorNumber < 1) || (sectorNumber > m_geometry.sectorsPerTrack())) {
                        displayInformation = true;
                        break;
                    }
                    else {
                        if (sectorNumbers[sectorNumber - 1] == true) {
                            displayInformation = true;
                            break;
                        }
                        else {
                            sectorNumbers[sectorNumber - 1] = true;
                        }
                    }
                }
            }
        }
        if (displayInformation) {
            m_trackInformationDisplayed[trackNumber] = true;
            qDebug() << "!n" << tr("[%1] Track $%2 information:")
                        .arg(deviceName())
                        .arg(trackNumber, 2, 16, QChar('0'));
        }
    }

    // extend the track to the maximum size
    track->SetMaxDataBitsPerRotation();
    // reset the counter which is used to know if we wrote more data than the track capacity (wrap)
    track->ResetWrittenDataBitCounter();
    eWRITESTATE state = STATE_WRITE_SECTOR_HEADER;
    int sector = 0;
    quint16 indexInPro = 0;
    QString strTrackInfo("");
    while (sector < m_sectorsInTrack) {
        switch (state) {

        case STATE_WRITE_SECTOR_HEADER:
            {
                indexInPro = m_trackContent[sector];
                if (displayInformation) {
                    if (strTrackInfo.size() > 0) {
                        qDebug() << "!u" << strTrackInfo;
                        strTrackInfo.clear();
                    }
                    qint16 weakBits = m_proSectorInfo[indexInPro].weakBits;
                    quint8 shortSize = m_proSectorInfo[indexInPro].shortSectorSize;
                    strTrackInfo.reserve(80);
                    QString wd1771Status = convertWd1771Status(m_proSectorInfo[indexInPro].wd1771Status, weakBits, shortSize, 0);
                    if (indexInPro >= 1040) {
                        wd1771Status += QString(" + ") + tr("Duplicate");
                    }
                    quint16 absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + m_proSectorInfo[indexInPro].sectorNumber;
                    strTrackInfo.append(tr("[%1] Pro index %2 Sector $%3 ($%4) Status $%5 (%6) ID position %7")
                            .arg(deviceName())
                            .arg(indexInPro)
                            .arg(m_proSectorInfo[indexInPro].sectorNumber, 2, 16, QChar('0'))
                            .arg(absoluteSector, 3, 16, QChar('0'))
                            .arg(m_proSectorInfo[indexInPro].wd1771Status, 2, 16, QChar('0'))
                            .arg(wd1771Status)
                            .arg(bitNumber));
                }
                crc.Reset();
                bitNumber = track->WriteRawByte(bitNumber, DISK_ID_ADDR_MARK_CLOCK, crc.Add(DISK_ID_ADDR_MARK));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)trackNumber));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add(0x00));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)m_proSectorInfo[indexInPro].sectorNumber));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((m_proSectorInfo[indexInPro].wd1771Status & 0x06) != 0 ? 0x00 : 0x01)); // Long sector
                if (m_proSectorInfo[indexInPro].wd1771Status != 0xE7) {
                    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)((crc.GetCrc() >> 8) & 0xFF));
                    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)(crc.GetCrc() & 0xFF));
                }
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 17);
                state = STATE_WRITE_DATA_MARK;
                break;
            }

        case STATE_WRITE_DATA_MARK:
            {
                indexInPro = m_trackContent[sector];
                if (displayInformation) {
                    strTrackInfo.append(tr(" Data position %6")
                            .arg(bitNumber));
                }
                crc.Reset();
                unsigned char mark = (m_proSectorInfo[indexInPro].wd1771Status & 0x20) ? DISK_DATA_ADDR_MARK4 : DISK_DATA_ADDR_MARK1;
                bitNumber = track->WriteRawByte(bitNumber, DISK_DATA_ADDR_MARK_CLOCK, crc.Add(mark));
                state = STATE_WRITE_SECTOR_DATA;
                break;
            }

        case STATE_WRITE_SECTOR_DATA:
            {
                // this algorithm is to write short sectors, respecting the content of both sectors (this one and the next one) and
                // writing ID address marks and DATA address marks.
                // It assumes that the next sector header is byte-aligned with the previous one !!!
                // shortSectorSize has been computed by readProTrack and is an index of the next header inside this sector data.
                // If the header is in the middle of this sector data, a real header with an ID address mark is written and,
				// if found, a DATA address mark is also written.
                int nextHeaderId = -1;
                int nextDataId = -1;
                // do not apply the overlap for short track (to make Spy vs Spy US version work)
                if ((m_proSectorInfo[indexInPro].shortSectorSize > 0) && (m_sectorsInTrack > m_geometry.sectorsPerTrack())) {
                    nextHeaderId = m_proSectorInfo[indexInPro].shortSectorSize;
                    // check if we find the DATA address mark after the ID address mark (need at least 6 $00 before DATA address mark)
                    int index = nextHeaderId + 6;
                    int nb00 = 0;
                    while (index < 128) {
                        if (m_proSectorInfo[indexInPro].sectorData[index] == (char)(0xFF - 0x00)) {
                            nb00++;
                        }
                        else if ((nb00 >= 6) && ((m_proSectorInfo[indexInPro].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK1)) ||
                                (m_proSectorInfo[indexInPro].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK2)) ||
                                (m_proSectorInfo[indexInPro].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK3)) ||
                                (m_proSectorInfo[indexInPro].sectorData[index] == (char)(0xFF - DISK_DATA_ADDR_MARK4)))) {
                            nextDataId = index;
                            break;
                        }
                        index++;
                    }
                }

                // copy sector data but write the right CLOCK at nextHeaderId and nextDataId indexes
                for (int i = 0; i < 128; i++) {
                    // we are going to write an ID Address Mark for the next sector.
                    if (nextHeaderId == i) {
                        // there is a special case to handle: if we are in the last sector data which contains a header for the first one (wrap).
                        // In this case, we should abort everything because all data has been written.
                        if (sector == m_sectorsInTrack - 1) {
                            sector++;
                            track->SetReadDataBitsPerRotation(bitNumber);
                            noResize = true;
                            break;
                        }
                        else {
                            if (displayInformation) {
                                if (strTrackInfo.size() > 0) {
                                    qDebug() << "!u" << strTrackInfo;
                                    strTrackInfo.clear();
                                }
                                quint16 nextIndexInPro = m_trackContent[(sector + 1) % m_sectorsInTrack];
                                qint16 weakBits = m_proSectorInfo[nextIndexInPro].weakBits;
                                quint8 shortSize = m_proSectorInfo[nextIndexInPro].shortSectorSize;
                                strTrackInfo.reserve(80);
                                QString wd1771Status = convertWd1771Status(m_proSectorInfo[nextIndexInPro].wd1771Status, weakBits, shortSize, 0);
                                if (nextIndexInPro >= 1040) {
                                    wd1771Status += QString(" + ") + tr("Duplicate");
                                }
                                quint16 absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + m_proSectorInfo[nextIndexInPro].sectorNumber;
                                strTrackInfo.append(tr("[%1] Pro index %2 Sector $%3 ($%4) Status $%5 (%6) ID position %7")
                                        .arg(deviceName())
                                        .arg(nextIndexInPro)
                                        .arg(m_proSectorInfo[nextIndexInPro].sectorNumber, 2, 16, QChar('0'))
                                        .arg(absoluteSector, 3, 16, QChar('0'))
                                        .arg(m_proSectorInfo[nextIndexInPro].wd1771Status, 2, 16, QChar('0'))
                                        .arg(wd1771Status)
                                        .arg(bitNumber));
                            }
                            bitNumber = track->WriteRawByte(bitNumber, DISK_ID_ADDR_MARK_CLOCK, crc.Add(0xFF - m_proSectorInfo[indexInPro].sectorData[i]));
                        }
                    }
                    // we have also a Data Address Mark in this sector content
                    else if (nextDataId == i) {
                        // start over with the next sector data
                        break;
                    }
                    // just normal data
                    else {
                        unsigned char clock = ((m_proSectorInfo[indexInPro].weakBits != 0xFFFF) && (i >= m_proSectorInfo[indexInPro].weakBits)) ? DISK_NO_DATA_CLOCK : DISK_NORMAL_DATA_CLOCK;
                        bitNumber = track->WriteRawByte(bitNumber, clock, crc.Add(0xFF - m_proSectorInfo[indexInPro].sectorData[i]));
                    }
                }

                // write CRC after sector data.
                // But if an ID address mark has been written, we must jump to the data section to prevent the same header from being written a second time
                if (nextHeaderId == -1) {
                    state = STATE_WRITE_DATA_CRC;
                }
                else {
                    // we are now working on the next sector
                    sector++;
                    if (nextDataId == -1) {
                        // write the CRC
                        bool crcError = ((m_proSectorInfo[indexInPro].wd1771Status & 0x08) == 0);
                        if (! crcError) {
                            unsigned char crc1 = (unsigned char)((crc.GetCrc() >> 8) & 0xFF);
                            unsigned char crc2 = (unsigned char)(crc.GetCrc() & 0xFF);
                            bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc1);
                            bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc2);
                        }
                        // write the 6 mandatory $00 bytes before the next DATA address mark
                        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 6);
                    }
                    state = STATE_WRITE_DATA_MARK;
                }
                break;
            }

        case STATE_WRITE_DATA_CRC:
            {
                bool crcError = ((m_proSectorInfo[indexInPro].wd1771Status & 0x08) == 0) || (m_proSectorInfo[indexInPro].shortSectorSize != 0);
                unsigned char crc1 = (unsigned char)((crc.GetCrc() >> 8) & 0xFF);
                unsigned char crc2 = (unsigned char)(crc.GetCrc() & 0xFF);
                if (crcError) {
                    crc1 += 0x88;
                    crc2 += 0x55;
                }
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc1);
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc2);
                // compute gap to place all sectors at an equal distance when there is many space left on the track.
                // This makes Ali Baba work and prevents from blowing a revolution (loading is at optimal speed).
                int gap = 0;
                if (m_sectorsInTrack <= m_geometry.sectorsPerTrack()) {
                    gap = ((m_geometry.sectorsPerTrack() - m_sectorsInTrack + 1) * (m_geometry.bytesPerSector() + 70)) / m_sectorsInTrack;
                }
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 12 + gap);
                sector++;
                state = STATE_WRITE_SECTOR_HEADER;
                break;
            }
        }
    }

    // do nothing if we had a wrap in a short sector. The track has already been shrinked.
    if (! noResize) {
        // resize the track. Check if the data exceeded the max track size
        int standardSizeInBits = track->GetNormalDataBitsPerRotation();
        int maxSizeInBits = track->GetMaxDataBitsPerRotation();
        int writtenDataBits = track->GetWrittenDataBitCounter();
        if (writtenDataBits < maxSizeInBits) {
            // no wrapping occurred
            if (bitNumber < standardSizeInBits) {
                // track is smaller than a typical 288rpm track so keep the standard size with a big gap at the end of the track
                track->SetReadDataBitsPerRotation(standardSizeInBits);
            }
            else {
                // track contains more bytes than a typical 288rpm track. Keep this specific size.
                track->SetReadDataBitsPerRotation(bitNumber);
            }
        }
    }
    if (displayInformation) {
        if (strTrackInfo.size() > 0) {
            qDebug() << "!u" << strTrackInfo;
        }
        double rpm = (double)7500000 / (double)track->GetReadDataBitsPerRotation();
        qDebug() << "!u" << tr("[%1] Track $%2 has a length of %3 bits (%4 bits were written). Write speed is %5 RPM")
                    .arg(deviceName())
                    .arg(trackNumber, 2, 16, QChar('0'))
                    .arg(track->GetReadDataBitsPerRotation())
                    .arg(track->GetWrittenDataBitCounter())
                    .arg(rpm, 0, 'f', 2);
    }
}

void FirmwareDiskImage::FillTrackFromAtxImage(int trackNumber, Track *track)
{
    // the algorithm for ATX looks like the one for PRO images but here we detect also short sectors containing the next header when
    // this header is NOT byte-aligned with the current data.
    // This feature is needed to rebuild an exact track for track 2 of Electronic Arts software
    Crc16 crc;
    int bitNumber = 0;
    eWRITESTATE state = STATE_WRITE_SECTOR_HEADER;
    int sector = 0;
    int shortSectorSize = 0;
    int firstIDAddressMark = 0;
    int firstDataAddressMark = 0;
    int lastIDAddressMark = 0;
    int lastDataAddressMark = 0;
    bool noResize = false;
    bool warningDone = false;
    AtxSectorInfo *sectorInfo = NULL;

    // check if the track contains a protection (other than timing).
    // If there is a protection and displayTrackInformation option is on, then try to explain the layout of the track.
    bool displayInformation = false;
    if ((! m_initializing) && (m_displayTrackInformation) && (m_trackInformationDisplayed[trackNumber] == false)) {
        if (m_atxTrackInfo[trackNumber].size() != m_geometry.sectorsPerTrack()) {
            displayInformation = true;
        }
        else {
            bool sectorNumbers[m_atxTrackInfo[trackNumber].size()];
            for (int i = 0; i < m_atxTrackInfo[trackNumber].size(); i++) {
                sectorNumbers[i] = false;
            }
            for (int i = 0; i < m_atxTrackInfo[trackNumber].size(); i++) {
                sectorInfo = m_atxTrackInfo[trackNumber].at(i);
                if (sectorInfo->wd1771Status() != 0xFF) {
                    displayInformation = true;
                    break;
                }
                else {
                    quint8 sectorNumber = sectorInfo->sectorNumber();
                    if ((sectorNumber < 1) || (sectorNumber > m_geometry.sectorsPerTrack())) {
                        displayInformation = true;
                        break;
                    }
                    else {
                        if (sectorNumbers[sectorNumber - 1] == true) {
                            displayInformation = true;
                            break;
                        }
                        else {
                            sectorNumbers[sectorNumber - 1] = true;
                        }
                    }
                }
            }
        }
        if (displayInformation) {
            m_trackInformationDisplayed[trackNumber] = true;
            qDebug() << "!n" << tr("[%1] Track $%2 information:")
                        .arg(deviceName())
                        .arg(trackNumber, 2, 16, QChar('0'));
        }
    }

    // extend the track to the maximum size
    track->SetMaxDataBitsPerRotation();
    // reset the counter which is used to know if we wrote more data than the track capacity (wrap)
    track->ResetWrittenDataBitCounter();
    while (sector < m_atxTrackInfo[trackNumber].size()) {
        switch (state) {

        case STATE_WRITE_SECTOR_HEADER:
            {
                sectorInfo = m_atxTrackInfo[trackNumber].at(sector);
                int nextBitNumber = (int)(quint32)sectorInfo->sectorPosition();
                if (nextBitNumber < bitNumber) {
                    if ((displayInformation) && (! warningDone)) {
                        warningDone = true;
                        qWarning() << "!w" << tr("[%1] Overlapping sectors on track $%2 sector $%3 at index %4: requested position %5 overwrites position %6 with %7 bytes")
                                      .arg(deviceName())
                                      .arg(trackNumber, 2, 16, QChar('0'))
                                      .arg(sectorInfo->sectorNumber(), 2, 16, QChar('0'))
                                      .arg(sector)
                                      .arg(nextBitNumber)
                                      .arg(bitNumber)
                                      .arg((bitNumber - nextBitNumber) / 8);
                    }
                }
                else {
                    // we are skipping some bytes so we need to adjust the counter
                    track->AdjustWrittenDataBitCounter(nextBitNumber - bitNumber);
                    bitNumber = nextBitNumber;
                }
                // keep the first ID Address Mark written so we can adjust the track if the last sector is a short sector and
                // includes the first sector ID header in its data.
                if (sector == 0) {
                    firstIDAddressMark = bitNumber;
                }
                lastIDAddressMark = bitNumber;
                crc.Reset();
                bitNumber = track->WriteRawByte(bitNumber, DISK_ID_ADDR_MARK_CLOCK, crc.Add(DISK_ID_ADDR_MARK));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)trackNumber));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add(0x00));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((unsigned char)sectorInfo->sectorNumber()));
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc.Add((sectorInfo->wd1771Status() & 0x06) != 0 ? 0x00 : 0x01)); // Long sector
                if (sectorInfo->wd1771Status() != 0xE7) {
                    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)((crc.GetCrc() >> 8) & 0xFF));
                    bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, (unsigned char)(crc.GetCrc() & 0xFF));
                }
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 17);
                state = STATE_WRITE_DATA_MARK;
                break;
            }

        case STATE_WRITE_DATA_MARK:
            {
                // keep the first Data Address Mark written so we can adjust the track if the last sector is a short sector and
                // includes the first sector ID header in its data.
                if (sector == 0) {
                    firstDataAddressMark = bitNumber;
                }
                lastDataAddressMark = bitNumber;
                sectorInfo = m_atxTrackInfo[trackNumber].at(sector);
                crc.Reset();
                unsigned char mark = (sectorInfo->wd1771Status() & 0x20) ? DISK_DATA_ADDR_MARK4 : DISK_DATA_ADDR_MARK1;
                bitNumber = track->WriteRawByte(bitNumber, DISK_DATA_ADDR_MARK_CLOCK, crc.Add(mark));
                state = STATE_WRITE_SECTOR_DATA;
                break;
            }

        case STATE_WRITE_SECTOR_DATA:
            {
                // this algorithm is to write short sectors, respecting the content of both sectors (this one and the next one) and
                // writing ID address marks and DATA address marks.
                // It supports headers not byte-aligned with the previous one.
                // The variable bitShift contains the number of bits right shifted from the previous sector
                // shortSectorSize is an index of the next header inside this sector data.
                int bitShift = 0;
                shortSectorSize = m_atxTrackInfo[trackNumber].shortSectorSize(trackNumber, sector, &bitShift);
                if (displayInformation) {
                    QString wd1771Status = convertWd1771Status(sectorInfo->wd1771Status(), sectorInfo->sectorWeakOffset(), shortSectorSize, bitShift);
                    int phantomIndex = m_atxTrackInfo[trackNumber].duplicateIndex(sectorInfo, sectorInfo->sectorNumber());
                    if (phantomIndex > 1) {
                        wd1771Status += QString(" + ") + tr("Duplicate");
                    }
                    quint16 absoluteSector = (trackNumber * m_geometry.sectorsPerTrack()) + sectorInfo->sectorNumber();
                    qDebug() << "!u" << tr("[%1] Atx index %2 Sector $%3 ($%4) Status $%5 (%6) ID position %7 Data position %8")
                            .arg(deviceName())
                            .arg(sectorInfo->sectorPosition())
                            .arg(sectorInfo->sectorNumber(), 2, 16, QChar('0'))
                            .arg(absoluteSector, 3, 16, QChar('0'))
                            .arg(sectorInfo->wd1771Status(), 2, 16, QChar('0'))
                            .arg(wd1771Status)
                            .arg(lastIDAddressMark)
                            .arg(lastDataAddressMark);
                }

                // if the header is in the middle of this sector data, a real header with an ID address mark is written and,
                // if found, a DATA address mark is also written.
                int nextHeaderId = -1;
                int nextDataId = -1;
                if (shortSectorSize > 0) {
                    // check if we find the DATA address mark after the ID address mark (need at least 6 $00 before DATA addess mark)
                    nextHeaderId = shortSectorSize;
                    nextDataId = sectorInfo->dataMarkOffset(shortSectorSize, bitShift);
                }

                // check if there is enough space for the last sector data.
                // Maybe the data should wrap to fill the gap before the start of the first sector
                int nbBytesRemaining = 2048; // dummy big number greater than the biggest sector size
                int standardSizeInBits = track->GetNormalDataBitsPerRotation();
                int writtenDataBits = track->GetWrittenDataBitCounter();
                int usualLastDataInBits = standardSizeInBits - (sectorInfo->size() << 3);
                if ((nextHeaderId == -1) && (sector == m_atxTrackInfo[trackNumber].size() - 1) && (writtenDataBits > usualLastDataInBits) && (firstIDAddressMark > 64)) {
                    int nbBitsRemaining = standardSizeInBits - bitNumber;
                    if (nbBitsRemaining < 0) {
                        nbBitsRemaining = 0;
                    }
                    int nbBitsMovedToStartofTrack = (sectorInfo->size() << 3) - nbBitsRemaining;
                    // keep some space for the CRC and a couple of $00 bytes
                    if (nbBitsMovedToStartofTrack > firstIDAddressMark - 64) {
                        nbBitsMovedToStartofTrack = firstIDAddressMark - 64;
                        nbBitsRemaining = (sectorInfo->size() << 3) - nbBitsMovedToStartofTrack;
                    }
                    nbBytesRemaining = nbBitsRemaining >> 3;
                }

                // copy sector data but write the right clock at nextHeaderId and nextDataId indexes
                for (int i = 0; i < sectorInfo->size(); i++) {
                    // we are going to write an ID Address Mark for the next sector.
                    if (nextHeaderId == i) {
                        if (bitShift > 0) {
                            // write the ID Address Mark with the bit shift if not aligned with the previous sector data
                            unsigned char clock = ((DISK_NORMAL_DATA_CLOCK << (8 - bitShift)) | (DISK_ID_ADDR_MARK_CLOCK >> bitShift)) & 0xFF;
                            bitNumber = track->WriteRawByte(bitNumber, clock, crc.Add(0xFF - sectorInfo->rawByteAt(i)));
                            i++;
                            nbBytesRemaining--;
                            clock = (DISK_ID_ADDR_MARK_CLOCK << (8 - bitShift)) | (DISK_NORMAL_DATA_CLOCK >> bitShift);
                            bitNumber = track->WriteRawByte(bitNumber, clock, crc.Add(0xFF - sectorInfo->rawByteAt(i)));
                        }
                        else {
                            bitNumber = track->WriteRawByte(bitNumber, DISK_ID_ADDR_MARK_CLOCK, crc.Add(0xFF - sectorInfo->rawByteAt(i)));
                        }
                        if (sector < m_atxTrackInfo[trackNumber].size() - 1) {
                            lastIDAddressMark = bitNumber;
                        }
                    }
                    // we have also a Data Address Mark in this sector content
                    else if (nextDataId == i) {
                        if (bitShift > 0) {
                            // write the remaining bits (not aligned with the previous sector) before writing the Data Address Mark
                            for (int shift = 1; shift <= bitShift; shift++) {
                                track->WriteClock(bitNumber, true);
                                track->WriteData(bitNumber++, (((0xFF - sectorInfo->rawByteAt(i)) >> (8 - shift)) & 0x01) != 0);
                            }
                        }
                        // start over with the next sector data
                        break;
                    }
                    // just normal data
                    else {
                        unsigned char clock = ((sectorInfo->sectorWeakOffset() != 0xFFFF) && (i >= sectorInfo->sectorWeakOffset())) ? DISK_NO_DATA_CLOCK : DISK_NORMAL_DATA_CLOCK;
                        bitNumber = track->WriteRawByte(bitNumber, clock, crc.Add(0xFF - sectorInfo->rawByteAt(i)));
                    }
                    // we should wrap to the beginning of the track if there is enough gap space at the beginning
                    if (--nbBytesRemaining < 0) {
                        nbBytesRemaining = 2048; // prevent from doing the same operations twice
                        int nbBytesToWrite = sectorInfo->size() - i - 1;
                        if ((nbBytesToWrite << 3) < firstIDAddressMark) {
                            // we have enough space. Wrap now
                            track->SetReadDataBitsPerRotation(bitNumber);
                            bitNumber = 0;
                            noResize = true;
                        }
                    }
                }

                // write CRC after sector data.
                // But if an ID address mark has been written, we must jump to the data section to prevent the same header from being written a second time
                if (nextHeaderId == -1) {
                    state = STATE_WRITE_DATA_CRC;
                }
                else {
                    // we are now working on the next sector
                    sector++;
                    if (nextDataId == -1) {
                        // write the CRC
                        bool crcError = ((sectorInfo->wd1771Status() & 0x08) == 0);
                        if (! crcError) {
                            unsigned char crc1 = (unsigned char)((crc.GetCrc() >> 8) & 0xFF);
                            unsigned char crc2 = (unsigned char)(crc.GetCrc() & 0xFF);
                            bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc1);
                            bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc2);
                        }
                        // write the 6 mandatory $00 bytes before the next DATA address mark
                        bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, 0x00, 6);

                        // check if this was the last sector of the track. We know that we have written an ID Address Mark
                        // in the sector data but this header has already been written at the start of the track.
                        // The end of the track should be moved to the space left at the beginning of the track, overwriting the first ID Address Mark
                        if (sector == m_atxTrackInfo[trackNumber].size()) {
                            // compute the maximum number of bytes that can be moved to start of track.
                            // The space left at the start of the track before the first Data Address Mark is in firstDataAdressMark.
                            // We consider the space before the first Data Address Mark to be free because we want to overwrite the first ID Address Mark.
                            int toOffset;
                            int maxBytesToMove;
                            int bitsSinceLastIDAddressMark;
                            // how much bits do we want to move from the end of the track ?
                            if (bitNumber < lastIDAddressMark) {
                                bitsSinceLastIDAddressMark = track->GetReadDataBitsPerRotation() + bitNumber - lastIDAddressMark;
                            }
                            else {
                                bitsSinceLastIDAddressMark = bitNumber - lastIDAddressMark;
                            }
                            // is there enough space at the beginning of the track ?
                            if (firstDataAddressMark <= bitsSinceLastIDAddressMark) {
                                // we can fill all the start of the track with the data at the end of the track
                                toOffset = 0;
                                maxBytesToMove = firstDataAddressMark;
                            }
                            else {
                                // there is not enough data to move to fill all the start of the track
                                toOffset = firstDataAddressMark - bitsSinceLastIDAddressMark;
                                maxBytesToMove = bitsSinceLastIDAddressMark;
                            }
                            // compute the offset of the first bit to move then move data and resize the track
                            if (bitNumber < lastIDAddressMark) {
                                bitNumber = track->GetReadDataBitsPerRotation() + bitNumber - maxBytesToMove;
                            }
                            else {
                                bitNumber -= maxBytesToMove;
                            }
                            track->MoveBits(toOffset, bitNumber, maxBytesToMove);
                            track->SetReadDataBitsPerRotation(bitNumber);
                            if (displayInformation) {
                                int firstBitNumber = track->FindNextIdAddressMark(0);
                                qDebug() << "!u" << tr("[%1] Last sector is linked with the first one. First ID position is now set to %2")
                                            .arg(deviceName())
                                            .arg(firstBitNumber);
                            }
                            // if there is still a gap at the start of the track, remove it
                            if (toOffset > 0) {
                                bitNumber -= toOffset;
                                track->CropBits(0, toOffset);
                                if (displayInformation) {
                                    qDebug() << "!u" << tr("[%1] Start of track shrinked to remove %2 unused bits")
                                                .arg(deviceName())
                                                .arg(toOffset);
                                }
                            }
                            noResize = true;
                        }
                    }
                    state = STATE_WRITE_DATA_MARK;
                }
                break;
            }

        case STATE_WRITE_DATA_CRC:
            {
                bool crcError = ((sectorInfo->wd1771Status() & 0x08) == 0) || (shortSectorSize != 0);
                unsigned char crc1 = (unsigned char)((crc.GetCrc() >> 8) & 0xFF);
                unsigned char crc2 = (unsigned char)(crc.GetCrc() & 0xFF);
                if (crcError) {
                    crc1 += 0x88;
                    crc2 += 0x55;
                }
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc1);
                bitNumber = track->WriteRawByte(bitNumber, DISK_NORMAL_DATA_CLOCK, crc2);
                sector++;
                state = STATE_WRITE_SECTOR_HEADER;
                break;
            }
        }
    }

    // do nothing if we had a wrap in a short sector. The track has already been shrinked.
    if (! noResize) {
        // resize the track. Check if the data exceeded the max track size
        int standardSizeInBits = track->GetNormalDataBitsPerRotation();
        int maxSizeInBits = track->GetMaxDataBitsPerRotation();
        int writtenDataBits = track->GetWrittenDataBitCounter();
        if (writtenDataBits < maxSizeInBits) {
            // no wrapping occurred
            if (bitNumber < standardSizeInBits) {
                // track is smaller than a typical 288rpm track so keep the standard size with a big gap at the end of the track
                track->SetReadDataBitsPerRotation(standardSizeInBits);
            }
            else {
                // track contains more bytes than a typical 288rpm track. Keep this specific size.
                track->SetReadDataBitsPerRotation(bitNumber);
            }
        }
    }
    if (displayInformation) {
        double rpm = (double)7500000 / (double)track->GetReadDataBitsPerRotation();
        qDebug() << "!u" << tr("[%1] Track $%2 has a length of %3 bits (%4 bits were written). Write speed is %5 RPM")
                    .arg(deviceName())
                    .arg(trackNumber, 2, 16, QChar('0'))
                    .arg(track->GetReadDataBitsPerRotation())
                    .arg(track->GetWrittenDataBitCounter())
                    .arg(rpm, 0, 'f', 2);
    }
}

void FirmwareDiskImage::FillTrackFromScpImage(int trackNumber, Track *track)
{
}

void FirmwareDiskImage::WriteTrack(int trackNumber, Track *track)
{
    bool onHalfTrack = (track->GetTrackType() == TRACK_HALF) && (trackNumber & 1);
    if (track->GetTrackType() == TRACK_HALF) {
        trackNumber >>= 1;
    }
    if (onHalfTrack) {
        return;
    }
    resetTrack(trackNumber);
    Crc16 crc;
    unsigned char data[1024 + 2];
    int bitNumber = track->FindNextIdAddressMark(0);
    while (bitNumber != -1) {
        crc.Reset();
        unsigned char clock;
        unsigned char header[7];
        unsigned char weak;
        int headerBitNumber = bitNumber;
        for (unsigned int i = 0; i < sizeof(header); i++) {
            headerBitNumber = track->ReadRawByte(headerBitNumber, &clock, &header[i], &weak);
            if (i < sizeof(header) - 2) {
                crc.Add(header[i]);
            }
        }
        unsigned short headerCrc = (((unsigned short)header[5]) << 8) | (((unsigned short)header[6]) & 0xFF);
        if ((header[1] == (unsigned char)trackNumber) && (headerCrc == crc.GetCrc())) {
            crc.Reset();
            int maxBitIndex = headerBitNumber + (30 * 8); //track->AddBytesToIndex(headerBitNumber, 30);
            int dataBitNumber = track->FindNextDataAddressMark(headerBitNumber);
            if (dataBitNumber <= maxBitIndex) {
                int nbDataBytes = 128 << (header[4] & 0x03);
                unsigned char dataMark;
                unsigned char crcHigh;
                unsigned char crcLow;
                int weakOffset = 0xFFFF;
                dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &dataMark, &weak);
                crc.Add(dataMark);
                for (int i = 0; i < nbDataBytes; i++) {
                    unsigned char oneDataByte;
                    dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &oneDataByte, &weak);
                    if ((weak != 0) && (weakOffset == 0xFFFF)) {
                        weakOffset = i;
                    }
                    data[i] = 0xFF - crc.Add(oneDataByte);
                }
                dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &crcHigh, &weak);
                dataBitNumber = track->ReadRawByte(dataBitNumber, &clock, &crcLow, &weak);
                unsigned short dataCrc = (crcHigh << 8) | (crcLow & 0xFF);
                bool crcError = (dataCrc != crc.GetCrc());
                writeSectorExtended(bitNumber, dataMark, header[1], header[2], header[3], header[4], QByteArray((const char *)data, nbDataBytes), crcError, weakOffset);
            }
        }
        bitNumber = track->FindNextIdAddressMark(bitNumber + 8);
    }
}

void FirmwareDiskImage::NotifyDirty()
{
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
}
