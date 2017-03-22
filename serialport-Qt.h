/*
 * serialport-Qt.h
 *
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef SERIALPORT_QT_H
#define SERIALPORT_QT_H

#include "serialport.h"
#include <QtSerialPort/QSerialPort>

class QtSerialPortBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    static QString defaultPortName();

    QtSerialPortBackend(QObject *parent = 0);
    ~QtSerialPortBackend();
    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose=false);
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

private:
    bool setNormalSpeed();
    bool setHighSpeed();
    int speed();
    quint8 sioChecksum(const QByteArray &data, uint size);
    QByteArray readDataFrame(uint size, bool verbose, bool skip_initial_wait);
    QByteArray readRawFrame(uint size, bool skip_initial_wait);
    QString lastErrorMessage();

    QSerialPort mSerial;
    bool mCanceled;
    bool mHighSpeed;
    int mSpeed;
    int mMethod;
    int mWriteDelay;
    int mCompErrDelay;
    QByteArray mSioDevices;
};

#endif // SERIALPORT_QT_H
