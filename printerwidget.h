#ifndef PRINTERWIDGET_H
#define PRINTERWIDGET_H

#include "printers/baseprinter.h"
#include "sioworker.h"
#include "printers/nativeoutput.h"

#include <QFrame>

namespace Ui {
    class PrinterWidget;
}

class PrinterWidget : public QFrame
{
    Q_OBJECT

public:
    explicit PrinterWidget(int printerNum, QWidget *parent = 0);
    ~PrinterWidget();

    int getPrinterNumber() const { return printerNo_; }
    void setup();

    BasePrinter *printer() const { return mPrinter; }
    void setPrinter(BasePrinter *printer) { mPrinter = printer; }

    void setSioWorker(SioWorker *sio);

signals:
    void actionEject(int deviceId);
    void actionConnectPrinter(int deviceId);

private slots:
    void on_atariPrinters_currentIndexChanged(int index);
    void on_outputSelection_currentIndexChanged(int index);
    void on_actionConnectPrinter_triggered();
    void on_actionDisconnectPrinter_triggered();

private:
    Ui::PrinterWidget *ui;
    int printerNo_;
    BasePrinter *mPrinter;
    NativeOutput *mDevice;
    SioWorker *mSio;
    bool mInitialized;
};

#endif // PRINTERWIDGET_H
