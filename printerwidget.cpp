#include "printerwidget.h"
#include "ui_printerwidget.h"
#include "respeqtsettings.h"
#include "printers/nativeoutput.h"
#include "printers/nativeprinter.h"
#include "printers/svgoutput.h"
#include "printers/windowoutput.h"

#include <QPrintDialog>
#include <QPrinterInfo>
#include <QStringList>
#include <QMessageBox>

PrinterWidget::PrinterWidget(int printerNum, QWidget *parent)
   : QFrame(parent)
   , ui(new Ui::PrinterWidget)
   , printerNo_(printerNum)
   , mPrinter(NULL)
   , mDevice(NULL)
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
    // TODO Better solution to getting labels?
    std::map<QString, int> list;
    for (int i = 1; i <= Printers::BasePrinter::NUM_KNOWN_PRINTERS; i++) {
        Printers::BasePrinter *printer = Printers::BasePrinter::createPrinter(i, NULL);
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

    ui->outputSelection->addItem("SVG", QVariant(false));
    //ui->outputSelection->addItem("Window", QVariant(false));
    QStringList printers = QPrinterInfo::availablePrinterNames();
    for (QStringList::const_iterator sit = printers.cbegin(); sit != printers.cend(); ++sit)
    {
        ui->outputSelection->addItem(*sit, QVariant(true));
    }

    ui->atariPrinters->setCurrentIndex(-1);
    ui->outputSelection->setCurrentIndex(-1);

    ui->atariPrinters->setEnabled(true);
    ui->outputSelection->setEnabled(false);
    ui->actionDisconnectPrinter->setEnabled(false);
    ui->actionConnectPrinter->setEnabled(false);

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

void PrinterWidget::on_atariPrinters_currentIndexChanged(int index)
{
    if (ui->atariPrinters->currentText() == "")
    {
        return;
    }
    // If we select a new printer, end the printing job of the old printer
    if (mPrinter)
    {
        delete mPrinter;
        mPrinter = NULL;
    }
    if (mSio) {
       // Create a new Atari printer device and install it.
       int typeId = ui->atariPrinters->itemData(index).toInt();
       Printers::BasePrinter *newPrinter = Printers::BasePrinter::createPrinter(typeId, mSio);
       mSio->installDevice(PRINTER_BASE_CDEVIC + printerNo_, newPrinter);
       connect(newPrinter, SIGNAL(statusChanged(int)), this, SLOT(on_sio_statusChanged(int)));
       mPrinter = newPrinter;

       respeqtSettings->setPrinterType(index, mPrinter->typeId());
       ui->outputSelection->setEnabled(true);
    }

}

void PrinterWidget::on_outputSelection_currentIndexChanged(int /*index*/)
{
    if (ui->outputSelection->currentText() == "")
    {
        return;
    }
    if (ui->outputSelection->currentData().toBool()) // true means printer driver
    {
        Printers::NativePrinter *temp = new Printers::NativePrinter();
        if (mDevice)
        {
            delete mDevice;
        }
        mDevice = temp;

        QString printerName = ui->outputSelection->currentText();
        temp->printer()->setPrinterName(printerName);
        QPrintDialog dialog(temp->printer());
        dialog.setOption(QAbstractPrintDialog::PrintSelection, false);
        dialog.setOption(QAbstractPrintDialog::PrintPageRange, false);
        dialog.setOption(QAbstractPrintDialog::PrintCurrentPage, false);
        if (dialog.exec() == QDialog::Accepted)
        {
            temp->printer()->setPrinterName(printerName);
            respeqtSettings->setConnectedPrinterName(printerNo_, printerName);
        }
    } else if (mInitialized) {
        // Handling the special devices.
        if (ui->outputSelection->currentText() == "SVG")
        {
            Printers::SVGOutput *svg = new Printers::SVGOutput();
            mDevice = svg;
            QString fileName = QFileDialog::getSaveFileName(this, tr("Save SVG"), "", tr("SVG (*.svg)"));
            svg->setFileName(fileName);
        } else if (ui->outputSelection->currentText() == "Window")
        {
            Printers::WindowOutput *window = new Printers::WindowOutput();
            mDevice = window;
            window->show();
        } else {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::information(this, tr("Output device"), tr("Not implemented yet"));
        }
    }
    if (ui->atariPrinters->currentIndex() >= 0
        && ui->outputSelection->currentIndex() >= 0
        && !ui->actionDisconnectPrinter->isEnabled())
    {
        ui->actionConnectPrinter->setEnabled(true);
        ui->atariPrinters->setEnabled(false);
    }
}

void PrinterWidget::on_actionConnectPrinter_triggered()
{
    if (ui->outputSelection->currentIndex() >= 0
            && ui->atariPrinters->currentIndex() >= 0
            && mPrinter && mDevice)
    {
        mPrinter->setOutput(mDevice);
        mPrinter->output()->beginOutput();
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
        mPrinter->setOutput(NULL);
    }
    ui->outputSelection->setEnabled(true);
    ui->atariPrinters->setEnabled(true);
    ui->actionDisconnectPrinter->setEnabled(false);
    ui->actionConnectPrinter->setEnabled(true);
}
