#ifndef NATIVEOUTPUT_H
#define NATIVEOUTPUT_H

#include <QPainter>
#include <QRect>
#include <QPaintDevice>
#include <QSharedPointer>
#include <QWeakPointer>
#include <cmath>
#include <memory>

// We need a forward class definition,
//because we reference NativeOutput in BasePrinter
namespace Printers
{
    class NativeOutput;
    using NativeOutputPtr = QSharedPointer<NativeOutput>;
    using NativeOutputWPtr = QWeakPointer<NativeOutput>;
}

#include "sioworker.h"
#include "baseprinter.h"

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
        QPaintDevicePtr device() const { return mDevice; }
        QPainterPtr painter() const { return mPainter; }
        virtual void calculateFixedFontSize(uint8_t line_char_count);
        int x() const { return mX; }
        int y() const { return mY; }
        void setX(int x) { mX = x; }
        void setY(int y) { mY = y; }

        void setPrinter(BasePrinterWPtr printer);
        BasePrinterWPtr printer() const { return mPrinter; }

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
        BasePrinterWPtr mPrinter;

        virtual void updateBoundingBox() = 0;
    };
}
#endif // NATIVEOUTPUT_H
