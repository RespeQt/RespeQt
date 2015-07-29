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
    BootOptionsDialog(QWidget *parent = 0);
    ~BootOptionsDialog();

public slots:
    void folderPath(QString fPath);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::BootOptionsDialog *m_ui;

private slots:
    void onClick(QAbstractButton* button);
    void picoDOSToggled();
    void picoHighSpeedToggled();

signals:
    void giveFolderPath(int slot);
};
#endif // BOOTOPTIONSDIALOG_H
