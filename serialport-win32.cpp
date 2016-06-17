/*
 * serialport-win32.cpp
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include <windows.h>
#include <string.h>
#include "serialport-win32.h"
#include "sioworker.h"
#include "respeqtsettings.h"

#include <QTime>
#include <QtDebug>

/*********/

AbstractSerialPortBackend::AbstractSerialPortBackend(QObject *parent)
    : QObject(parent)
{
}

AbstractSerialPortBackend::~AbstractSerialPortBackend()
{
}

StandardSerialPortBackend::StandardSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent)
{
    mHandle = INVALID_HANDLE_VALUE;
}

StandardSerialPortBackend::~StandardSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

QString StandardSerialPortBackend::defaultPortName()
{
    return QString("COM1");
}

bool StandardSerialPortBackend::open()
{
    Sleep(250);        // 
    if (isOpen()) {
        close();
    }
//    qDebug() << "!d" << tr("DBG -- Serial Port Open...");

    QString name("\\\\.\\");
    name.append(respeqtSettings->serialPortName());

    mMethod = respeqtSettings->serialPortHandshakingMethod();
    mWriteDelay = SLEEP_FACTOR * respeqtSettings->serialPortWriteDelay();
    mCompErrDelay = respeqtSettings->serialPortCompErrDelay();

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        mHandle = (CreateFile(
            (WCHAR*)name.utf16(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_WRITE_THROUGH,
            0
        ));
        if (mHandle == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot open serial port '%1': %2").arg(respeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
    }
    else
    {
        mHandle = (CreateFile(
            (WCHAR*)name.utf16(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            0
        ));
        if (mHandle == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot open serial port '%1': %2").arg(respeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
        if (!EscapeCommFunction(mHandle, CLRRTS)) {
            qCritical() << "!e" << tr("Cannot clear RTS line in serial port '%1': %2").arg(respeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
        if (!EscapeCommFunction(mHandle, CLRDTR)) {
            qCritical() << "!e" << tr("Cannot clear DTR line in serial port '%1': %2").arg(respeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
    }

    mCanceled = false;

    mCancelHandle = CreateEvent(0, true, false, 0);

    if (!setNormalSpeed()) {
        close();
        return false;
    }

    QString m;
    switch (mMethod) {
    case HANDSHAKE_RI:
        m = "RI";
        break;
    case HANDSHAKE_DSR:
        m = "DSR";
        break;
    case HANDSHAKE_CTS:
        m = "CTS";
        break;
    case HANDSHAKE_NO_HANDSHAKE:
        m = "NO";
        break;
    case HANDSHAKE_SOFTWARE:
    default:
        m = "SOFTWARE (SIO2BT)";
        break;
    }

    /* Notify the user that emulation is started */
    qWarning() << "!i" << tr("Emulation started through standard serial port backend on '%1' with %2 handshaking")
                  .arg(respeqtSettings->serialPortName())
                  .arg(m);

    return true;
}

bool StandardSerialPortBackend::isOpen()
{
    return mHandle != INVALID_HANDLE_VALUE;
}

void StandardSerialPortBackend::close()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port Close...");

    cancel();
    if (!CloseHandle(mHandle)) {
        qCritical() << "!e" << tr("Cannot close serial port: %1").arg(lastErrorMessage());
    }
    CloseHandle(mCancelHandle);
    mCancelHandle = mHandle = INVALID_HANDLE_VALUE;
}

void StandardSerialPortBackend::cancel()
{
    mCanceled = true;
    SetEvent(mCancelHandle);
}

int StandardSerialPortBackend::speedByte()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port speedByte...");

    if (respeqtSettings->serialPortHandshakingMethod()==HANDSHAKE_SOFTWARE) {
        return 0x28; // standard speed (19200)
    } else if (respeqtSettings->serialPortUsePokeyDivisors()) {
        return respeqtSettings->serialPortPokeyDivisor();
    } else {
        int speed = 0x08;
        switch (respeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 0x28;
            break;
        case 1:
            speed = 0x10;
            break;
        case 2:
            speed = 0x08;
            break;
        }
        return speed;
    }
}

bool StandardSerialPortBackend::setNormalSpeed()
{
    mHighSpeed = false;
    return setSpeed(19200);
}

bool StandardSerialPortBackend::setHighSpeed()
{
    mHighSpeed = true;
    if (respeqtSettings->serialPortUsePokeyDivisors()) {
        return setSpeed(divisorToBaud(respeqtSettings->serialPortPokeyDivisor()));
    } else {
        int speed = 57600;
        switch (respeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 19200;
            break;
        case 1:
            speed = 38400;
            break;
        case 2:
            speed = 57600;
            break;
        }
        return setSpeed(speed);
    }
}

bool StandardSerialPortBackend::setSpeed(int speed)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port setSpeed...");

    DCB dcb;
    COMMTIMEOUTS to;

    /* Adjust parameters */
    dcb.DCBlength = sizeof dcb;
    dcb.BaudRate = speed;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    dcb.fDummy2 = 0;
    dcb.wReserved = 0;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
/*    if (speed > 100000) {
        dcb.StopBits = TWOSTOPBITS;
    } else*/ {
        dcb.StopBits = ONESTOPBIT;
    }
    dcb.XonChar = 0;
    dcb.XoffChar = 0;
    dcb.ErrorChar = 0;
    dcb.EofChar = 0;
    dcb.EvtChar = 0;
    dcb.wReserved1 = 0;

    /* Set serial port state */
    if (!SetCommState(mHandle, &dcb)) {
        qCritical() << "!e" << tr("Cannot set serial port speed to %1: %2")
                       .arg(speed)
                       .arg(lastErrorMessage());
        return false;
    }

    /* Adjust serial port timeouts */
    to.ReadIntervalTimeout = 0;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        to.ReadTotalTimeoutConstant = 300;
        to.WriteTotalTimeoutConstant = 300;
        to.ReadTotalTimeoutMultiplier = 200;
        to.WriteTotalTimeoutMultiplier = 200;
    }
    else
    {
        to.ReadTotalTimeoutConstant = 0;
        to.WriteTotalTimeoutConstant = 0;
        to.ReadTotalTimeoutMultiplier = 100;
        to.WriteTotalTimeoutMultiplier = 100;
    }

    /* Set serial port timeouts */
    if (!SetCommTimeouts(mHandle, &to)) {
        qCritical() << "!e" << tr("Cannot set serial port timeouts: %1").arg(lastErrorMessage());
        return false;
    }

    emit statusChanged(tr("%1 bits/sec").arg(speed));
    qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);
    mSpeed = speed;
    return true;
}

int StandardSerialPortBackend::speed()
{
    return mSpeed;
}

QByteArray StandardSerialPortBackend::readCommandFrame()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port readCommandFrame...");

    QByteArray data;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        if (!PurgeComm(mHandle, PURGE_RXCLEAR)) {
            qCritical() << "!e" << tr("Cannot clear serial port read buffer: %1").arg(lastErrorMessage());
            return data;
        }

        const int size = 4;
        quint8 expected = 0;
        quint8 got = 1;
        do
        {
            DWORD result;
            char c;
            if(ReadFile(mHandle, &c, 1, &result, NULL) && result == 1)
            {
                data.append(c);
                if(data.size()==size+2)
                {
                    data.remove(0,1);
                }
                if(data.size()==size+1)
                {
                    for(int i=0 ; i<mSioDevices.size() ; i++)
                    {
                        if(data.at(0)==mSioDevices[i])
                        {
                            expected = (quint8)data.at(size);
                            got = sioChecksum(data, size);
                            break;
                        }
                    }
                }
            }
        } while(got!=expected && !mCanceled);

        if(got==expected)
        {
            data.resize(size);
            // After sending the last byte of the command frame
            // ATARI does not drop the command line immediately.
            // Within this small time window ATARI is not able to process the ACK byte.
            // For the "software handshake" approach, we need to wait here a little bit.
            QThread::usleep(500);
        }
        else
        {
            data.clear();
        }
    }
    else
    {
        DWORD MASK;
        DWORD MODEM_STAT;
        DWORD tmp;

        switch (mMethod) {
        case HANDSHAKE_RI:
            MASK = EV_RING;
            MODEM_STAT = MS_RING_ON;
            break;
        case HANDSHAKE_DSR:
            MASK = EV_DSR;
            MODEM_STAT = MS_DSR_ON;
            break;
        case HANDSHAKE_CTS:
            MASK = EV_CTS;
            MODEM_STAT = MS_CTS_ON;
            break;
        case HANDSHAKE_NO_HANDSHAKE:
        default:
            MASK = EV_RXCHAR;
            MODEM_STAT = 0;
            break;
        }

        if (!SetCommMask(mHandle, MASK)) {
            qCritical() << "!e" << tr("Cannot set serial port event mask: %1").arg(lastErrorMessage());
            return data;
        }

        int retries = 0, totalRetries = 0;
        do {
            data.clear();
            OVERLAPPED ov;

            memset(&ov, 0, sizeof(ov));
            ov.hEvent = CreateEvent(0, true, false, 0);

            HANDLE events[2];
            events[0] = ov.hEvent;
            events[1] = mCancelHandle;

            if (!WaitCommEvent(mHandle, &tmp, &ov)) {
                if (GetLastError() == ERROR_IO_PENDING) {
                    DWORD x = WaitForMultipleObjects(2, events, false, INFINITE);
                    CloseHandle(ov.hEvent);
                    if (x == WAIT_OBJECT_0 + 1) {
                        data.clear();
                        return data;
                    }
                    if (x == WAIT_FAILED) {
                        qCritical() << "!e" << tr("Cannot wait for serial port event: %1").arg(lastErrorMessage());
                        data.clear();
                        return data;
                    }
                } else {
                    CloseHandle(ov.hEvent);
                    qCritical() << "!e" << tr("Cannot wait for serial port event: %1").arg(lastErrorMessage());
                    return data;
                }
            }

            // for hardware handshake methods check if the command line status is ON
            if( (MODEM_STAT != 0) && (!GetCommModemStatus(mHandle, &tmp) || !(tmp & MODEM_STAT)) )continue;

            if(MASK != EV_RXCHAR)
            {            
               if (!PurgeComm(mHandle, PURGE_RXCLEAR)) {
                   qCritical() << "!e" << tr("Cannot clear serial port read buffer: %1").arg(lastErrorMessage());
                   return data;
               }
            }

    //        qDebug() << "!d" << tr("DBG -- Serial Port, just about to readDataFrame...");

            data = readDataFrame(4, false);

            if (!data.isEmpty()) {

//            qDebug() << "!d" << tr("DBG -- Serial Port, data not empty: [%1]").arg(data.data());

                if(MASK != EV_RXCHAR) { //
                    do {
                        GetCommModemStatus(mHandle, &tmp);
                    } while ((tmp & MODEM_STAT) && !mCanceled);
                }
                else
                {
                    // After sending the last byte of the command frame
                    // ATARI does not drop the command line immediately.
                    // Within this small time window ATARI is not able to process the ACK byte.
                    // For the "software handshake" approach, we need to wait here a little bit.
                    QThread::usleep(500);
                }
                break;
            } else {
                retries++;
                totalRetries++;
                if (retries == 2) {
                    retries = 0;
                    if (mHighSpeed) {
                        setNormalSpeed();
                    } else {
                        setHighSpeed();
                    }
                }
            }
    //    } while (totalRetries < 100);
        } while (1);
    }
    return data;
}

QByteArray StandardSerialPortBackend::readDataFrame(uint size, bool verbose)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port readDataFrame...");

    QByteArray data = readRawFrame(size + 1, verbose);
    if (data.isEmpty()) {
        return data;
    }
    quint8 expected = (quint8)data.at(size);
    quint8 got = sioChecksum(data, size);
    if (expected == got) {
        data.resize(size);
        return data;
    } else {
        if (verbose) {
            qWarning() << "!w" << tr("Data frame checksum error, expected: %1, got: %2. (%3)")
                           .arg(expected)
                           .arg(got)
                           .arg(QString(data.toHex()));
        }
        data.clear();
        return data;
    }
}

bool StandardSerialPortBackend::writeDataFrame(const QByteArray &data)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeDataFrame...");

    QByteArray copy(data);
    copy.resize(copy.size() + 1);
    copy[copy.size() - 1] = sioChecksum(copy, copy.size() - 1);
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    SioWorker::usleep(50);
    return writeRawFrame(copy);
}

bool StandardSerialPortBackend::writeCommandAck()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeCommandAck...");

    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeCommandNak()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeCommandNak...");

    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeDataAck()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeDataAck...");

    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeDataNak()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeDataNak...");

    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeComplete()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeComplete...");

    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, 67));
}

bool StandardSerialPortBackend::writeError()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeError...");

    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, 69));
}

quint8 StandardSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port sioChecksum...");

    uint i;
    uint sum = 0;

    for (i=0; i < size; i++) {
        sum += (quint8)data.at(i);
        if (sum > 255) {
            sum -= 255;
        }
    }

    return sum;
}

QByteArray StandardSerialPortBackend::readRawFrame(uint size, bool verbose)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port readRawFrame...");

    QByteArray data;
    DWORD result;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        data.resize(size);
        if (!ReadFile(mHandle, data.data(), size, &result, NULL) || (result != (DWORD)size))
        {
            data.clear();
        }
    }
    else
    {
        OVERLAPPED ov;

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEvent(0, true, false, 0);

        if (ov.hEvent == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot create event: %1").arg(lastErrorMessage());
            return data;
        }

        data.resize(size);
        if (!ReadFile(mHandle, data.data(), size, &result, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(mHandle, &ov, &result, true)) {
                    qCritical() << "!e" << tr("Cannot read from serial port: %1").arg(lastErrorMessage());
                    data.clear();
                    CloseHandle(ov.hEvent);
                    return data;
                }
            } else {
                qCritical() << "!e" << tr("Cannot read from serial port: %1").arg(lastErrorMessage());
                data.clear();
                CloseHandle(ov.hEvent);
                return data;
            }
        }
        CloseHandle(ov.hEvent);
        if (result != (DWORD)size)
        {
            if(verbose)
            {
                qCritical() << "!e" << tr("Serial port read timeout.");
            }
            data.clear();
            return data;
        }
    }
    return data;
}

bool StandardSerialPortBackend::writeRawFrame(const QByteArray &data)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeRawFrame...");

    DWORD result;

    if (!PurgeComm(mHandle, PURGE_TXCLEAR)) {
        qCritical() << "!e" << tr("Cannot clear serial port write buffer: %1").arg(lastErrorMessage());
        return false;
    }

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        return WriteFile(mHandle, data.constData(), data.size(), &result, NULL);
    }
    else
    {
        OVERLAPPED ov;

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEvent(0, true, false, 0);

        if (!WriteFile(mHandle, data.constData(), data.size(), &result, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(mHandle, &ov, &result, true)) {
                    qCritical() << "!e" << tr("Cannot write to serial port: %1").arg(lastErrorMessage());
                    CloseHandle(ov.hEvent);
                    return false;
                }
            } else {
                qCritical() << "!e" << tr("Cannot write to serial port: %1").arg(lastErrorMessage());
                return false;
            }
        }
        CloseHandle(ov.hEvent);
    }

    if (result != (DWORD)data.size()) {
        qCritical() << "!e" << tr("Serial port write timeout.");
        return false;
    }
    return true;
}

QString StandardSerialPortBackend::lastErrorMessage()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port lastErrorMessage...");

    QString result;
    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    result.setUtf16((ushort *)lpMsgBuf, wcslen((wchar_t*)lpMsgBuf) - 2);
    LocalFree(lpMsgBuf);
    return result;
}

void StandardSerialPortBackend::setActiveSioDevices(const QByteArray &data)
{
    mSioDevices = data;
}

/* Dummy AtariSIO backend */

bool AtariSioBackend::open()
{
    qCritical() << "!e" << tr("AtariSIO is only available under Linux.");
    return false;
}

QString AtariSioBackend::defaultPortName()
{
    return QString();
}

AtariSioBackend::AtariSioBackend(QObject *) {}
AtariSioBackend::~AtariSioBackend() {}
bool AtariSioBackend::isOpen() {return false;}
void AtariSioBackend::close() {}
void AtariSioBackend::cancel() {}
int AtariSioBackend::speedByte() {return 0;}
QByteArray AtariSioBackend::readCommandFrame() {return QByteArray();}
QByteArray AtariSioBackend::readDataFrame(uint, bool) {return QByteArray();}
bool AtariSioBackend::writeDataFrame(const QByteArray &) {return false;}
bool AtariSioBackend::writeCommandAck() {return false;}
bool AtariSioBackend::writeCommandNak() {return false;}
bool AtariSioBackend::writeDataAck() {return false;}
bool AtariSioBackend::writeDataNak() {return false;}
bool AtariSioBackend::writeComplete() {return false;}
bool AtariSioBackend::writeError() {return false;}
bool AtariSioBackend::setSpeed(int ) {return false;}
bool AtariSioBackend::writeRawFrame(const QByteArray &) {return false;}
void AtariSioBackend::setActiveSioDevices(const QByteArray &){}
