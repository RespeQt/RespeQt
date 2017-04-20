#ifndef QT_NO_DEBUG
#include "serialport-test.h"
#include "respeqtsettings.h"
#include "logdisplaydialog.h"

#include <QFile>
#include <exception>
#include <QDir>
#include <QThread>

TestSerialPortBackend::TestSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent),
      mXmlReader(NULL)
{
    mTestFilename = respeqtSettings->testFile();
}

TestSerialPortBackend::~TestSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

bool TestSerialPortBackend::open()
{
    if (isOpen()) {
        close();
    }
    // Open XML and parse it
    if (mTestFilename.size() == 0) return false;

    QFile *file = new QFile(mTestFilename);
    if (!file->exists()) return false;
    file->open(QIODevice::Text|QFile::ReadOnly);

    mXmlReader = new QXmlStreamReader(file);

    if (mXmlReader->readNext() != QXmlStreamReader::StartDocument) {
        return false;
    }

    if (mXmlReader->readNext() != QXmlStreamReader::StartElement) {
        return false;
    }

    if (mXmlReader->name() != "testcase")
        return false;

    return true;
}

bool TestSerialPortBackend::isOpen()
{
    if (!mXmlReader) return false;
    if (!mXmlReader->device()) return false;
    return mXmlReader->device()->isOpen();
}

void TestSerialPortBackend::close()
{
    cancel();
}

void TestSerialPortBackend::cancel()
{
    if (mXmlReader)
    {
        mXmlReader->device()->close();
    }
    delete mXmlReader;
    mXmlReader = NULL;
}

int TestSerialPortBackend::speedByte()
{
    return 0x28; // standard speed (19200)
}

bool TestSerialPortBackend::setSpeed(int /*speed*/)
{
    return true;
}

bool TestSerialPortBackend::readPauseTag()
{
     if (!mXmlReader->isStartElement()
             && !mXmlReader->readNextStartElement())
         return false;

     if (mXmlReader->name() == "pause")
     {
        QXmlStreamAttributes attr = mXmlReader->attributes();
        bool ok;
        if (attr.hasAttribute("sec")) {
            int sec = attr.value("sec").toInt(&ok);
            qDebug() << "!n" << tr("Sleeping %1 seconds").arg(sec);
            if (ok) QThread::currentThread()->sleep(sec);
        }
        if (attr.hasAttribute("msec")) {
            int msec = attr.value("msec").toInt(&ok);
            qDebug() << "!n" << tr("Sleeping %1 milliseconds").arg(msec);
            if (ok) QThread::currentThread()->msleep(msec);
        }
        forwardXml();
    }
    return true;
}

inline
void TestSerialPortBackend::forwardXml()
{
    QXmlStreamReader::TokenType token;
    do {
        token = mXmlReader->readNext();
        /*qDebug()<<"!n is start: "<<mXmlReader->isStartElement()
               << " is end: "<<mXmlReader->isEndElement()
               <<" Name: "<<mXmlReader->name();*/
    } while(!mXmlReader->atEnd()
          && token != QXmlStreamReader::EndElement
          && token != QXmlStreamReader::StartElement);
}

QByteArray TestSerialPortBackend::readCommandFrame()
{
    if (!mXmlReader) return NULL;

    QByteArray data;

    data.resize(4);

    forwardXml();
    if (!readPauseTag()) return NULL;

    if (!mXmlReader->isStartElement()
            && !mXmlReader->readNextStartElement()) return NULL;
    if (mXmlReader->name() != "commandframe") return NULL;

    QXmlStreamAttributes attr = mXmlReader->attributes();
    if (!attr.hasAttribute("device")
        || !attr.hasAttribute("command")
        || !attr.hasAttribute("aux1")
        || !attr.hasAttribute("aux2"))
        return NULL;

    bool ok;
    data[0] = attr.value("device").toInt(&ok, 16);
    if (!ok)
    {
        data[0] = attr.value("device").toInt(&ok, 10);
        if (!ok) return NULL;
    }
    data[1] = attr.value("command").toInt(&ok, 16);
    if (!ok)
    {
        data[1] = attr.value("command").toInt(&ok, 10);
        if (!ok) return NULL;
    }
    data[2] = attr.value("aux1").toInt(&ok, 16);
    if (!ok)
    {
        data[2] = attr.value("aux1").toInt(&ok, 10);
        if (!ok) return NULL;
    }
    data[3] = attr.value("aux2").toInt(&ok, 16);
    if (!ok)
    {
        data[3] = attr.value("aux2").toInt(&ok, 10);
        if (!ok) return NULL;
    }

    return data;
}

QByteArray TestSerialPortBackend::readDataFrame(uint size, bool verbose)
{
    if (!mXmlReader) return NULL;

    forwardXml();
    if (!readPauseTag()) return NULL;

    if (!mXmlReader->isStartElement()
            && !mXmlReader->readNextStartElement())
        return NULL;

    if (mXmlReader->name() != "dataframe")
        return NULL;

    QByteArray data = mXmlReader->readElementText().toLatin1();
    if (verbose)
    {
        qDebug() << "!n" << tr("Data frame contents: %1").arg(QString(data));
        if (size != uint(data.size()))
        {
            qDebug() << "!n" << tr("Read error: got %1 bytes, expected %2 bytes")
                      .arg(data.size()).arg(size);
        }
    }

    return data;
}

bool TestSerialPortBackend::writeDataFrame(const QByteArray &/*data*/)
{
    return true;
}

bool TestSerialPortBackend::writeCommandAck()
{
    return true;
}

bool TestSerialPortBackend::writeCommandNak()
{
    return true;
}

bool TestSerialPortBackend::writeDataAck()
{
    return true;
}

bool TestSerialPortBackend::writeDataNak()
{
    return true;
}

bool TestSerialPortBackend::writeComplete()
{
    return true;
}

bool TestSerialPortBackend::writeError()
{
    return true;
}

bool TestSerialPortBackend::writeRawFrame(const QByteArray &/*data*/)
{
    return true;
}

void TestSerialPortBackend::setActiveSioDevices(const QByteArray &/*data*/)
{}
#endif
