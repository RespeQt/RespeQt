#ifndef TEXTPRINTERWINDOW_H
#define TEXTPRINTERWINDOW_H

#include <QMainWindow>

namespace Ui {
    class TextPrinterWindow;
}

class TextPrinterWindow : public QMainWindow {
    Q_OBJECT
public:
    TextPrinterWindow(QWidget *parent = 0);
    ~TextPrinterWindow();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::TextPrinterWindow *ui;

private slots:
    void on_actionSave_triggered();
    void on_actionClear_triggered();
    void on_actionWord_wrap_triggered();
    void on_actionPrint_triggered();

    // To manipulate fonts and ascii/atascii windows  // 
    void print(const QString &text);
    void on_actionAtasciiFont_triggered();
    void on_actionFont_Size_triggered();
    void on_actionHideShow_Ascii_triggered();
    void on_actionHideShow_Atascii_triggered();
    void on_actionStrip_Line_Numbers_triggered();
    void asciiFontChanged (const QFont &);

signals:
    void closed();
};

#endif // TEXTPRINTERWINDOW_H
