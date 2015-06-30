#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QtWidgets/QDialog>

namespace Ui {
    class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    AboutDialog(QWidget *parent, QString version);
    ~AboutDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::AboutDialog *m_ui;

//private slots:
};

#endif // ABOUTDIALOG_H
