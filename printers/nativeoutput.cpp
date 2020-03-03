#include <cmath>
#include "nativeoutput.h"
#include "logdisplaydialog.h"

namespace Printers
{
    NativeOutput::NativeOutput():
        mPainter(nullptr),
        mDevice(nullptr),
        mFont(nullptr),
        mX(0), mY(0),
        mCharsPerLine(80),
        mCharCount(0),
        mLPIMode(0),
        mCharMode(true),
        hResolution(0), vResolution(0),
        mPrinter()
    {
        calculateFixedFontSize(mCharsPerLine);
    }

    NativeOutput::~NativeOutput()
    {
        endOutput();
    }

    bool NativeOutput::beginOutput() {
        mPainter = QPainterPtr::create();
        mPainter->setRenderHint(QPainter::Antialiasing);
        mPainter->begin(mDevice.data());
        setFont(mFont);
        updateBoundingBox();
        if (mPrinter)
        {
            BasePrinterPtr temp = mPrinter.lock();
            if (temp)
            {
                temp->setupOutput();
                temp->setupFont();
            }
        }
        return true;
    }

    bool NativeOutput::endOutput() {
        if (mPainter)
        {
            mPainter->end();
        }
        return true;
    }

    void NativeOutput::calculateFixedFontSize(uint8_t charsPerLine)
    {
        if (font() == nullptr)
        {
            return;
        }
        qreal painterWidth = mBoundingBox.right() - mBoundingBox.left();
        qreal oldFontSize = font()->pointSizeF();
        int oldWidth;

        // Loop to approximate correct font size
        for (int i=0 ; i<3 ; i++)
        {
            QFontMetrics metrics(*mFont);
            QRect bounds = metrics.boundingRect('M');
            oldWidth = bounds.width();
            qreal scale = painterWidth / (oldWidth * charsPerLine);
            mFont->setPointSizeF(bounds.height() * scale);
            setFont(mFont);
            oldFontSize = bounds.height() * scale;
        }

        // End
        mFont->setPointSizeF(oldFontSize);
        setFont(mFont);
        mCharsPerLine = charsPerLine;
    }

    void NativeOutput::setFont(QFontPtr font)
    {
        if (font != mFont)
        {
            mFont = font;
        }
        if (mFont && mPainter)
        {
            mPainter->setFont(*mFont);
        }
    }

    void NativeOutput::printChar(const QChar &c)
    {
        QFontMetrics metrics(*mFont);
        //qDebug() << "!n" << mBoundingBox.right();
        if (metrics.width(c) + mX > mBoundingBox.right()
            || mCharCount + 1 > mCharsPerLine) {
            // Char has to go on next line
            newLine();
        }
        QColor color(255, 0, 0);
        mPainter->setPen(color);
        mPainter->drawText(mX, mY + metrics.height(), c);
        mX += metrics.width(c);
        mCharCount++;
    }

    void NativeOutput::printString(const QString &s)
    {
        for(auto cit: s)
        {
            printChar(cit);
        }
    }

    void NativeOutput::newLine(bool linefeed)
    {
        QFontMetrics metrics(*mFont);

        int lineSpacing = metrics.lineSpacing();
        if (mLPIMode > 0)
        {
            lineSpacing = metrics.height();
        }
        if (!linefeed)
        {
            mX = static_cast<int>(trunc(mBoundingBox.left()));
            mCharCount = 0;
        }
        if (mY + metrics.height() > mBoundingBox.bottom())
        {
            newPage(linefeed);
            mY = static_cast<int>(trunc(mBoundingBox.top()));
        } else {
            mY += lineSpacing;
        }
    }

    void NativeOutput::plot(QPoint p, uint8_t dot)
    {
        if (dot > 0)
            mPainter->setPen(QColor("black"));
        else
            mPainter->setPen(QColor("white"));
        mPainter->drawPoint(p);
    }

    void NativeOutput::setPrinter(QWeakPointer<BasePrinter> printer)
    {
        if (printer)
        {
            mPrinter = printer;
        }
    }
}
