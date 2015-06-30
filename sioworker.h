#ifndef SIOWORKER_H
#define SIOWORKER_H

#include <QThread>
#include <QMutex>

#include "serialport.h"

class SioWorker;

class SioDevice : public QObject
{
    Q_OBJECT

protected:
    int m_deviceNo;
    QMutex mLock;
    SioWorker *sio;
public:
    SioDevice(SioWorker *worker);
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

    QString deviceName(int device);
    static void usleep(unsigned long time) {QThread::usleep(time);}
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
