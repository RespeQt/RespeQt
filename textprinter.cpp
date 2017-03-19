#include "textprinter.h"
#include "respeqtsettings.h"

#include <QtDebug>

TextPrinter::TextPrinter(SioWorker *worker)
    : BasePrinter(worker)
{
    mTypeId = 1;
    mTypeName = new QString("Text");

    connect(this, SIGNAL(print(QString)), MainWindow::getInstance()->getTextPrinterWindow(), SLOT(print(QString)));
}

bool TextPrinter::conversionMsgdisplayedOnce = false;
void TextPrinter::handleCommand(quint8 command, quint16 aux)
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
                // Display Info message once  //
                if (!conversionMsgdisplayedOnce) {
                    qDebug() << "!n" << tr("[%1] Converting Inverse Video Characters for ASCII viewing").arg(deviceName()).arg(len);
                    conversionMsgdisplayedOnce = true;
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
                int n = data.indexOf('\x9b');
                if (n == -1) {
                    n = len;
                }
                data.resize(n);
                data.replace('\n', '\x9b');
                if (n < len) {
                    data.append("\n");
                }
                emit print(QString::fromLatin1(data));
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
