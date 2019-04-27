#include "printerwidget.h"
#include "ui_printerwidget.h"
#include "respeqtsettings.h"
#include "printers/printers.h"
#include "printers/outputs.h"
#include "printers/printerfactory.h"
#include "printers/outputfactory.h"

#include <QVector>
#include <QString>
#include <QMessageBox>

PrinterWidget::PrinterWidget(int printerNum, QWidget *parent)
   : QFrame(parent)
   , ui(new Ui::PrinterWidget)
   , printerNo_(printerNum)
   , mPrinter(Q_NULLPTR)
   , mDevice(Q_NULLPTR)
   , mSio(Q_NULLPTR)
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

    RespeqtSettings::PrinterSettings ps = respeqtSettings->printerSettings(printerNo_);

    ui->atariPrinters->clear();
    std::map<QString, int> list;
    ui->atariPrinters->addItem(tr("None"), -1);
    Printers::PrinterFactory* factory = Printers::PrinterFactory::instance();
    const QVector<QString> pnames = factory->getPrinterNames();
    QVector<QString>::const_iterator it;
    for(it = pnames.begin(); it != pnames.end(); ++it)
    {
        ui->atariPrinters->addItem(*it);
    }

    // Set to default (none) and then look whether we have a settings
    ui->atariPrinters->setCurrentIndex(0);
    if (ps.printerName != "")
    {
        int index = ui->atariPrinters->findText(ps.printerName);
        if (index != -1)
        {
            ui->atariPrinters->setCurrentIndex(index);
        }
    }

    ui->outputSelection->clear();
    ui->outputSelection->addItem(tr("None"), -1);
    const QVector<QString> onames = Printers::OutputFactory::instance()->getOutputNames();
    for (it = onames.begin(); it != onames.end(); ++it)
    {
        ui->outputSelection->addItem(*it);
    }

    // Set to default (none) and then look whether we have a settings
    ui->outputSelection->setCurrentIndex(0);
    if (ps.outputName != "")
    {
        int index = ui->outputSelection->findText(ps.outputName);
        if (index != -1)
        {
            ui->outputSelection->setCurrentIndex(index);
        }
    }

    ui->atariPrinters->setEnabled(true);
    ui->outputSelection->setEnabled(true);
    ui->actionDisconnectPrinter->setEnabled(false);
    ui->actionConnectPrinter->setEnabled(true);

    // Connect widget actions to buttons
    ui->buttonDisconnectPrinter->setDefaultAction(ui->actionDisconnectPrinter);
    ui->buttonConnectPrinter->setDefaultAction(ui->actionConnectPrinter);
}

void PrinterWidget::setSioWorker(SioWorker *sio)
{
    mSio = sio;
    if (!mInitialized)
    {
        mInitialized = true;
    }
}

bool PrinterWidget::selectPrinter()
{
    if (ui->atariPrinters->currentText() == tr("None"))
    {
        return false;
    }
    // If we select a new printer, end the printing job of the old printer
    if (mPrinter)
    {
        delete mPrinter;
        mPrinter = Q_NULLPTR;
    }
    if (mSio) {
       Printers::PrinterFactory* factory = Printers::PrinterFactory::instance();
       Printers::BasePrinter *newPrinter = factory->createPrinter(ui->atariPrinters->currentText(), mSio);
       if (newPrinter != Q_NULLPTR)
       {
           mSio->installDevice(static_cast<quint8>(PRINTER_BASE_CDEVIC + printerNo_), newPrinter);
           mPrinter = newPrinter;
           respeqtSettings->setPrinterName(printerNo_, ui->atariPrinters->currentText());
           return true;
       }
    }
    return false;
}

bool PrinterWidget::selectOutput()
{
    if (ui->outputSelection->currentText() == tr("None"))
    {
        return false;
    }
    if (mDevice != Q_NULLPTR)
    {
        delete mDevice;
        mDevice = Q_NULLPTR;
    }

    QString label = ui->outputSelection->currentText();
    Printers::NativeOutput* output = Printers::OutputFactory::instance()->createOutput(label);
    // Try to cast to NativePrinter, if successful set printer name;
    Printers::NativePrinter *printoutput = dynamic_cast<Printers::NativePrinter*>(output);
    if (printoutput != Q_NULLPTR)
        printoutput->printer()->setPrinterName(label);

    if (output != Q_NULLPTR)
    {
        if (output->setupOutput())
        {
            mDevice = output;
            respeqtSettings->setOutputName(printerNo_, label);
        }

        return true;
    }
    return false;
}

void PrinterWidget::on_actionConnectPrinter_triggered()
{
    if (ui->outputSelection->currentIndex() == 0
            || ui->atariPrinters->currentIndex() == 0)
    {
        QMessageBox::warning(this, tr("Printers"), tr("Please select an output device as well as a printer emulation."));
        return;
    }

    if (!selectPrinter() || !selectOutput())
    {
         on_actionDisconnectPrinter_triggered();
         return;
    }

    if (mPrinter != Q_NULLPTR && mDevice != Q_NULLPTR)
    {
        try {
            Printers::Passthrough *ptemp = dynamic_cast<Printers::Passthrough*>(mPrinter);
            Printers::RawOutput *otemp = dynamic_cast<Printers::RawOutput*>(mDevice);
            if (ptemp == Q_NULLPTR || otemp == Q_NULLPTR)
            {
                QMessageBox::critical(this, tr("Printer emulation"), tr("You are not allowed to use the passthrough emulation without an raw output."));
                on_actionDisconnectPrinter_triggered();
                return;
            }
        } catch(...)
        {}

        mPrinter->setOutput(mDevice);
        if (!mPrinter->output()->beginOutput())
        {
            QMessageBox::critical(this, tr("Beginning output"), tr("The output device couldn't start."));
            return;
        }
        ui->outputSelection->setEnabled(false);
        ui->atariPrinters->setEnabled(false);
        ui->actionDisconnectPrinter->setEnabled(true);
        ui->actionConnectPrinter->setEnabled(false);
    }
}

void PrinterWidget::on_actionDisconnectPrinter_triggered()
{
    if (mPrinter)
    {
        // mPrinter->setOutput delete the output device,
        // so we don't need an explicit delete
        mPrinter->setOutput(Q_NULLPTR);
    } else {
        // Here we do need a delete
        delete mDevice;
    }
    mDevice = Q_NULLPTR;

    ui->outputSelection->setEnabled(true);
    ui->atariPrinters->setEnabled(true);
    ui->actionDisconnectPrinter->setEnabled(false);
    ui->actionConnectPrinter->setEnabled(true);
}

void PrinterWidget::on_outputSelection_currentIndexChanged(const QString &outputName)
{
    respeqtSettings->setOutputName(printerNo_, outputName);
}

void PrinterWidget::on_atariPrinters_currentIndexChanged(const QString &printerName)
{
    respeqtSettings->setPrinterName(printerNo_, printerName);
}
