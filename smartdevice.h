/*
 * smartdevice.h
 *
 * Copyright 2015, 2017 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef SMARTDEVICE_H
#define SMARTDEVICE_H

#include "sioworker.h"

// SmartDevice (ApeTime + URL submit)
class SmartDevice: public SioDevice
{
    Q_OBJECT

public:
    SmartDevice(SioWorkerPtr worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};

#endif // SMARTDEVICE_H
