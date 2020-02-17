/*
 * rcl.h
 *
 * Copyright 2015, 2017 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef RCL_H
#define RCL_H

#include "sioworker.h"

// RespeQt Time Server //
class RCl: public SioDevice
{
    Q_OBJECT

public:
    RCl(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
    QString toAtariFileName(QString dosFileName);
    QString toDosFileName(QString atariFileName);

public slots:
    void gotNewSlot (int slot);                         // 
    void fileMounted (bool mounted);                    // 

signals:
    void findNewSlot (int startFrom, bool createOne);
    void mountFile (int no, const QString fileName);
    void toggleAutoCommit (int no, bool st);

private:
    QString imageFileName;
    static char g_rclSlotNo;
};

#endif // RCL_H
