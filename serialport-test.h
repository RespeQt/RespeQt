#ifndef SERIALPORTTEST_H
#define SERIALPORTTEST_H

#ifdef QT_NO_DEBUG
#error "This class is not allowed in non-debug builds"
#endif

#include "serialport.h"

#include <QXmlStreamReader>

class TestSerialPortBackend : public AbstractSerialPortBackend
{
    Q_OBJECT
public:
    TestSerialPortBackend(QObject *parent = 0);
    virtual ~TestSerialPortBackend();

    virtual bool open();
    virtual bool isOpen();
    virtual void close();
    virtual void cancel();
    virtual int speedByte();
    virtual QByteArray readCommandFrame();
    virtual QByteArray readDataFrame(uint size, bool verbose = true);
    virtual bool writeDataFrame(const QByteArray &data);
    virtual bool writeCommandAck();
    virtual bool writeCommandNak();
    virtual bool writeDataAck();
    virtual bool writeDataNak();
    virtual bool writeComplete();
    virtual bool writeError();
    virtual bool setSpeed(int speed);
    virtual bool writeRawFrame(const QByteArray &data);
    virtual void setActiveSioDevices(const QByteArray &data);

    QString testFilename() { return mTestFilename; }
    void setTestFilename(QString filename) { mTestFilename = filename; }

protected:
    QString mTestFilename;
    QXmlStreamReader *mXmlReader;
};

#endif // SERIALPORTTEST_H
