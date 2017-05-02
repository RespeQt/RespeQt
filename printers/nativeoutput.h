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
        virtual void newPage();
        virtual void newLine();
        virtual void printChar(const QChar &c);
        virtual void printString(const QString &s);
        virtual void setWindow(const QRect &rectangle);
        virtual void setPen(const QColor &color);
        virtual void setPen(Qt::PenStyle style);
        virtual void setPen(const QPen &pen);
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
        int x, y;
        QRectF mBoundingBox;

        virtual void updateBoundingBox();
    };
}
#endif // NATIVEOUTPUT_H
