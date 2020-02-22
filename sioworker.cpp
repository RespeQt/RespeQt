/*
 * sioworker.cpp
 *
 * Copyright 2015, 2017 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "sioworker.h"

#include "respeqtsettings.h"
#include <QFile>
#include <QDateTime>
#include <QtDebug>
#ifndef QT_NO_DEBUG
#include <QFileDialog>
#endif

/* SioDevice */
SioDevice::SioDevice(SioWorkerPtr worker)
        :QObject()
{
    sio = worker;
    m_deviceNo = -1;
}

SioDevice::~SioDevice()
{
    if (m_deviceNo != -1) {
        sio->uninstallDevice(m_deviceNo);
    }
}

QString SioDevice::deviceName()
{
    return sio->deviceName(m_deviceNo);
}

/* SioWorker */

SioWorker::SioWorker()
        : QThread()
#ifndef QT_NO_DEBUG
      , mSnapshotRunning(false), mSnapshotWriter(NULL)
#endif
{
    deviceMutex = new QMutex(QMutex::Recursive);
    for (int i=0; i <= 255; i++) {
        devices[i] = 0;
    }
    mPort = 0;  
}

SioWorker::~SioWorker()
{
    for (int i=0; i <= 255; i++) {
        if (devices[i]) {
            delete devices[i];
        }
    }
    delete deviceMutex;
}

bool SioWorker::wait(unsigned long time)
{
    mustTerminate = true;

    if (mPort) {
        mPort->cancel();
    }

    bool result = QThread::wait(time);

    if (mPort) {
        delete mPort;
        mPort = 0;
    }

    return result;
}

void SioWorker::start(Priority p)
{
    switch (respeqtSettings->backend()) {
        default:
        case SERIAL_BACKEND_STANDARD:
            mPort = new StandardSerialPortBackend(this);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(this);
            break;
#ifndef QT_NO_DEBUG
        case SERIAL_BACKEND_TEST:
            mPort = new TestSerialPortBackend(this);
            break;
#endif
    }

    QByteArray data;
    for (int i=0; i <= 255; i++)
    {
        if(devices[i])
        {
            data.append(i);
        }
    }
    mPort->setActiveSioDevices(data);

    mustTerminate = false;
    QThread::start(p);
}

void SioWorker::run()
{
    connect(mPort, SIGNAL(statusChanged(QString)), this, SIGNAL(statusChanged(QString)));

    /* Open serial port */
    if (!mPort->open()) {
        return;
    }

    /* Process SIO commands until we're explicitly stopped */
    while (!mustTerminate) {

//        qDebug() << "!d" << tr("DBG -- SIOWORKER...");

        QByteArray cmd = mPort->readCommandFrame();
        if (mustTerminate) {
            break;
        }
        if (cmd.isEmpty()) {
            qCritical() << "!e" << tr("Cannot read command frame.");
            break;
        }
        /* Decode the command */
        quint8 no = (quint8)cmd[0];
        quint8 command = (quint8)cmd[1];
        quint16 aux = ((quint8)cmd[2]) + ((quint8)cmd[3] * 256);

#ifndef QT_NO_DEBUG
        writeSnapshotCommandFrame(no, command, cmd[2], cmd[3]);
#endif

        /* Redirect the command to the appropriate device */
        deviceMutex->lock();
        if (devices[no]) {
            if (devices[no]->tryLock()) {
                devices[no]->handleCommand(command, aux);
                devices[no]->unlock();
            } else {
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 ignored because the image explorer is open.")
                               .arg(deviceName(no))
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
            }
        } else {
            if ((displayCommandName) && (no >= 0x31 && no <= 0x3F)) {
                qDebug() << "!u" << tr("[%1] command: $%2, aux: $%3 ignored: %4")
                            .arg(deviceName(no))
                            .arg(command, 2, 16, QChar('0'))
                            .arg(aux, 4, 16, QChar('0'))
                            .arg(guessDiskCommand(command, aux));
            }
            else {
                qDebug() << "!u" << tr("[%1] command: $%2, aux: $%3 ignored.")
                               .arg(deviceName(no))
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
            }
        }
        deviceMutex->unlock();
        cmd.clear();
    }
    mPort->close();
}

QString SioWorker::guessDiskCommand(quint8 command, quint16 aux)
{
#if ALL_COMMANDS
    // displays all known command meaning
    switch (command) {
    case 0x20:  return tr("[810 Rev. E] Upload and Execute Code or [Speedy 1050] Format Disk Asynchronously");
    case 0x21:  return tr("Format Single Density Disk");
    case 0x22:  return tr("Format Enhanced Density Disk");
    case 0x23:  return tr("[1050, Happy 1050] Run Speed Diagnostic or [Turbo 1050] Service Put");
    case 0x24:  return tr("[1050, Happy 1050] Run Diagnostic or [Turbo 1050] Service Get");
    case 0x36:  return tr("[Happy 1050 Rev. 7] Clear Sector Flags (Broadcast)");
    case 0x37:  return tr("[Happy 1050 Rev. 7] Get Sector Flags (Broadcast)");
    case 0x38:  return tr("[Happy 1050 Rev. 7] Send Sector (Broadcast)");
    case 0x3F:  return tr("[Super Archiver 1050, Happy 1050, Speedy 1050, Duplicator 1050] Poll Speed");
    case 0x41:  return tr("[Happy 810/1050 Rev. 7] Read Track or [Speedy 1050] Add or Delete a Command");
    case 0x42:  return tr("[Chip 810, Super Archiver 1050] Write Sector using Index or [Happy 810/1050 Rev. 7] Read All Sectors");
    case 0x43:  return tr("[Chip 810, Super Archiver 1050] Read All Sector Statuses or [Happy 810/1050 Rev. 7] Set Skew Alignment");
    case 0x44:  return tr("[Chip 810, Super Archiver 1050] Read Sector using Index or [Happy 810/1050 Rev. 7] Read Skew Alignment or [Speedy 1050] Configure Drive and Display");
    case 0x46:  return tr("[Chip 810, Super Archiver 1050] Write Track");
    case 0x47:  return tr("[Chip 810, Super Archiver 1050] Read Track (128 bytes)");
    case 0x48:
        if ((aux & 0xFF) == 0x01)
                return tr("[Happy 810/1050 Rev. 7] Set Idle Timeout");
        else if ((aux & 0xFF) == 0x02)
                return tr("[Happy 810/1050 Rev. 7] Set Alternate Device ID");
        else if ((aux & 0xFF) == 0x03)
                return tr("[Happy 810/1050 Rev. 7] Reinitialize Drive");
        else	return tr("[Happy 810/1050 Rev. 7] Configure Drive");
    case 0x49:	return tr("[Happy 810/1050 Rev. 7] Write Track with Skew Alignment");
    case 0x4A:	return tr("[Happy 810/1050 Rev. 7] Init Skew Alignment");
    case 0x4B:	return tr("[Happy 810/1050 Rev. 7] Prepare backup or [Speedy 1050] Configure Slow/Fast Speed");
    case 0x4C:	return tr("[Chip 810, Super Archiver 1050] Set RAM Buffer or [Speedy 1050] Jump to Address");
    case 0x4D:	return tr("[Chip 810] Upload and Execute Code or [Speedy 1050] Jump to Address with Acknowledge");
    case 0x4E:	return tr("[Super Archiver 1050, Speedy 1050, Turbo 1050, Duplicator 1050] Get PERCOM Block or [Chip 810] Set Shutdown Delay");
    case 0x4F:	return tr("[Speedy 1050, Turbo 1050, Duplicator 1050] Set PERCOM Block or [Chip 810, Super Archiver 1050] Open CHIP");
    case 0x50:	return tr("Put Sector (no verify)");
    case 0x51:	return tr("[Happy 810] Execute code or [Speedy 1050] Flush Write");
    case 0x52:	return tr("Read Sector");
    case 0x53:	return tr("Get Status");
    case 0x54:	return tr("[Chip 810, Super Archiver 1050, 810 Rev. E] Get RAM Buffer");
    case 0x55:	return tr("[Happy 810] Execute code");
    case 0x56:	return tr("[Happy 810] Execute code");
    case 0x57:	return tr("Write Sector (with verify)");
    case 0x5A:	return tr("[Chip 810, Super Archiver 1050] Set Trace On/Off");
    case 0x60:	return tr("[Speedy 1050] Write Track at Address");
    case 0x61:	return tr("[Duplicator 1050] Analyze Track");
    case 0x62:	return tr("[Super Archiver 1050] Write Fuzzy Sector using Index or [Speedy 1050] Read Track at Address or [Duplicator 1050] Set Drive buffer Mode");
    case 0x63:	return tr("[Duplicator 1050] Custom Track Format");
    case 0x64:	return tr("[Duplicator 1050] Set Density Sensing");
    case 0x66:	return tr("[Super Archiver 1050, Duplicator 1050] Format with Custom Sector Skewing");
    case 0x67:	return tr("[Super Archiver 1050] Read Track (256 bytes) or [Duplicator 1050] Seek to Track");
    case 0x68:	return tr("[Speedy 1050] Get SIO Routine size or [Duplicator 1050] Change Drive Hold on Time");
    case 0x69:	return tr("[Speedy 1050] Get SIO Routine relocated at Address");
    case 0x6A:	return tr("[Duplicator 1050] Change Drive RPM");
    case 0x6D:	return tr("[Duplicator 1050] Upload Sector Pattern and Read Track");
    case 0x6E:	return tr("[Duplicator 1050] Upload Sector Pattern and Write Buffer to Disk");
    case 0x70:	return tr("[Happy 810/1050 Rev. 7] High Speed Put Sector (no verify)");
    case 0x71:	return tr("[Super Archiver 1050] Write Fuzzy Sector");
    case 0x72:	return tr("[Happy 810/1050 Rev. 7] High Speed Read Sector or [Duplicator 1050] Run Uploaded Program at Address");
    case 0x73:	return tr("[Super Archiver 1050] Set Speed or [Duplicator 1050] Set Load Address");
    case 0x74:	return tr("[Super Archiver 1050] Read Memory or [Duplicator 1050] Get Sector Data");
    case 0x75:	return tr("[Super Archiver 1050] Upload and Execute Code or [Duplicator 1050] Upload Data to Drive");
    case 0x76:	return tr("[Duplicator 1050] Upload Sector Data in Buffer");
    case 0x77:	return tr("[Happy 810/1050 Rev. 7] High Speed Write Sector (with verify) or [Duplicator 1050] Write Sector with Deleted Data");
    case 0xFF:	return tr("[810 Rev. E] Execute Code");
    default:    return tr("Unknown");
    }
#else
    // displays only labels coresponding to common commands, Happy commands and Super Archiver commands
    switch (command) {
    case 0x21:  return tr("Format Single Density Disk");
    case 0x22:  return tr("Format Enhanced Density Disk");
    case 0x23:  return tr("Run Speed Diagnostic");
    case 0x24:  return tr("Run Diagnostic");
    case 0x36:  return tr("Happy Clear Sector Flags (Broadcast)");
    case 0x37:  return tr("Happy Get Sector Flags (Broadcast)");
    case 0x38:  return tr("Happy Send Sector (Broadcast)");
    case 0x3F:  return tr("Poll Speed");
    case 0x41:  return tr("Happy Read Track");
    case 0x42:  return tr("Super Archiver Write Sector using Index or Happy Read All Sectors");
    case 0x43:  return tr("Super Archiver Read All Sector Statuses or Happy Set Skew Alignment");
    case 0x44:  return tr("Super Archiver Read Sector using Index or Happy Read Skew Alignment");
    case 0x46:  return tr("Super Archiver Write Track or Happy Write Track");
    case 0x47:  return tr("Super Archiver Read Track (128 bytes) or Happy Write All Sectors");
    case 0x48:
        if ((aux & 0xFF) == 0x01)
                return tr("Happy Set Idle Timeout");
        else if ((aux & 0xFF) == 0x02)
                return tr("Happy Set Alternate Device ID");
        else if ((aux & 0xFF) == 0x03)
                return tr("Happy Reinitialize Drive");
        else	return tr("Happy Configure Drive");
    case 0x49:	return tr("Happy Write Track with Skew Alignment");
    case 0x4A:	return tr("Happy Init Skew Alignment");
    case 0x4B:	return tr("Happy Prepare backup");
    case 0x4C:	return tr("Super Archiver Set RAM Buffer");
    case 0x4D:	return tr("Chip 810 Upload and Execute Code");
    case 0x4E:	return tr("Get PERCOM Block");
    case 0x4F:	return tr("Set PERCOM Block or Super Archiver Open CHIP");
    case 0x50:	return tr("Put Sector (no verify)");
    case 0x51:	return tr("Happy Execute code");
    case 0x524:	return tr("Read Sector");
    case 0x53:	return tr("Get Status");
    case 0x54:	return tr("Super Archiver Get RAM Buffer");
    case 0x55:	return tr("Happy Execute code");
    case 0x56:	return tr("Happy Execute code");
    case 0x57:	return tr("Write Sector (with verify)");
    case 0x5A:	return tr("Super Archiver Set Trace On/Off");
    case 0x62:	return tr("Super Archiver Write Fuzzy Sector using Index");
    case 0x66:	return tr("Super Archiver Format with Custom Sector Skewing");
    case 0x67:	return tr("Super Archiver Read Track (256 bytes)");
    case 0x70:	return tr("Happy High Speed Put Sector (no verify)");
    case 0x71:	return tr("Super Archiver Write Fuzzy Sector");
    case 0x72:	return tr("Happy High Speed Read Sector");
    case 0x73:	return tr("Super Archiver Set Speed");
    case 0x74:	return tr("Super Archiver Read Memory");
    case 0x75:	return tr("Super Archiver Upload and Execute Code");
    case 0x77:	return tr("Happy High Speed Write Sector (with verify)");
    default:    return tr("Unknown");
    }
#endif
}

void SioWorker::installDevice(quint8 no, SioDevice *device)
{
    deviceMutex->lock();
    if (devices[no]) {
        delete devices[no];
    }
    devices[no] = device;
    device->setDeviceNo(no);
    deviceMutex->unlock();
    if(mPort)
    {
        QByteArray data;
        for (int i=0; i <= 255; i++)
        {
            if(devices[i])
            {
                data.append(i);
            }
        }
        mPort->setActiveSioDevices(data);
    }
}

void SioWorker::uninstallDevice(quint8 no)
{
    deviceMutex->lock();
    if (devices[no]) {
        devices[no]->setDeviceNo(-1);
    }
    devices[no] = 0;
    deviceMutex->unlock();
    if(mPort)
    {
        QByteArray data;
        for (int i=0; i <= 255; i++)
        {
            if(devices[i])
            {
                data.append(i);
            }
        }
        mPort->setActiveSioDevices(data);
    }
}

void SioWorker::swapDevices(quint8 d1, quint8 d2)
{
    SioDevice *t1, *t2;

    deviceMutex->lock();
    t1 = devices[d1];
    t2 = devices[d2];
    uninstallDevice(d1);
    uninstallDevice(d2);
    if (t2) {
        installDevice(d1, t2);
    }
    if (t1) {
        installDevice(d2, t1);
    }
    deviceMutex->unlock();
}

SioDevice* SioWorker::getDevice(quint8 no)
{
    SioDevice *result;
    deviceMutex->lock();
    result = devices[no];
    deviceMutex->unlock();
    return result;
}

QString SioWorker::deviceName(int device)
{
    QString result;
    switch (device) {
        case -1:
            // It must be because of the piggy-backed autoboot
            result = tr("Disk 1 (below autoboot)");
            break;
        case DISK_BASE_CDEVIC+0x0:
        case DISK_BASE_CDEVIC+0x1:
        case DISK_BASE_CDEVIC+0x2:
        case DISK_BASE_CDEVIC+0x3:
        case DISK_BASE_CDEVIC+0x4:
        case DISK_BASE_CDEVIC+0x5:
        case DISK_BASE_CDEVIC+0x6:
        case DISK_BASE_CDEVIC+0x7:
        case DISK_BASE_CDEVIC+0x8:
        case DISK_BASE_CDEVIC+0x9:
        case DISK_BASE_CDEVIC+0xA:
        case DISK_BASE_CDEVIC+0xB:
        case DISK_BASE_CDEVIC+0xC:
        case DISK_BASE_CDEVIC+0xD:
        case DISK_BASE_CDEVIC+0xE:
            result = tr("Disk %1").arg(device & 0x0F);
            break;
        case PRINTER_BASE_CDEVIC+0:
        case PRINTER_BASE_CDEVIC+1:
        case PRINTER_BASE_CDEVIC+2:
        case PRINTER_BASE_CDEVIC+3:
            result = tr("Printer %1").arg((device & 0x0F) + 1);
            break;
        case SMART_CDEVIC:
            result = tr("Smart device (APE time + URL)");
            break;
        case RESPEQT_CLIENT_CDEVIC:
            result = tr("RespeQt Client");
            break;
        case RS232_BASE_CDEVIC+0:
        case RS232_BASE_CDEVIC+1:
        case RS232_BASE_CDEVIC+2:
        case RS232_BASE_CDEVIC+3:
            result = tr("RS232 %1").arg((device & 0x0F) +1);
            break;
        case PCLINK_CDEVIC:
            result = tr("PCLINK");
            break;
        default:
            result = tr("Device $%1").arg(device, 2, 16, QChar('0'));
            break;
    }
    return result;
}

#ifndef QT_NO_DEBUG
void SioWorker::startSIOSnapshot()
{
    QString fileName = QFileDialog::getSaveFileName(MainWindow::getInstance(),
                 tr("Save test XML File"), QString(), tr("XML Files (*.xml)"));
    if (fileName.length() > 0)
    {
        QFile *file = new QFile(fileName);
        file->open(QFile::WriteOnly | QFile::Truncate);
        if (!mSnapshotWriter)
        {
            mSnapshotWriter = new QXmlStreamWriter(file);
        }
        mSnapshotWriter->setAutoFormatting(true);
        mSnapshotWriter->writeStartDocument();
        mSnapshotWriter->writeStartElement("testcase");
        mSnapshotRunning = true;
    }
}

void SioWorker::stopSIOSnapshot()
{
    if (mSnapshotWriter)
    {
        mSnapshotWriter->writeEndElement();
        mSnapshotWriter->writeEndDocument();
        mSnapshotWriter->device()->close();
        delete mSnapshotWriter;
        mSnapshotWriter = NULL;
        mSnapshotRunning = false;
    }
}

void SioWorker::writeSnapshotCommandFrame(qint8 no, qint8 command, qint8 aux1, qint8 aux2)
{
    // Record the command frame, if the snapshot is running
    if (mSnapshotRunning && mSnapshotWriter)
    {
        mSnapshotWriter->writeStartElement("commandframe");
        mSnapshotWriter->writeAttribute("device", QString::number(no));
        mSnapshotWriter->writeAttribute("command", QString::number(command));
        mSnapshotWriter->writeAttribute("aux1", QString::number(aux1));
        mSnapshotWriter->writeAttribute("aux2", QString::number(aux2));
        mSnapshotWriter->writeEndElement();
    }
}

void SioWorker::writeSnapshotDataFrame(QByteArray &data)
{
    // Record the command frame, if the snapshot is running
    if (mSnapshotRunning && mSnapshotWriter)
    {
        mSnapshotWriter->writeStartElement("dataframe");
        QString cs;
        foreach(char c, data)
        {
            unsigned char chr = static_cast<unsigned char>(c);
            cs.append("&#");
            cs.append(QString::number(chr, 10));
            cs.append(";");
        }
        mSnapshotWriter->writeCDATA(cs);
        mSnapshotWriter->writeEndElement();
    }
}

#endif

/* CassetteWorker */

CassetteWorker::CassetteWorker()
    : QThread()
{
    mPort = 0;
    mustTerminate.lock();
}

CassetteWorker::~CassetteWorker()
{
}

bool CassetteWorker::loadCasImage(const QString &fileName)
{
    mRecords.clear();

    QFile casFile(fileName);

    if (!casFile.open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    QByteArray header, data;
    uint magic;
    int length, aux;

    header = casFile.read(8);

    if (header.length() != 8) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    magic = (quint8)header.at(0) + (quint8)header.at(1) * 256 + (quint8)header.at(2) * 65536 + (quint8)header.at(3) * 16777216;
    length = (quint8)header.at(4) + (quint8)header.at(5) * 256;
    aux = (quint8)header.at(6) + (quint8)header.at(7) * 256;


    data = casFile.read(length);
    if (data.length() != length) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    /* Verify the header */
    if (magic != 0x494a5546) { // "FUJI"
        qCritical() << "!e" << tr("Cannot open '%1': The header does not match.").arg(fileName);
        return false;
    }

    if (!data.isEmpty()) {
        qDebug() << "!n" << tr("[Cassette]: File description '%2'.").arg(QString::fromLatin1(data));
    }

    int lastBaud = 600;
    mTotalDuration = 0;

    /* Read the cas file */
    do {
        header = casFile.read(8);

        if (header.length() != 8) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        magic = (quint8)header.at(0) + (quint8)header.at(1) * 256 + (quint8)header.at(2) * 65536 + (quint8)header.at(3) * 16777216;
        length = (quint8)header.at(4) + (quint8)header.at(5) * 256;
        aux = (quint8)header.at(6) + (quint8)header.at(7) * 256;

        data = casFile.read(length);
        if (data.length() != length) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        /* Verify the header */
        if (magic == 0x64756162) {          // "baud"
            if (respeqtSettings->useCustomCasBaud()) {
                lastBaud = respeqtSettings->customCasBaud();
            } else {
                lastBaud = aux;
            }
        } else if (magic == 0x61746164) {   // "data"
            CassetteRecord record;
            record.baudRate = lastBaud;
            record.data = data;
            record.gapDuration = aux;
            record.totalDuration = aux + (length * 10000 + (lastBaud/2))/lastBaud;
            mTotalDuration += record.totalDuration;
            mRecords.append(record);
        } else {
            qCritical() << "!e" << tr("Cannot open '%1': Unknown chunk header %2.").arg(fileName).arg(magic);
            return false;
        }

    } while (!casFile.atEnd());

    return true;
}

bool CassetteWorker::wait (unsigned long time)
{
    if (mPort) {
        mPort->cancel();
    }

    mustTerminate.unlock();

    bool result = QThread::wait(time);

    if (mPort) {
        mPort->close();
        delete mPort;
        mPort = 0;
    }

    return result;
}

void CassetteWorker::run()
{
    /* Open serial port */
    if (!mPort->open()) {
        return;
    }

    int lastBaud = 0;
    int block = 1;
    int remainingTime = mTotalDuration;

    QTime tm = QTime::currentTime();

    foreach (CassetteRecord record, mRecords) {
        if (lastBaud != record.baudRate) {
            lastBaud = record.baudRate;
            if (!mPort->setSpeed(lastBaud)) {
                return;
            }
        }
        emit statusChanged(remainingTime);
        qDebug() << "!n" << tr("[Cassette] Playing record %1 of %2 (%3 ms of gap + %4 bytes of data)")
                .arg(block)
                .arg(mRecords.count())
                .arg(record.gapDuration)
                .arg(record.data.length());
        tm = tm.addMSecs(record.gapDuration);
        int w = QTime::currentTime().msecsTo(tm);
        if (w < 0) {
            w = 0;
        }
        if (mustTerminate.tryLock(w)) {
            return;
        }
        tm = QTime::currentTime();
        tm = tm.addMSecs((record.data.length() * 10000 + (lastBaud/2))/lastBaud);
        for (int i=0; i < record.data.length(); i+=10) {
            mPort->writeRawFrame(record.data.mid(i, 10));
            if (mustTerminate.tryLock()) {
                return;
            }
        }
        block++;
        remainingTime -= record.totalDuration;
    }
    // Wait until last written bytes are transferred and then some (FTDI bug)
    int w = QTime::currentTime().msecsTo(tm);
    if (w < 0) {
        w = 0;
    }
    msleep(w + 500);
    mPort->close();
}

void CassetteWorker::start(Priority p)
{
    switch (respeqtSettings->backend()) {
        case SERIAL_BACKEND_STANDARD:
            mPort = new StandardSerialPortBackend(this);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(this);
            break;
#ifndef QT_NO_DEBUG
        case SERIAL_BACKEND_TEST:
            mPort = new TestSerialPortBackend(this);
            break;
#endif
    }
    QThread::start(p);
}
