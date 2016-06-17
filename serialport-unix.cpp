/*
 * serialport-unix.cpp
 *
 * Copyright 2015 DrVenkman
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "serialport.h"
#include "sioworker.h"
#include "headers/atarisio.h"
#include "respeqtsettings.h"
#include <QTime>
#include <QtDebug>

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef Q_OS_UNIX
    #ifdef Q_OS_LINUX
        #include <stropts.h>
        #include <termio.h>
        #include <linux/serial.h>
    #endif

    #ifdef Q_OS_MAC
        #include <termios.h>
        #include <sys/ioctl.h>
        #define IOSSIOSPEED    _IOW('T', 2, speed_t)
    #endif
#endif

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
    mHandle = -1;
}

StandardSerialPortBackend::~StandardSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

#ifdef Q_OS_LINUX
  QString StandardSerialPortBackend::defaultPortName()
  {
      return QString("/dev/ttyS0");
  }
#endif

#ifdef Q_OS_MAC
QString StandardSerialPortBackend::defaultPortName()
{
    return QString("/dev/tty.usbserial");
}
#endif

bool StandardSerialPortBackend::open()
{
    if (isOpen()) {
        close();
    }

    QString name = respeqtSettings->serialPortName();
    mMethod = respeqtSettings->serialPortHandshakingMethod();
    mWriteDelay = SLEEP_FACTOR * respeqtSettings->serialPortWriteDelay();
    mCompErrDelay = respeqtSettings->serialPortCompErrDelay();

    mHandle = ::open(name.toLocal8Bit().constData(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (mHandle < 0) {
        qCritical() << "!e" << tr("Cannot open serial port '%1': %2")
                       .arg(name, lastErrorMessage());
        return false;
    }

    if(mMethod!=HANDSHAKE_SOFTWARE)
    {
        int status;
        if (ioctl(mHandle, TIOCMGET, &status) < 0) {
            qCritical() << "!e" << tr("Cannot clear DTR and RTS lines in serial port '%1': %2").arg(name, lastErrorMessage());
            return false;
        }
        status = status & ~(TIOCM_DTR & TIOCM_RTS);
        if (ioctl(mHandle, TIOCMSET, &status) < 0) {
            qCritical() << "!e" << tr("Cannot clear DTR and RTS lines in serial port '%1': %2").arg(name, lastErrorMessage());
            return false;
        }
    }

    mCanceled = false;

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
    qWarning() << "!i" << tr("Emulation started through standard serial port backend on '%1' with %2 handshaking.")
                  .arg(respeqtSettings->serialPortName())
                  .arg(m);

    return true;
}

bool StandardSerialPortBackend::isOpen()
{
    return mHandle >= 0;
}

void StandardSerialPortBackend::close()
{
    cancel();
    if (::close(mHandle)) {
        qCritical() << "!e" << tr("Cannot close serial port: %1")
                       .arg(lastErrorMessage());
    }
    mHandle = -1;
}

void StandardSerialPortBackend::cancel()
{
    mCanceled = true;
}

int StandardSerialPortBackend::speedByte()
{
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

#ifdef Q_OS_LINUX
bool StandardSerialPortBackend::setSpeed(int speed)
{
    termios tios;
    struct serial_struct ss;

    tcgetattr(mHandle, &tios);
    tios.c_cflag &= ~CSTOPB;
    cfmakeraw(&tios);
    switch (speed) {
        case 600:
            cfsetispeed(&tios, B600);
            cfsetospeed(&tios, B600);
            break;
        case 19200:
            cfsetispeed(&tios, B19200);
            cfsetospeed(&tios, B19200);
            break;
        case 38400:
            // reset special handling of 38400bps back to 38400
            ioctl(mHandle, TIOCGSERIAL, &ss);
            ss.flags &= ~ASYNC_SPD_MASK;
            ioctl(mHandle, TIOCSSERIAL, &ss);

            cfsetispeed(&tios, B38400);
            cfsetospeed(&tios, B38400);
            break;
        case 57600:
            cfsetispeed(&tios, B57600);
            cfsetospeed(&tios, B57600);
            break;
        default:
            // configure port to use custom speed instead of 38400
            ioctl(mHandle, TIOCGSERIAL, &ss);
            ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
            ss.custom_divisor = (ss.baud_base + (speed / 2)) / speed;
            int customSpeed = ss.baud_base / ss.custom_divisor;

            if (customSpeed < speed * 98 / 100 || customSpeed > speed * 102 / 100) {
                qCritical() << "!e" << tr("Cannot set serial port speed to %1: %2").arg(speed).arg(tr("Closest possible speed is %2.").arg(customSpeed));
                return false;
            }

            ioctl(mHandle, TIOCSSERIAL, &ss);

            cfsetispeed(&tios, B38400);
            cfsetospeed(&tios, B38400);
            break;
    }

    /* Set serial port state */
    if (tcsetattr(mHandle, TCSANOW, &tios) != 0) {
        qCritical() << "!e" << tr("Cannot set serial port speed to %1: %2")
                       .arg(speed)
                       .arg(lastErrorMessage());
        return false;
    }

    emit statusChanged(tr("%1 bits/sec").arg(speed));
    qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);
    mSpeed = speed;
    return true;
}
#endif

#ifdef Q_OS_MAC
bool StandardSerialPortBackend::setSpeed(int speed)
{
    termios tios;

    tcgetattr(mHandle, &tios);
    tios.c_cflag &= ~CSTOPB;
    cfmakeraw(&tios);
    tios.c_cflag = CREAD | CLOCAL;     // turn on READ
    tios.c_cflag |= CS8;
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 10;     // 1 sec timeout

    if (ioctl(mHandle, TIOCSETA, &tios) != 0) {
        qCritical() << "!e" << tr("Failed to set serial attrs");
        return false;
    }
    switch (speed) {
        case 600:
            cfsetispeed(&tios, B600);
            cfsetospeed(&tios, B600);
            break;
        case 19200:
            cfsetispeed(&tios, B19200);
            cfsetospeed(&tios, B19200);
            break;
        case 38400:
            cfsetispeed(&tios, B38400);
            cfsetospeed(&tios, B38400);
            break;
        case 57600:
            cfsetispeed(&tios, B57600);
            cfsetospeed(&tios, B57600);
            break;
        default:
           if (ioctl(mHandle, IOSSIOSPEED, &speed) < 0 ){
                qCritical() << "!e" << tr("Failed to set serial port speed to %1").arg(speed);
                return false;
            }
           break;
    }

    /* Set serial port state */
    if (tcsetattr(mHandle, TCSANOW, &tios) != 0) {
        qCritical() << "!e" << tr("Cannot set serial port speed to %1: %2")
                       .arg(speed)
                       .arg(lastErrorMessage());
        return false;
    }
   emit statusChanged(tr("%1 bits/sec").arg(speed));
    qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);
    mSpeed = speed;
    return true;
}
#endif

int StandardSerialPortBackend::speed()
{
    return mSpeed;
}

QByteArray StandardSerialPortBackend::readCommandFrame()
{
    QByteArray data;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        if (tcflush(mHandle, TCIFLUSH) != 0)
        {
            qCritical() << "!e" << tr("Cannot clear serial port read buffer: %1").arg(lastErrorMessage());
            return data;
        }

        const int size = 4;
        quint8 expected = 0;
        quint8 got = 1;
        do
        {
            char c;
            if(1 == ::read(mHandle, &c, 1))
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
        // RI/DSR/CTS/- handshake

        int mask;
        switch (mMethod) {
        case HANDSHAKE_RI:
            mask = TIOCM_RI;
            break;
        case HANDSHAKE_DSR:
            mask = TIOCM_DSR;
            break;
        case HANDSHAKE_CTS:
        default:
            mask = TIOCM_CTS;
            break;
        }

        int status;
        int retries = 0, totalRetries = 0;

        do {
            data.clear();

            if(mMethod == HANDSHAKE_NO_HANDSHAKE)
            {
                int bytes;
                do {
                    ioctl(mHandle, FIONREAD, &bytes);
                    #ifdef Q_OS_UNIX
                        QThread::yieldCurrentThread();
                    #endif

                    #ifdef Q_OS_MAC
                        QThread::usleep(300);
                    #endif
                } while ((bytes==0) && !mCanceled);
            }
            else
            {
                // RI/DSR/CTS handshake
                /* First, wait until command line goes off */
                do {
                    if (ioctl(mHandle, TIOCMGET, &status) < 0) {
                        qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                        return data;
                    }
                    if (status & mask) {
                        #ifdef Q_OS_UNIX
                            QThread::yieldCurrentThread();   // Venkman 07132015 OS definition blocks added
                        #endif

                        #ifdef Q_OS_MAC
                            QThread::usleep(500);
                        #endif
                    }
                } while ((status & mask) && !mCanceled);
                /* Now wait for it to go on again */
                do {
                    if (ioctl(mHandle, TIOCMGET, &status) < 0) {
                        qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                        return data;
                    }
                    if (!(status & mask)) {
                        #ifdef Q_OS_UNIX
                            QThread::yieldCurrentThread();   // Venkman 07132015 OS definition blocks added
                        #endif

                        #ifdef Q_OS_MAC
                           QThread::usleep(500);
                        #endif
                    }
                } while (!(status & mask) && !mCanceled);

                if (tcflush(mHandle, TCIFLUSH) != 0) {
                    qCritical() << "!e" << tr("Cannot clear serial port read buffer: %1")
                                   .arg(lastErrorMessage());
                    return data;
                }
            }

            if (mCanceled) {
                return data;
            }

            data = readDataFrame(4, false);

            if (!data.isEmpty()) {
                if(mMethod != HANDSHAKE_NO_HANDSHAKE)
                {
                    // wait for command line to go off
                    do {
                        if (ioctl(mHandle, TIOCMGET, &status) < 0) {
                            qCritical() << "!e" << tr("Cannot retrieve serial port status: %1").arg(lastErrorMessage());
                            return data;
                        }
                    } while ((status & mask) && !mCanceled);
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
    QByteArray data = readRawFrame(size + 1, verbose);
    if (data.isEmpty()) {
        return NULL;
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
    QByteArray copy(data);
    copy.resize(copy.size() + 1);
    copy[copy.size() - 1] = sioChecksum(copy, copy.size() - 1);
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    SioWorker::usleep(50);
    return writeRawFrame(copy);
}

bool StandardSerialPortBackend::writeCommandAck()
{
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeCommandNak()
{
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeDataAck()
{
    return writeRawFrame(QByteArray(1, 65));
}

bool StandardSerialPortBackend::writeDataNak()
{
    return writeRawFrame(QByteArray(1, 78));
}

bool StandardSerialPortBackend::writeComplete()
{
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, 67));
}

bool StandardSerialPortBackend::writeError()
{
    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, 69));
}

quint8 StandardSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
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

QByteArray StandardSerialPortBackend::readRawFrame(uint size, bool verbose)
{
    QByteArray data;
    int result;
    uint total, rest;

    data.resize(size);

    total = 0;
    rest = size;
    QTime startTime = QTime::currentTime();
    int timeOut;
    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        timeOut = data.count() * 24000 / mSpeed + 100;
    }
    else
    {
        timeOut = data.count() * 12000 / mSpeed + 10;
    }
    int elapsed;

    do {
        result = ::read(mHandle, data.data() + total, rest);
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
        elapsed = QTime::currentTime().msecsTo(startTime);
    } while (total < size && elapsed > -timeOut);

    if ((uint)total != size) {
        if (verbose) {
            data.resize(total);
            qCritical() << "!e" << tr("Serial port read timeout.");
        }
        data.clear();
        return data;
    }
    return data;
}

bool StandardSerialPortBackend::writeRawFrame(const QByteArray &data)
{
    int result;
    uint total, rest;

    total = 0;
    rest = data.count();
    QTime startTime = QTime::currentTime();
    int timeOut;
    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        timeOut = data.count() * 24000 / mSpeed + 100;
    }
    else
    {
        timeOut = data.count() * 12000 / mSpeed + 10;
    }
    int elapsed;

    do {
        result = ::write(mHandle, data.constData() + total, rest);
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
        elapsed = QTime::currentTime().msecsTo(startTime);
    } while (total < (uint)data.count() && elapsed > -timeOut);

    if (total != (uint)data.count()) {
        qCritical() << "!e" << tr("Serial port write timeout. (%1 of %2 written)").arg(total).arg(data.count());
        return false;
    }

    if (tcdrain(mHandle) != 0) {
        qCritical() << "!e" << tr("Cannot flush serial port write buffer: %1")
                       .arg(lastErrorMessage());
        return false;
    }

    return true;
}

QString StandardSerialPortBackend::lastErrorMessage()
{
    return QString::fromUtf8(strerror(errno)) + ".";
}

void StandardSerialPortBackend::setActiveSioDevices(const QByteArray &data)
{
    mSioDevices = data;
}

AtariSioBackend::AtariSioBackend(QObject *parent)
    : AbstractSerialPortBackend(parent)
{
    mHandle = -1;
}

AtariSioBackend::~AtariSioBackend()
{
    if (isOpen()) {
        close();
    }
}

QString AtariSioBackend::defaultPortName()
{
    return QString("/dev/atarisio0");
}

bool AtariSioBackend::open()
{
    if (isOpen()) {
        close();
    }

    QString name = respeqtSettings->atariSioDriverName();

    mHandle = ::open(name.toLocal8Bit().constData(), O_RDWR);

    if (mHandle < 0) {
        qCritical() << "!e" << tr("Cannot open serial port '%1': %2")
                       .arg(name, lastErrorMessage());
        return false;
    }

    int version;
    version = ioctl(mHandle, ATARISIO_IOC_GET_VERSION);

    if (version < 0) {
        qCritical() << "!e" << tr("Cannot open AtariSio driver '%1': %2")
                      .arg(name).arg("Cannot determine AtariSio version.");
        close();
        return false;
    }

    if ((version >> 8) != (ATARISIO_VERSION >> 8) ||
         (version & 0xff) < (ATARISIO_VERSION & 0xff)) {
        qCritical() << "!e" << tr("Cannot open AtariSio driver '%1': %2")
                      .arg(name).arg("Incompatible AtariSio version.");
        close();
        return false;
    }

    int mode;

    mMethod = respeqtSettings->atariSioHandshakingMethod();

    switch (mMethod) {
    case HANDSHAKE_RI:
        mode = ATARISIO_MODE_SIOSERVER;
        break;
    case HANDSHAKE_DSR:
        mode = ATARISIO_MODE_SIOSERVER_DSR;
        break;
    default:
    case HANDSHAKE_CTS:
        mode = ATARISIO_MODE_SIOSERVER_CTS;
        break;
    }

    if (ioctl(mHandle, ATARISIO_IOC_SET_MODE, mode) < 0) {
        qCritical() << "!e" << tr("Cannot set AtariSio driver mode: %1")
                       .arg(lastErrorMessage());
        close();
        return false;
    }

    if (ioctl(mHandle, ATARISIO_IOC_SET_AUTOBAUD, true) < 0) {
        qCritical() << "!e" << tr("Cannot set AtariSio to autobaud mode: %1")
                       .arg(lastErrorMessage());
        close();
        return false;
    }

    if (pipe(mCancelHandles) != 0) {
        qCritical() << "!e" << tr("Cannot create the cancel pipe");
    }

    QString m;
    switch (mMethod) {
    case HANDSHAKE_RI:
        m = "RI";
        break;
    case HANDSHAKE_DSR:
        m = "DSR";
        break;
    default:
    case HANDSHAKE_CTS:
        m = "CTS";
        break;
    }

    /* Notify the user that emulation is started */
    qWarning() << "!i" << tr("Emulation started through AtariSIO backend on '%1' with %2 handshaking.")
                  .arg(respeqtSettings->atariSioDriverName())
                  .arg(m);

    return true;
}

bool AtariSioBackend::isOpen()
{
    return mHandle >= 0;
}

void AtariSioBackend::close()
{
    cancel();
    if (::close(mHandle)) {
        qCritical() << "!e" << tr("Cannot close serial port: %1")
                       .arg(lastErrorMessage());
    }
    ::close(mCancelHandles[0]);
    ::close(mCancelHandles[1]);
    mHandle = -1;
}

void AtariSioBackend::cancel()
{
    if (::write(mCancelHandles[1], "C", 1) < 1) {
        qCritical() << "!e" << tr("Cannot stop AtariSio backend.");
    }
}

bool AtariSioBackend::setSpeed(int speed)
{
    if (ioctl(mHandle, ATARISIO_IOC_SET_BAUDRATE, speed) < 0) {
        qCritical() << "!e" << tr("Cannot set AtariSio speed to %1: %2").arg(speed).arg(lastErrorMessage());
        return false;
    } else {
        emit statusChanged(tr("%1 bits/sec").arg(speed));
        qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);
        return true;
    }
}

QByteArray AtariSioBackend::readCommandFrame()
{
    QByteArray data;

    fd_set read_set;
    fd_set except_set;

    int ret;

    int maxfd = mHandle;

    FD_ZERO(&except_set);
    FD_SET(mHandle, &except_set);

    if (mCancelHandles[0] > maxfd) {
        maxfd = mCancelHandles[0];
    }
    FD_ZERO(&read_set);
    FD_SET(mCancelHandles[0], &read_set);

    ret = select(maxfd+1, &read_set, NULL, &except_set, 0);
    if (ret == -1 || ret == 0) {
        return data;
    }
    if (FD_ISSET(mCancelHandles[0], &read_set)) {
        return data;
    }
    if (FD_ISSET(mHandle, &except_set)) {
        SIO_command_frame frame;
        if (ioctl(mHandle, ATARISIO_IOC_GET_COMMAND_FRAME, &frame) < 0) {
            return data;
        }
        data.resize(4);
        data[0] = frame.device_id;
        data[1] = frame.command;
        data[2] = frame.aux1;
        data[3] = frame.aux2;

        int sp = ioctl(mHandle, ATARISIO_IOC_GET_BAUDRATE);
        if (sp >= 0 && mSpeed != sp) {
            emit statusChanged(tr("%1 bits/sec").arg(sp));
            qWarning() << "!i" << tr("Serial port speed set to %1.").arg(sp);
            mSpeed = sp;
        }

        return data;
    }
    qCritical() << "!e" << tr("Illegal condition using select!");
    return data;
}

int AtariSioBackend::speedByte()
{
    return 0x08;
}

QByteArray AtariSioBackend::readDataFrame(uint size, bool verbose)
{
    QByteArray data;
    SIO_data_frame frame;

    data.resize(size);
    frame.data_buffer = (unsigned char*)data.data();
    frame.data_length = size;

    if (ioctl(mHandle, ATARISIO_IOC_RECEIVE_DATA_FRAME, &frame) < 0 ) {
        if (verbose) {
            qCritical() << "!e" << tr("Cannot read data frame: %1").arg(lastErrorMessage());
        }
        data.clear();
    }

    return data;
}

bool AtariSioBackend::writeDataFrame(const QByteArray &data)
{
    SIO_data_frame frame;

    frame.data_buffer = (unsigned char*)data.constData();
    frame.data_length = data.size();

    if (ioctl(mHandle, ATARISIO_IOC_SEND_DATA_FRAME, &frame) < 0 ) {
        qCritical() << "!e" << tr("Cannot write data frame: %1").arg(lastErrorMessage());
        return false;
    }

    return true;
}

bool AtariSioBackend::writeCommandAck()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_COMMAND_ACK) < 0 ) {
        if (errno != EATARISIO_COMMAND_TIMEOUT) {
            qCritical() << "!e" << tr("Cannot write command ACK: %1").arg(lastErrorMessage());
        }
        return false;
    }
    return true;
}

bool AtariSioBackend::writeCommandNak()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_COMMAND_NAK) < 0 ) {
        qCritical() << "!e" << tr("Cannot write command NAK: %1").arg(lastErrorMessage());
        return false;
    }
    return true;
}

bool AtariSioBackend::writeDataAck()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_DATA_ACK) < 0 ) {
        qCritical() << "!e" << tr("Cannot write data ACK: %1").arg(lastErrorMessage());
        return false;
    }
    return true;
}

bool AtariSioBackend::writeDataNak()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_DATA_NAK) < 0 ) {
        qCritical() << "!e" << tr("Cannot write data NAK: %1").arg(lastErrorMessage());
        return false;
    }
    return true;
}

bool AtariSioBackend::writeComplete()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_COMPLETE) < 0 ) {
        qCritical() << "!e" << tr("Cannot write COMPLETE byte: %1").arg(lastErrorMessage());
        return false;
    }
    return true;
}

bool AtariSioBackend::writeError()
{
    if (ioctl(mHandle, ATARISIO_IOC_SEND_ERROR) < 0 ) {
        qCritical() << "!e" << tr("Cannot write ERROR byte: %1").arg(lastErrorMessage());
        return false;
    }
    return true;
}

bool AtariSioBackend::writeRawFrame(const QByteArray &data)
{
    SIO_data_frame frame;

    frame.data_buffer = (unsigned char*)data.constData();
    frame.data_length = data.size();

    if (ioctl(mHandle, ATARISIO_IOC_SEND_RAW_FRAME, &frame) < 0 ) {
        qCritical() << "!e" << tr("Cannot write raw frame: %1").arg(lastErrorMessage());
        return false;
    }

    return true;
}

QString AtariSioBackend::lastErrorMessage()
{
    switch (errno) {
        case EATARISIO_ERROR_BLOCK_TOO_LONG:
            return tr("Block too long.");
            break;
        case EATARISIO_COMMAND_NAK:
            return tr("Command not acknowledged.");
            break;
        case EATARISIO_COMMAND_TIMEOUT:
            return tr("Command timeout.");
            break;
        case EATARISIO_CHECKSUM_ERROR:
            return tr("Checksum error.");
            break;
        case EATARISIO_COMMAND_COMPLETE_ERROR:
            return tr("Device error.");
            break;
        case EATARISIO_DATA_NAK:
            return tr("Data frame not acknowledged.");
            break;
        case EATARISIO_UNKNOWN_ERROR:
            return tr("Unknown AtariSio driver error.");
            break;
        default:
            return QString::fromUtf8(strerror(errno)) + ".";
    }
}

void AtariSioBackend::setActiveSioDevices(const QByteArray &){}
