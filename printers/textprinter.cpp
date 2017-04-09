#include "textprinter.h"
#include "mainwindow.h"

#include <QtDebug>

TextPrinter::TextPrinter(SioWorker *worker)
    : BasePrinter(worker)
{
    mTypeId = TEXTPRINTER;
    mTypeName = QString("Text printer");

    if (MainWindow::getInstance() && MainWindow::getInstance()->getTextPrinterWindow())
    {
        connect(this, SIGNAL(print(QString)), MainWindow::getInstance()->getTextPrinterWindow(), SLOT(print(QString)));
    }
}

TextPrinter::~TextPrinter()
{
    disconnect(this, SIGNAL(print(QString)), MainWindow::getInstance()->getTextPrinterWindow(), SLOT(print(QString)));
}

bool TextPrinter::conversionMsgdisplayedOnce = false;
bool TextPrinter::handleBuffer(QByteArray &buffer, int len)
{
    if (!conversionMsgdisplayedOnce) {
        qDebug() << "!n" << tr("[%1] Converting Inverse Video Characters for ASCII viewing").arg(deviceName()).arg(len);
        conversionMsgdisplayedOnce = true;
    }

    int n = buffer.indexOf('\x9b');
    if (n == -1) {
        n = len;
    }
    buffer.resize(n);
    buffer.replace('\n', '\x9b');
    if (n < len) {
        buffer.append("\n");
    }
    emit print(QString::fromLatin1(buffer));
    sio->port()->writeComplete();
    return true;
}
