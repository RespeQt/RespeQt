#include "printerwidget.h"
#include "ui_printerwidget.h"

#include <QPrintDialog>

PrinterWidget::PrinterWidget(int printerNum, QWidget *parent)
   : QFrame(parent)
   , ui(new Ui::PrinterWidget)
   , printerNo_(printerNum)
   , mPrinter(NULL)
   , mSio(NULL)
{
    ui->setupUi(this);
}

PrinterWidget::~PrinterWidget()
{
    delete ui;
}

void PrinterWidget::setup()
{
    QString printerTxt = QString("P%1").arg(printerNo_ + 1);
    ui->printerLabel->setText(printerTxt);

    ui->atariPrinters->clear();
    ui->atariPrinters->addItem("Text printer");
    ui->atariPrinters->addItem("Atari 1027");
    ui->atariPrinters->setEnabled(false);

    ui->actionConnectPrinter->setEnabled(false);
    ui->actionDisconnectPrinter->setEnabled(false);

    // Connect widget actions to buttons
    ui->buttonConnectPrinter->setDefaultAction(ui->actionConnectPrinter);
    ui->buttonDisconnectPrinter->setDefaultAction(ui->actionDisconnectPrinter);
}

void PrinterWidget::setSioWorker(SioWorker *sio)
{
    mSio = sio;
    ui->atariPrinters->setEnabled(true);
    on_atariPrinters_currentIndexChanged(0);
}

void PrinterWidget::on_atariPrinters_currentIndexChanged(int index)
{
    // If we select a new printer, end the printing job of the old printer
    if (mPrinter && mPrinter->requiresNativePrinter())
    {
        mPrinter->endPrint();
    }
    if (mSio) {
       // Create a new Atari printer device and install it.
       BasePrinter *newPrinter = BasePrinter::createPrinter(index + 1, mSio);
       mSio->installDevice(PRINTER_BASE_CDEVIC + printerNo_, newPrinter);
       mPrinter = newPrinter;

       // Setup buttons
       ui->actionConnectPrinter->setEnabled(mPrinter->requiresNativePrinter());
       ui->actionDisconnectPrinter->setEnabled(false);
    }
}

void PrinterWidget::on_buttonConnectPrinter_triggered(QAction * /*arg1*/)
{
    QPrintDialog dialog(mPrinter->nativePrinter());
    if (dialog.exec() == QDialog::Accepted)
    {
        ui->actionDisconnectPrinter->setEnabled(true);
        ui->actionConnectPrinter->setEnabled(false);
        mPrinter->beginPrint();
    }
}

void PrinterWidget::on_buttonDisconnectPrinter_triggered(QAction * /*arg1*/)
{
    mPrinter->endPrint();
    ui->actionConnectPrinter->setEnabled(mPrinter->requiresNativePrinter());
    ui->actionDisconnectPrinter->setEnabled(false);
}
