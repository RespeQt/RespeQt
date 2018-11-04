/*
 * cassettedialog.h
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef CASSETTEDIALOG_H
#define CASSETTEDIALOG_H

#include "sioworker.h"
#include <QDialog>
#include <QMovie>

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
    QMovie *mCassMovie;

public slots:
    int exec();
    void accept();
    void progress(int remainingTime);
    void tick();
};

#endif // CASSETTEDIALOG_H
