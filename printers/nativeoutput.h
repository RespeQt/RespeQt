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
        virtual void setWindow(const QRect &rectangle);
        virtual void setPen(const QColor &color);
        virtual void setPen(Qt::PenStyle style);
        virtual void setPen(const QPen &pen);
        virtual int width() { return mBoundingBox.width(); }
        virtual int height() { return mBoundingBox.height(); }
        virtual int dpiX() { return mDevice->logicalDpiX(); }
        virtual const QPen &pen() const { return mPainter->pen(); }
        void setFont(QFont *font);
        QFont *font() const { return mFont; }
        void translate(const QPointF &offset);
        void drawLine(const QPointF &p1, const QPointF &p2);
        QPaintDevice *device() { return mDevice; }
        QPainter *painter() { return mPainter; }

    protected:
        QPainter *mPainter;
        QPaintDevice *mDevice;
        QFont *mFont;
        int mX, mY;
        QRectF mBoundingBox;

        virtual void updateBoundingBox() = 0;
    };
}
#endif // NATIVEOUTPUT_H
