#ifndef CASSETTEDIALOG_H
#define CASSETTEDIALOG_H

#include "sioworker.h"
#include <QDialog>

namespace Ui {
    class CassetteDialog;
}

class CassetteDialog : public QDialog {
    Q_OBJECT
public:
    CassetteDialog(QWidget *parent, const QString &fileName);
    ~CassetteDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::CassetteDialog *ui;
    CassetteWorker *worker;
    QTimer *mTimer;
    int mTotalDuration;
    int mRemainingTime;
    QString mFileName;

public slots:
    int exec();
    void accept();
    void progress(int remainingTime);
    void tick();
};

#endif // CASSETTEDIALOG_H
