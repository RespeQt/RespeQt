#ifndef QT_NO_DEBUG
#include "serialport-test.h"
#include "respeqtsettings.h"
#include "logdisplaydialog.h"

#include <QFile>
#include <exception>
#include <QDir>
#include <QThread>
#include <QRegularExpressionMatchIterator>
#include <QRegularExpressionMatch>

TestSerialPortBackend::TestSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent),
      mXmlReader(nullptr),
      mRegexp("&#(x?)(\\d{0,3});", QRegularExpression::CaseInsensitiveOption)
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
    mXmlReader = nullptr;
}

int TestSerialPortBackend::speedByte()
{
    return 0x28; // standard speed (19200)
}

int TestSerialPortBackend::speed()
{
    return 19200;
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
            if (ok)
                QThread::currentThread()->sleep(static_cast<unsigned long>(sec));
        }
        if (attr.hasAttribute("msec")) {
            int msec = attr.value("msec").toInt(&ok);
            qDebug() << "!n" << tr("Sleeping %1 milliseconds").arg(msec);
            if (ok)
                QThread::currentThread()->msleep(static_cast<unsigned long>(msec));
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
    if (!mXmlReader)
        return nullptr;

    QByteArray data;

    data.resize(4);

    forwardXml();
    if (!readPauseTag())
        return nullptr;

    if (!mXmlReader->isStartElement()
            && !mXmlReader->readNextStartElement())
        return nullptr;
    if (mXmlReader->name() != "commandframe")
        return nullptr;

    QXmlStreamAttributes attr = mXmlReader->attributes();
    if (!attr.hasAttribute("device")
        || !attr.hasAttribute("command")
        || !attr.hasAttribute("aux1")
        || !attr.hasAttribute("aux2"))
        return nullptr;

    bool ok;
    data[0] = static_cast<char>(attr.value("device").toInt(&ok, 10));
    if (!ok)
       return nullptr;

    data[1] = static_cast<char>(attr.value("command").toInt(&ok, 10));
    if (!ok)
        return nullptr;

    data[2] = static_cast<char>(attr.value("aux1").toInt(&ok, 10));
    if (!ok)
        return nullptr;

    data[3] = static_cast<char>(attr.value("aux2").toInt(&ok, 10));
    if (!ok)
        return nullptr;


    return data;
}

QByteArray TestSerialPortBackend::readDataFrame(uint size, bool verbose)
{
    if (!mXmlReader)
        return nullptr;

    forwardXml();
    if (!readPauseTag())
        return nullptr;

    if (!mXmlReader->isStartElement()
            && !mXmlReader->readNextStartElement())
        return nullptr;

    if (mXmlReader->name() != "dataframe")
        return nullptr;

    QString dataframe = mXmlReader->readElementText(QXmlStreamReader::IncludeChildElements);
    QRegularExpressionMatchIterator i = mRegexp.globalMatch(dataframe);
    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        QString hex = match.captured(1);
        QString number = match.captured(2);
        bool ok;
        int code = number.toInt(&ok, hex.count() == 0 ? 10 : 16);
        dataframe.replace(match.captured(0), QChar(code));
    }
    QByteArray data = dataframe.toLatin1();
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
