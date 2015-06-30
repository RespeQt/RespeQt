#ifndef SERIALPORTWIN32_H
#define SERIALPORTWIN32_H

#include "serialport.h"

class StandardSerialPortBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    StandardSerialPortBackend(QObject *parent = 0);
    ~StandardSerialPortBackend();

    static QString defaultPortName();

    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(const QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);

private:
    bool mCanceled;
    bool mHighSpeed;
    void *mHandle, *mCancelHandle;
    int mSpeed;
    int mMethod;

    bool setNormalSpeed();
    bool setHighSpeed();
    int speed();
    quint8 sioChecksum(const QByteArray &data, uint size);
    QByteArray readRawFrame(uint size, bool verbose = true);
    QString lastErrorMessage();
};

/* Dummy AtariSIO backend */

class AtariSioBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    static QString defaultPortName();

    AtariSioBackend(QObject *parent = 0);
    ~AtariSioBackend();
    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(const QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);
};

#endif // SERIALPORTWIN32_H
