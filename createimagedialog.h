/*
 * createimagedialog.cpp
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef CREATEIMAGEDIALOG_H
#define CREATEIMAGEDIALOG_H

#include <QtWidgets/QDialog>

namespace Ui {
    class CreateImageDialog;
}

class CreateImageDialog : public QDialog {
    Q_OBJECT

public:
    CreateImageDialog(QWidget *parent = 0);
    ~CreateImageDialog();
    int sectorCount();
    int sectorSize();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::CreateImageDialog *m_ui;

public slots:
    void recalculate();
    void harddiskToggled(bool checked);
    void customToggled(bool checked);
    void doubleDoubleToggled(bool checked);
    void standardDoubleToggled(bool checked);
    void standardSingleToggled(bool checked);
    void standardEnhancedToggled(bool checked);
};

#endif // CREATEIMAGEDIALOG_H
