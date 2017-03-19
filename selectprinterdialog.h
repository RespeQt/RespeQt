#ifndef SELECTPRINTERDIALOG_H
#define SELECTPRINTERDIALOG_H

#include <QDialog>

namespace Ui {
class SelectPrinterDialog;
}

class SelectPrinterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectPrinterDialog(QWidget *parent = 0);
    ~SelectPrinterDialog();

    inline const QString &selectedPrinterName() const { return mSelectedPrinterName; }

private slots:
    void on_printerList_itemSelectionChanged();

private:
    Ui::SelectPrinterDialog *ui;
    QString mSelectedPrinterName;
};

#endif // SELECTPRINTERDIALOG_H
