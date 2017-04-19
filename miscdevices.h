/*
 * miscdevices.h
 *
 * Copyright 2015, 2017 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef MISCDEVICES_H
#define MISCDEVICES_H

#include "sioworker.h"

class Printer: public SioDevice
{
    Q_OBJECT
private:
    int m_lastOperation;
public:
    Printer(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
signals:
    void print(const QString &text);
};

// SmartDevice (ApeTime + URL submit)
class SmartDevice: public SioDevice
{
    Q_OBJECT

public:
    SmartDevice(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};

// RespeQt Time Server //
class RCl: public SioDevice
{
    Q_OBJECT

public:
    RCl(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);

public slots:
    void gotNewSlot (int slot);                         // 
    void fileMounted (bool mounted);                    // 

signals:
    void findNewSlot (int startFrom, bool createOne);
    void mountFile (int no, const QString fileName);
    void toggleAutoCommit (int no);                     // 

};

#endif // MISCDEVICES_H
