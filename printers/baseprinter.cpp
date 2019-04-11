#include "baseprinter.h"
#include "atari1027.h"
#include "atari1020.h"
#include "atari1029.h"
#include "escp.h"
#include "respeqtsettings.h"
#include "logdisplaydialog.h"

namespace Printers
{
    BasePrinter::BasePrinter(SioWorker *worker)
        : SioDevice(worker),
          mOutput(Q_NULLPTR)
    {}

    BasePrinter::~BasePrinter()
    {}

    const QChar BasePrinter::translateAtascii(const unsigned char b)
    {
        return mAtascii(b);
    }

    void BasePrinter::handleCommand(quint8 command, quint16 aux)
    {
        if (respeqtSettings->printerEmulation() && mOutput) {  // Ignore printer commands  if Emulation turned OFF)    //
            qDebug() << "!n" << "[" << deviceName() << "] "
                     << hex << "command: " << command << " aux: " << aux;
            switch(command) {
            case 0x53: // dec 83
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
            case 0x57: // dec 87
                {
                    // Write
                    int aux2 = aux % 256;

                    unsigned int len;
                    switch (aux2) {
                    case 0x4e: // dec 78
                        // Normal
                        len = 40;
                        break;
                    case 0x53: // dec 83
                        // Sideways
                        len = 29;
                        break;
                    case 0x44: // dec 68
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
