#ifndef LOGDISPLAYDIALOG_H
#define LOGDISPLAYDIALOG_H

#include <QtWidgets/QDialog>
#include <QDebug>
#include <QAbstractButton>
#include <QTextEdit>

namespace Ui {
    class LogDisplayDialog;
}

class LogDisplayDialog : public QDialog {
    Q_OBJECT

public:
    LogDisplayDialog(QWidget *parent = 0);
    ~LogDisplayDialog();

public slots:
    void getLogText(QString logText);
    void getLogTextChange(QString logChange);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    Ui::LogDisplayDialog *l_ui;

private slots:
    void diskFilter();
    void onClick(QAbstractButton* button);

signals:
};
#endif // LOGDISPLAYDIALOG_H
