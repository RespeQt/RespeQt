#include "printerwidget.h"
#include "ui_printerwidget.h"
#include "respeqtsettings.h"
#include "printers/nativeprintersupport.h"

#include <QPrintDialog>

PrinterWidget::PrinterWidget(int printerNum, QWidget *parent)
   : QFrame(parent)
   , ui(new Ui::PrinterWidget)
   , printerNo_(printerNum)
   , mPrinter(NULL)
   , mSio(NULL)
   , mInitialized(false)
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
    std::map<QString, int> list;
    for (int i = 1; i <= BasePrinter::NUM_KNOWN_PRINTERS; i++) {
        BasePrinter *printer = BasePrinter::createPrinter(i, NULL);
        list[printer->typeName()] = printer->typeId();
        delete printer;
    }
    std::map<QString, int>::const_iterator cit;
    for(cit = list.cbegin(); cit != list.cend(); ++cit)
    {
        ui->atariPrinters->addItem(cit->first, QVariant(cit->second));
        if (cit->second == 1)
        {
            ui->atariPrinters->setCurrentText(cit->first);
        }
    }

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
    if (!mInitialized)
    {
        ui->atariPrinters->setEnabled(true);
        int count = ui->atariPrinters->count();
        for(int i = 0; i < count; i++)
        {
            if (ui->atariPrinters->itemData(i).toInt() == BasePrinter::TEXTPRINTER)
            {
                ui->atariPrinters->setCurrentIndex(i);
                on_atariPrinters_currentIndexChanged(i);
                break;
            }
        }
        mInitialized = true;
    }
}

void PrinterWidget::on_atariPrinters_currentIndexChanged(int index)
{
    // If we select a new printer, end the printing job of the old printer
    if (mPrinter && mPrinter->requiresNativePrinter())
    {
        NativePrinterSupport *nativePrinter = dynamic_cast<NativePrinterSupport*>(mPrinter);
        nativePrinter->endPrint();
    }
    if (mSio) {
       // Create a new Atari printer device and install it.
       int typeId = ui->atariPrinters->itemData(index).toInt();
       BasePrinter *newPrinter = BasePrinter::createPrinter(typeId, mSio);
       mSio->installDevice(PRINTER_BASE_CDEVIC + printerNo_, newPrinter);
       mPrinter = newPrinter;

       // Setup buttons
       ui->actionConnectPrinter->setEnabled(mPrinter->requiresNativePrinter());
       ui->actionDisconnectPrinter->setEnabled(false);
       if (mPrinter && !mPrinter->requiresNativePrinter())
       {
           respeqtSettings->setConnectedPrinterName(printerNo_, QString());
       }
    }
    if (mPrinter)
    {
        respeqtSettings->setPrinterType(index, mPrinter->typeId());
    }
}

void PrinterWidget::on_buttonConnectPrinter_triggered(QAction * /*arg1*/)
{
    if (mPrinter && mPrinter->requiresNativePrinter())
    {
        NativePrinterSupport *nativePrinter = dynamic_cast<NativePrinterSupport*>(mPrinter);
        QPrintDialog dialog(nativePrinter->nativePrinter());
        if (dialog.exec() == QDialog::Accepted)
        {
            ui->actionDisconnectPrinter->setEnabled(true);
            ui->actionConnectPrinter->setEnabled(false);
            ui->atariPrinters->setEnabled(false);
            nativePrinter->beginPrint();
            respeqtSettings->setConnectedPrinterName(printerNo_, nativePrinter->nativePrinter()->printerName());
        }
    }
}

void PrinterWidget::on_buttonDisconnectPrinter_triggered(QAction * /*arg1*/)
{
    if (mPrinter && mPrinter->requiresNativePrinter())
    {
        NativePrinterSupport *nativePrinter = dynamic_cast<NativePrinterSupport*>(mPrinter);
        nativePrinter->endPrint();
        ui->actionConnectPrinter->setEnabled(nativePrinter->requiresNativePrinter());
        ui->actionDisconnectPrinter->setEnabled(false);
        ui->atariPrinters->setEnabled(true);
    }
}
