// 
#ifndef DOCDISPLAYWINDOW_H
#define DOCDISPLAYWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>

namespace Ui {
    class DocDisplayWindow;
}

class DocDisplayWindow : public QMainWindow {
    Q_OBJECT
public:
    DocDisplayWindow(QWidget *parent = 0);
    ~DocDisplayWindow();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::DocDisplayWindow *ui;

private slots:
    void on_actionPrint_triggered();
    void print(const QString &text);

signals:
    void closed();
};

#endif // DOCDISPLAYWINDOW_H
