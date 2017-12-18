#include "baseprinter.h"
#include "textprinter.h"
#include "atari1027.h"
#include "atari1020.h"
#include "escp.h"
#include "respeqtsettings.h"
#include "logdisplaydialog.h"

namespace Printers
{
    BasePrinter::BasePrinter(SioWorker *worker)
        : SioDevice(worker),
          mOutput(NULL)
    {}

    BasePrinter::~BasePrinter()
    {

    }

    const QChar BasePrinter::translateAtascii(const char b)
    {
        return mAtascii(b);
    }

    BasePrinter *BasePrinter::createPrinter(int type, SioWorker *worker)
    {
        switch (type)
        {
            case TEXTPRINTER:
                return new TextPrinter(worker);
            case ATARI1027:
                return new Atari1027(worker);
            case ATARI1020:
                return new Atari1020(worker);
            case ESCP:
                return new Escp(worker);
            default:
                throw new std::invalid_argument("Unknown printer type");
        }
        return NULL;
    }


    void BasePrinter::handleCommand(quint8 command, quint16 aux)
    {
        if (respeqtSettings->printerEmulation() && mOutput) {  // Ignore printer commands  if Emulation turned OFF)    //
            qDebug() << "!n" << "[" << deviceName() << "] "
                     << hex << "command: " << command << " aux: " << aux;
            switch(command) {
            case 0x53:
                {
                    // Get status
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }

                    QByteArray status(4, 0);
                    status[0] = 0;
                    status[1] = m_lastOperation;
                    status[2] = 3;
                    status[3] = 0;
                    sio->port()->writeComplete();
                    sio->port()->writeDataFrame(status);
                    qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
                    break;
                }
            case 0x57:
                {
                    // Write
                    int aux2 = aux % 256;

                    int len;
                    switch (aux2) {
                    case 0x4e:
                        // Normal
                        len = 40;
                        break;
                    case 0x53:
                        // Sideways
                        len = 29;
                        break;
                    case 0x44:
                        // Double width
                        len = 21;
                        break;
                    default:
                        sio->port()->writeCommandNak();
                        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                                       .arg(deviceName())
                                       .arg(command, 2, 16, QChar('0'))
                                       .arg(aux, 4, 16, QChar('0'));
                        return;
                    }
                    sio->port()->writeCommandAck();

                    QByteArray data = sio->port()->readDataFrame(len);
                    if (data.isEmpty()) {
                        qCritical() << "!e" << tr("[%1] Print: data frame failed")
                                      .arg(deviceName());
                        sio->port()->writeDataNak();
                        return;
                    }
#ifndef QT_NO_DEBUG
                    sio->writeSnapshotDataFrame(data);
#endif

                    handleBuffer(data, len);
                    sio->port()->writeDataAck();
                    qDebug() << "!n" << tr("[%1] Print (%2 chars)").arg(deviceName()).arg(len);
                    sio->port()->writeComplete();
                    break;
                }
            default:
                sio->port()->writeCommandNak();
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                               .arg(deviceName())
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
            }
        } else {
            qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
        }
    }

    void BasePrinter::setOutput(NativeOutput *output)
    {
        if (mOutput != output)
        {
            if (mOutput)
            {
                mOutput->endOutput();
            }
            delete mOutput;
        }
        mOutput = output;
        setupOutput();
        setupFont();
    }

    void BasePrinter::setupOutput()
    {}
}
