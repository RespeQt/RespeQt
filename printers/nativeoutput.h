#ifndef NATIVEOUTPUT_H
#define NATIVEOUTPUT_H

#include <QPainter>
#include <QRect>
#include <QPaintDevice>
#include <QSharedPointer>
#include <math.h>
#include <memory>
#include "sioworker.h"

using QPaintDevicePtr = QSharedPointer<QPaintDevice>;
using QPainterPtr = QSharedPointer<QPainter>;
using QFontPtr = QSharedPointer<QFont>;

namespace Printers
{
    class NativeOutput
    {
    public:
        NativeOutput();
        virtual ~NativeOutput();

        virtual bool setupOutput() = 0;

        virtual bool beginOutput();
        virtual bool endOutput();
        virtual void newPage(bool linefeed = false) = 0;
        virtual void newLine(bool linefeed = false);
        virtual void printChar(const QChar &c);
        virtual void printString(const QString &s);
        virtual void plot(QPoint p, uint8_t dot);
        virtual int width() {
            return static_cast<int>(trunc(mBoundingBox.width()));
        }
        virtual int height() {
            return static_cast<int>(trunc(mBoundingBox.height()));
        }
        virtual int dpiX() { return mDevice->logicalDpiX(); }
        virtual void setFont(QFontPtr font);
        QFontPtr font() const { return mFont; }
        QPaintDevicePtr device() { return mDevice; }
        QPainterPtr painter() { return mPainter; }
        virtual void calculateFixedFontSize(uint8_t line_char_count);
        int x() { return mX; }
        int y() { return mY; }
        void setX(int x) { mX = x; }
        void setY(int y) { mY = y; }

        static QString typeName()
        {
            throw new std::invalid_argument("Not implemented");
        }

    protected:
        QPainterPtr mPainter;
        QPaintDevicePtr mDevice;
        QFontPtr mFont;
        int mX, mY;
        QRectF mBoundingBox;
        uint8_t mCharsPerLine;
        uint8_t mCharCount;
        uint8_t mLPIMode;
        bool mCharMode;
        uint32_t hResolution, vResolution;

        virtual void updateBoundingBox() = 0;
    };

    using NativeOutputPtr = QSharedPointer<NativeOutput>;
}
#endif // NATIVEOUTPUT_H
