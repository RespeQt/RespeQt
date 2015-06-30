#ifndef SERIALPORTUNIX_H
#define SERIALPORTUNIX_H

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
    int mHandle;
    int mSpeed;
    int mMethod;

    bool setNormalSpeed();
    bool setHighSpeed();
    int speed();
    bool mHighSpeed;
    quint8 sioChecksum(const QByteArray &data, uint size);
    QByteArray readRawFrame(uint size, bool verbose = true);
    QString lastErrorMessage();
};

class AtariSioBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    AtariSioBackend(QObject *parent = 0);
    ~AtariSioBackend();

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
    int mHandle, mCancelHandles[2];
    int mSpeed;
    int mMethod;
    QString lastErrorMessage();
};

#endif // SERIALPORTUNIX_H
