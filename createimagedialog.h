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

private slots:
    void on_harddiskButton_toggled(bool checked);
    void recalculate();
    void on_customButton_toggled(bool checked);
    void on_doubleDoubleButton_toggled(bool checked);
    void on_stdDoubleButton_toggled(bool checked);
    void on_stdSingleButton_toggled(bool checked);
    void on_stdEnhancedButton_toggled(bool checked);
};

#endif // CREATEIMAGEDIALOG_H
