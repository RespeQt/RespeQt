/*
 * bootoptionsdialog.h
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef BOOTOPTIONSDIALOG_H
#define BOOTOPTIONSDIALOG_H

#include <QtWidgets/QDialog>
#include <QDebug>
#include <QAbstractButton>

namespace Ui {
    class BootOptionsDialog;
}

class BootOptionsDialog : public QDialog {
    Q_OBJECT

public:
    BootOptionsDialog(const QString& bootFolderPath, QWidget *parent = 0);
    ~BootOptionsDialog();

protected:
    void changeEvent(QEvent *e);

private:
    const QString& bootFolderPath_;
    Ui::BootOptionsDialog *m_ui;

public slots:
    virtual void accept();

private slots:
    void picoDOSToggled();
};
#endif // BOOTOPTIONSDIALOG_H
