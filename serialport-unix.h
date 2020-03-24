/*
 * serialport-unix.h
 *
 * Copyright 2016 TheMontezuma
 * Copyright 2017 blind
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

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
    void setActiveSioDevices(const QByteArray &data);
    int speed();
    void forceHighSpeed(int speed);

private:
    bool mCanceled;
    bool mHighSpeed;
    int mForceHighSpeed;
    int mHandle;
    int mSpeed;
    int mMethod;
    int mWriteDelay;
    int mCompErrDelay;
    QByteArray mSioDevices;

    bool setNormalSpeed();
    bool setHighSpeed();
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
    void setActiveSioDevices(const QByteArray &data);
    int speed();
    void forceHighSpeed(int speed);

private:
    int mHandle, mCancelHandles[2];
    int mSpeed;
    int mMethod;
    QString lastErrorMessage();
};

#endif // SERIALPORTUNIX_H
