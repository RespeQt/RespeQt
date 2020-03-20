/*
 * autoboot.cpp
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef AUTOBOOT_H
#define AUTOBOOT_H

#include <QFile>

#include "sioworker.h"

class AtariExeChunk
{
public:
    int address;
    QByteArray data;
};

class AutoBoot : public SioDevice
{
    Q_OBJECT

private:
    QByteArray bootSectors;
    QList <AtariExeChunk> chunks;
    int sectorCount;
    SioDevice *oldDevice;
    bool started, loaded;
    bool readExecutable(const QString &fileName);

public:
    AutoBoot(SioWorkerPtr worker, SioDevice *aOldDevice): SioDevice(worker) {oldDevice = aOldDevice; started = loaded = false;}
    ~AutoBoot();
    void handleCommand(quint8 command, quint16 aux);
    void passToOldHandler(quint8 command, quint16 aux);
    bool open(const QString &fileName, bool highSpeed);
    void close();
    bool readSector(quint16 sector, QByteArray &data);
    QString deviceName();
signals:
    void booterStarted();
    void booterLoaded();
    void blockRead(int current, int all);
    void loaderDone();
};

#endif // AUTOBOOT_H
