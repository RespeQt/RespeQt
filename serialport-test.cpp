#include "serialport-test.h"

#include <QFile>
#include <exception>
#include <QDir>
#include "logdisplaydialog.h"

TestSerialPortBackend::TestSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent),
      mXmlReader(NULL)
{
    mTestFilename = QString("/Users/jochen/work/RespeQt/test.xml");
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

QByteArray TestSerialPortBackend::readCommandFrame()
{
    if (!mXmlReader) return NULL;

    QByteArray data;

    data.resize(4);
    do {
    } while (mXmlReader->readNext() != QXmlStreamReader::StartElement);

    if (mXmlReader->name() != "commandframe")
        return NULL;

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

    do {
    } while (mXmlReader->readNext() != QXmlStreamReader::StartElement);

    if (mXmlReader->name() != "dataframe")
        return NULL;

    QByteArray data = mXmlReader->readElementText().toLatin1();
    if (verbose)
    {
        qDebug() << "!n" << tr("Data frame contents: %1").arg(QString(data));
        if (size != uint(data.size()))
        {
            qDebug() << "!n" << tr("Read: got %1 bytes, expected %2 bytes")
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
