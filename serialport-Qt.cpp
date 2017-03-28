/*
 * serialport-Qt.cpp
 *
 * TheMontezuma
 *
 */

#include "serialport-Qt.h"
#include <QtSerialPort/QtSerialPort>
#include "errno.h"
#include "sioworker.h"
#include "respeqtsettings.h"

const int MSECS_TIMEOUT = 300;


QtSerialPortBackend::QtSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent),
      mCanceled(false),
      mHighSpeed(false)
{
}

QtSerialPortBackend::~QtSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

QString QtSerialPortBackend::defaultPortName()
{
    return QString("COM1");
}

bool QtSerialPortBackend::open()
{
    if (isOpen()) {
        close();
    }

    QString name = respeqtSettings->QtSerialPortName();
    mMethod = respeqtSettings->QtSerialPortHandshakingMethod();
    mWriteDelay = SLEEP_FACTOR * respeqtSettings->QtSerialPortWriteDelay();
    mCompErrDelay = respeqtSettings->QtSerialPortCompErrDelay();

    if (!setNormalSpeed()) {
        return false;
    }

    mSerial.setPortName(name);
    mSerial.setDataBits(QSerialPort::Data8);
    mSerial.setStopBits(QSerialPort::OneStop);
    mSerial.setParity(QSerialPort::NoParity);
    mSerial.setFlowControl(QSerialPort::NoFlowControl);

    if (!mSerial.open(QIODevice::ReadWrite))
    {
        qCritical() << "!e" << tr("Cannot open serial port '%1': %2").arg(name, lastErrorMessage());
        return false;
    }

    mSerial.flush();
    mSerial.setDataTerminalReady(true);
    mSerial.setRequestToSend(true);

    mCanceled = false;

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
    qWarning() << "!i" << tr("Emulation started through Qt serial port backend on '%1' with %2 handshaking.")
                  .arg(name)
                  .arg(m);
    return true;
}

bool QtSerialPortBackend::isOpen()
{
    return mSerial.isOpen();
}

void QtSerialPortBackend::close()
{
    mSerial.close();
}

void QtSerialPortBackend::cancel()
{
    mCanceled = true;
}

int QtSerialPortBackend::speedByte()
{
    if (respeqtSettings->QtSerialPortHandshakingMethod()==HANDSHAKE_SOFTWARE) {
        return 0x28; // standard speed (19200)
    } else if (respeqtSettings->QtSerialPortUsePokeyDivisors()) {
        return respeqtSettings->QtSerialPortPokeyDivisor();
    } else {
        int speed = 0x08;
        switch (respeqtSettings->QtSerialPortMaximumSpeed()) {
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

QByteArray QtSerialPortBackend::readCommandFrame()
{
    QByteArray data;
    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        mSerial.clear(QSerialPort::Input);

        const int size = 4;
        quint8 expected = 0;
        quint8 got = 1;
        do
        {
            if( mSerial.waitForReadyRead(MSECS_TIMEOUT) )
            {
                QByteArray all = mSerial.readAll();

                for(int i=0 ; i<all.size() ; i++)
                {
                    char c = all[i];
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
            }

        } while(!mCanceled && got!=expected);

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
        QSerialPort::PinoutSignal mask;
        switch (mMethod) {
        case HANDSHAKE_RI:
            mask = QSerialPort::RingIndicatorSignal;
            break;
        case HANDSHAKE_DSR:
            mask = QSerialPort::DataSetReadySignal;
            break;
        case HANDSHAKE_CTS:
        default:
            mask = QSerialPort::ClearToSendSignal;
            break;
        }

        int status;
        int retries = 0, totalRetries = 0;

        do {
            data.clear();

            if(mMethod == HANDSHAKE_NO_HANDSHAKE)
            {
                mSerial.clear(QSerialPort::Input);
                while( !mCanceled && !mSerial.waitForReadyRead(MSECS_TIMEOUT));
            }
            else
            {
                // RI/DSR/CTS handshake

                /* First, wait until command line goes off */
                do {
                    status = mSerial.pinoutSignals();
                    if (status & mask)
                    {
                            QThread::usleep(500);
                    }
                } while (!mCanceled && (status & mask));

                /* Now wait for it to go on again */
                do {
                    status = mSerial.pinoutSignals();
                    if((status & mask)==0)
                    {
                            QThread::usleep(500);
                    }
                } while (!mCanceled && (status & mask)==0);

                mSerial.clear(QSerialPort::Input);
            }

            if (mCanceled) {
                return data;
            }

            data = readDataFrame(4, false, (mMethod == HANDSHAKE_NO_HANDSHAKE));

            if (!data.isEmpty())
            {
                if(mMethod != HANDSHAKE_NO_HANDSHAKE)
                {
                    // wait for command line to go off
                    do {
                        status = mSerial.pinoutSignals();
                        if (status & mask)
                        {
                                QThread::usleep(500);
                        }
                    } while (!mCanceled && (status & mask));
                }
                else
                {
                    // After sending the last byte of the command frame
                    // ATARI does not drop the command line immediately.
                    // Within this small time window ATARI is not able to process the ACK byte.
                    // For the "no handshake" approach, we need to wait here a little bit.
                    QThread::usleep(500);
                }
                break;
            }
            else
            {
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
        } while (!mCanceled);
    }
    return data;
}

QByteArray QtSerialPortBackend::readDataFrame(uint size, bool verbose)
{
    return readDataFrame(size, verbose, false);
}

QByteArray QtSerialPortBackend::readDataFrame(uint size, bool verbose, bool skip_initial_wait)
{
    QByteArray data = readRawFrame(size + 1, skip_initial_wait);
    if (data.isEmpty()) {
        return data;
    }
    quint8 expected = (quint8)data.at(size);
    quint8 got = sioChecksum(data, size);
    if (expected == got)
    {
        data.resize(size);
        return data;
    }
    else
    {
        if(verbose)
        {
            qWarning() << "!w" << tr("Data frame checksum error, expected: %1, got: %2. (%3)")
                           .arg(expected)
                           .arg(got)
                           .arg(QString(data.toHex()));
        }
        data.clear();
        return data;
    }
}

bool QtSerialPortBackend::writeDataFrame(const QByteArray& data)
{
    QByteArray copy(data);
    copy.resize(copy.size() + 1);
    copy[copy.size() - 1] = sioChecksum(copy, copy.size() - 1);
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    SioWorker::usleep(50);
    return writeRawFrame(copy);
}

bool QtSerialPortBackend::writeCommandAck()
{
    return writeRawFrame(QByteArray(1, SIO_ACK));
}

bool QtSerialPortBackend::writeCommandNak()
{
    return writeRawFrame(QByteArray(1, SIO_NACK));
}

bool QtSerialPortBackend::writeDataAck()
{
    return writeRawFrame(QByteArray(1, SIO_ACK));
}

bool QtSerialPortBackend::writeDataNak()
{
    return writeRawFrame(QByteArray(1, SIO_NACK));
}

bool QtSerialPortBackend::writeComplete()
{
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, SIO_COMPLETE));
}

bool QtSerialPortBackend::writeError()
{
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, SIO_ERROR));
}

bool QtSerialPortBackend::setSpeed(int speed)
{
    bool result;
    bool wasOpen = isOpen();
    if (wasOpen)
    {
        mSerial.close();
    }

    result = mSerial.setBaudRate(speed);
    if (wasOpen && !mSerial.open(QIODevice::ReadWrite))
    {
        return false;
    }

    if(result)
    {
        mSpeed = speed;
        emit statusChanged(tr("%1 bits/sec").arg(speed));
    }
    return result;
}

bool QtSerialPortBackend::writeRawFrame(const QByteArray& data)
{
    int result;
    uint total, rest;

    total = 0;
    rest = data.count();
    QTime startTime = QTime::currentTime();

    int timeOut = data.count() * 12000 / mSpeed + 100;
    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        timeOut += 100;
    }

    int elapsed;
    do {
        result = mSerial.write(data.constData() + total, rest);

        if (result < 0 && errno != EAGAIN) {
            qCritical() << "!e" << tr("Cannot read from serial port: %1")
                           .arg(lastErrorMessage());
            return false;
        }
        if (result < 0) {
            result = 0;
        }
        total += result;
        rest -= result;
        elapsed = startTime.msecsTo(QTime::currentTime());
    } while (total < (uint)data.count() && elapsed < timeOut);

    if (total != (uint)data.count())
    {
        qCritical() << "!e" << tr("Serial port write timeout. %1 of %2 written in %3 ms").arg(total).arg(data.count()).arg(elapsed);
        return false;
    }
    mSerial.waitForBytesWritten(MSECS_TIMEOUT);
    return true;
}

void QtSerialPortBackend::setActiveSioDevices(const QByteArray& data)
{
    mSioDevices = data;
}

bool QtSerialPortBackend::setNormalSpeed()
{
    mHighSpeed = false;
    return setSpeed(19200);
}

bool QtSerialPortBackend::setHighSpeed()
{
    mHighSpeed = true;
    if (respeqtSettings->QtSerialPortUsePokeyDivisors()) {
        return setSpeed(divisorToBaud(respeqtSettings->QtSerialPortPokeyDivisor()));
    } else {
        int speed = 57600;
        switch (respeqtSettings->QtSerialPortMaximumSpeed()) {
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

int QtSerialPortBackend::speed()
{
    return mSpeed;
}

quint8 QtSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
{
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

QByteArray QtSerialPortBackend::readRawFrame(uint size, bool skip_initial_wait)
{
    QByteArray data;
    int result;
    uint total, rest;

    data.resize(size);

    total = 0;
    rest = size;
    QTime startTime = QTime::currentTime();

    int timeOut = data.count() * 12000 / mSpeed + 100;
    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        timeOut += 100;
    }

    int elapsed;
    do
    {
        if(!skip_initial_wait || total!=0)
        {
            mSerial.waitForReadyRead(MSECS_TIMEOUT);
        }
        result = mSerial.read(data.data() + total, rest);
        if (result < 0 && errno != EAGAIN) {
            qCritical() << "!e" << tr("Cannot read from serial port: %1")
                           .arg(lastErrorMessage());
            data.clear();
            return data;
        }
        if (result < 0) {
            result = 0;
        }
        total += result;
        rest -= result;
        elapsed = startTime.msecsTo(QTime::currentTime());
    } while (total < size && elapsed < timeOut);

    if ((uint)total != size)
    {
        qCritical() << "!e" << tr("Serial port read timeout. %1 of %2 read in %3 ms").arg(total).arg(data.count()).arg(elapsed);
        data.clear();
        return data;
    }
    return data;
}

QString QtSerialPortBackend::lastErrorMessage()
{
    return QString::fromUtf8(strerror(errno)) + ".";
}

