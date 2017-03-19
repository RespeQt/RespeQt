#include "atari1027.h"
#include "respeqtsettings.h"

#include <QtDebug>

Atari1027::Atari1027(SioWorker *worker)
    : BasePrinter(worker)
{
    mTypeId = 2;
    mTypeName = new QString("Atari 2017");
}

void Atari1027::handleCommand(quint8 command, quint16 aux)
{
    if(respeqtSettings->printerEmulation()) {  // Ignore printer commands  if Emulation turned OFF)    //
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
