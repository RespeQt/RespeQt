/*
 * sioworker.h
 *
 * Copyright 2016 TheMontezuma
 * Copyright 2017 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef SIOWORKER_H
#define SIOWORKER_H

#include <QThread>
#include <QMutex>
#ifndef QT_NO_DEBUG
#include <QFile>
#endif

#include "serialport.h"
#include <memory>

enum SIO_CDEVIC:quint8
{
    DISK_BASE_CDEVIC = 0x31,
    PRINTER_BASE_CDEVIC = 0x40,
    SMART_CDEVIC = 0x45,
    RESPEQT_CLIENT_CDEVIC = 0x46,
    RS232_BASE_CDEVIC = 0x50,
    PCLINK_CDEVIC = 0x6F
};

enum SIO_DEVICE_COUNT:quint8
{
    DISK_COUNT = 15,
    PRINTER_COUNT = 4,
    RS232_COUNT = 4
};

class SioWorker;
using SioWorkerPtr = std::shared_ptr<SioWorker>;

class SioDevice : public QObject
{
    Q_OBJECT

protected:
    int m_deviceNo;
    QMutex mLock;
    SioWorkerPtr sio;
public:
    SioDevice(SioWorkerPtr worker);
    virtual ~SioDevice();
    virtual void handleCommand(quint8 command, quint16 aux) = 0;
    virtual QString deviceName();
    inline void lock() {mLock.lock();}
    inline bool tryLock() {return mLock.tryLock();}
    inline void unlock() {mLock.unlock();}
    inline void setDeviceNo(int no) {emit statusChanged(m_deviceNo); m_deviceNo = no; emit statusChanged(no);}
    inline int deviceNo() const {return m_deviceNo;}
signals:
    void statusChanged(int deviceNo);
};

class SioWorker : public QThread
{
    Q_OBJECT

private:
    quint8 sioChecksum(const QByteArray &data, uint size);
    QMutex *deviceMutex;
    SioDevice* devices[256];
    AbstractSerialPortBackend *mPort;
    bool mustTerminate;
#ifndef QT_NO_DEBUG
    bool mSnapshotRunning;
    QXmlStreamWriter *mSnapshotWriter;
    void writeSnapshotCommandFrame(qint8 no, qint8 command, qint8 aux1, qint8 aux2);
#endif
    bool displayCommandName;

public:
    AbstractSerialPortBackend* port() {return mPort;}
    int maxSpeed;

    SioWorker();
    ~SioWorker();

    bool wait (unsigned long time = ULONG_MAX);

    void run();

    void installDevice(quint8 no, SioDevice *device);
    void uninstallDevice(quint8 no);
    void swapDevices(quint8 d1, quint8 d2);
    SioDevice* getDevice(quint8 no);

    QString guessDiskCommand(quint8 command, quint16 aux);
    QString deviceName(int device);
    void setDisplayCommandName(bool display) {displayCommandName = display;}
    static void usleep(unsigned long time) {QThread::usleep(time);}

#ifndef QT_NO_DEBUG
    void startSIOSnapshot();
    void stopSIOSnapshot();
    bool isSnapshotRunning() { return mSnapshotRunning; }
    void writeSnapshotDataFrame(QByteArray &data);
#endif

signals:
    void statusChanged(QString status);
public slots:
    void start(Priority p = InheritPriority);
};

class CassetteRecord {
public:
    int baudRate;
    int gapDuration;
    int totalDuration;
    QByteArray data;
};

class CassetteWorker : public QThread
{
    Q_OBJECT

private:
    QMutex mustTerminate;
    AbstractSerialPortBackend *mPort;
    QList<CassetteRecord> mRecords;
public:
    AbstractSerialPortBackend* port() {return mPort;}

    CassetteWorker();
    ~CassetteWorker();

    bool loadCasImage(const QString &fileName);

    bool wait (unsigned long time = ULONG_MAX);

    void run();

    int mTotalDuration;
signals:
    void statusChanged(int remainingTime);
public slots:
    void start(Priority p = InheritPriority);
};

#endif // SIOWORKER_H
