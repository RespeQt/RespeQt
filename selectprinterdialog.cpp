#include "selectprinterdialog.h"
#include "ui_selectprinterdialog.h"

#include <QStringList>
#include <QPrinterInfo>
#include <QListWidgetItem>

SelectPrinterDialog::SelectPrinterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectPrinterDialog),
    mSelectedPrinterName("")
{
    ui->setupUi(this);

    QStringList printerNames = QPrinterInfo::availablePrinterNames();
    int size = printerNames.size();
    for(int i = 0; i < size; i++)
    {
        QListWidgetItem *item = new QListWidgetItem(ui->printerList);
        item->setText(printerNames.at(i));
        ui->printerList->addItem(item);
    }
}

SelectPrinterDialog::~SelectPrinterDialog()
{
    delete ui;
}

void SelectPrinterDialog::on_printerList_itemSelectionChanged()
{
    QList<QListWidgetItem*> items = ui->printerList->selectedItems();
    if (items.size() > 0)
    {
        mSelectedPrinterName = items.at(0)->text();
    } else {
        mSelectedPrinterName = "";
    }
}
