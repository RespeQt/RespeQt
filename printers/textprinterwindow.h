/*
 * textprinterwindow.h
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef TEXTPRINTERWINDOW_H
#define TEXTPRINTERWINDOW_H

#include <QMainWindow>
#include <QString>
#include "nativeoutput.h"

namespace Ui {
    class TextPrinterWindow;
}

namespace Printers {

class TextPrinterWindow : public QMainWindow, public NativeOutput {
    Q_OBJECT
public:
    explicit TextPrinterWindow(QWidget *parent = Q_NULLPTR);
    ~TextPrinterWindow();

    virtual void newLine(bool linefeed = false);
    virtual void newPage(bool) {}
    virtual void updateBoundingBox() {}
    virtual void beginOutput() {}
    virtual void endOutput() {}
    virtual void printChar(const QChar &c);
    virtual void printString(const QString &s);
    virtual void setWindow(const QRect &) {}
    virtual void setPen(const QColor &) {}
    virtual void setPen(Qt::PenStyle) {}
    virtual void setPen(const QPen &) {}
    virtual int dpiX() { return 1; }
    virtual const QPen &pen() const { return mPen; }
    virtual void setFont(QFont *) {}
    virtual void translate(const QPointF &) {}
    virtual void drawLine(const QPointF &, const QPointF &) {}
    virtual void calculateFixedFontSize(uint8_t) {}

    virtual bool setupOutput();

public slots:
    void print(const QString &text);

signals:
    void textPrint(const QString &text);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::TextPrinterWindow *ui;
    QPen mPen;
    int effAtasciiFont;
    int effFontSize;
    bool showAscii;
    bool showAtascii;
    int fontSize;
    QString atasciiFont;

private slots:
    void on_actionSave_triggered();
    void on_actionClear_triggered();
    void on_actionWord_wrap_triggered();
    void on_actionPrint_triggered();

    // To manipulate fonts and ascii/atascii windows  // 
    void on_actionAtasciiFont_triggered();
    void on_actionFont_Size_triggered();
    void on_actionHideShow_Ascii_triggered();
    void on_actionHideShow_Atascii_triggered();
    void on_actionStrip_Line_Numbers_triggered();
    void asciiFontChanged (const QFont &);

signals:
    void closed();
};

}
#endif // TEXTPRINTERWINDOW_H
