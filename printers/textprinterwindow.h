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
    explicit TextPrinterWindow(QWidget *parent = nullptr);
    ~TextPrinterWindow();

    virtual void newLine(bool linefeed = false) override;
    virtual void newPage(bool) override {}
    virtual void updateBoundingBox() override {}
    virtual bool beginOutput() override { return true; }
    virtual bool endOutput() override { return true; }
    virtual void printChar(const QChar &c) override;
    virtual void printString(const QString &s) override;
    virtual void setWindow(const QRect &) {}
    virtual void setPen(const QColor &) {}
    virtual void setPen(Qt::PenStyle) {}
    virtual void setPen(const QPen &) {}
    virtual int dpiX() override { return 1; }
    virtual const QPen &pen() const { return mPen; }
    virtual void setFont(QFontPtr) override {}
    virtual void translate(const QPointF &) {}
    virtual void drawLine(const QPointF &, const QPointF &) {}
    virtual void calculateFixedFontSize(uint8_t) override {}
    virtual bool setupOutput() override;

    static QString typeName()
    {
        return QObject::tr("Text printer");
    }


public slots:
    void print(const QString &text);

signals:
    void textPrint(const QString &text);

protected:
    void changeEvent(QEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

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
