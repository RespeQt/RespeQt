/*
 * aboutdialog.h - sets up the about dialog
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

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
