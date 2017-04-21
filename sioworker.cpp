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

/* SioDevice */
SioDevice::SioDevice(SioWorker *worker)
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
#ifdef QT_NO_DEBUG
        case SERIAL_BACKEND_TEST:
#endif
        case SERIAL_BACKEND_STANDARD:
            mPort = new StandardSerialPortBackend(0);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(0);
            break;
#ifndef QT_NO_DEBUG
        case SERIAL_BACKEND_TEST:
            mPort = new TestSerialPortBackend(0);
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
            qDebug() << "!u" << tr("[%1] command: $%2, aux: $%3 ignored.")
                           .arg(deviceName(no))
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
        }
        deviceMutex->unlock();
        cmd.clear();
    }
    mPort->close();
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
            mPort = new StandardSerialPortBackend(0);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(0);
            break;
    }
    QThread::start(p);
}
