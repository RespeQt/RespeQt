#ifndef NATIVEOUTPUT_H
#define NATIVEOUTPUT_H

#include <QPainter>
#include <QRect>
#include <QPaintDevice>
#include "sioworker.h"

namespace Printers
{
    class NativeOutput
    {
    public:
        NativeOutput();
        virtual ~NativeOutput();

        virtual void beginOutput();
        virtual void endOutput();
        virtual void newPage(bool linefeed = false) = 0;
        virtual void newLine(bool linefeed = false);
        virtual void printChar(const QChar &c);
        virtual void printString(const QString &s);
        virtual void plot(QPoint p, uint8_t dot);
        virtual void setWindow(const QRect &rectangle);
        virtual void setPen(const QColor &color);
        virtual void setPen(Qt::PenStyle style);
        virtual void setPen(const QPen &pen);
        virtual int width() { return mBoundingBox.width(); }
        virtual int height() { return mBoundingBox.height(); }
        virtual int dpiX() { return mDevice->logicalDpiX(); }
        virtual const QPen &pen() const { return mPainter->pen(); }
        virtual void setFont(QFont *font);
        QFont *font() const { return mFont; }
        virtual void translate(const QPointF &offset);
        virtual void drawLine(const QPointF &p1, const QPointF &p2);
        QPaintDevice *device() { return mDevice; }
        QPainter *painter() { return mPainter; }
        virtual void calculateFixedFontSize(uint8_t line_char_count);
        int x() { return mX; }
        int y() { return mY; }
        void setX(int x) { mX = x; }
        void setY(int y) { mY = y; }

    protected:
        QPainter *mPainter;
        QPaintDevice *mDevice;
        QFont *mFont;
        int mX, mY;
        QRectF mBoundingBox;
        uint8_t mCharsPerLine;
        uint8_t mCharCount;

        virtual void updateBoundingBox() = 0;
    };
}
#endif // NATIVEOUTPUT_H
