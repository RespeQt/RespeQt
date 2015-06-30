#ifndef AUTOBOOTDIALOG_H
#define AUTOBOOTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
    class AutoBootDialog;
}

class AutoBootDialog : public QDialog {
    Q_OBJECT

public:
    AutoBootDialog(QWidget *parent = 0);
    ~AutoBootDialog();

signals:

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    Ui::AutoBootDialog *ui;

private slots:
    void booterStarted();
    void booterLoaded();
    void blockRead(int current, int all);
    void loaderDone();
    void onClick(QAbstractButton* button);
    void reloadExe();
};

#endif // AUTOBOOTDIALOG_H
