#include "math.h"
#include "nativeoutput.h"
#include "logdisplaydialog.h"

namespace Printers
{
    NativeOutput::NativeOutput():
        mPainter(NULL),
        mDevice(NULL),
        mFont(NULL),
        mX(0), mY(0)
    {

    }

    NativeOutput::~NativeOutput()
    {
        endOutput();
    }

    void NativeOutput::beginOutput() {
        mPainter = new QPainter();
        mPainter->setRenderHint(QPainter::Antialiasing);
        mPainter->begin(mDevice);
        setFont(mFont);
        updateBoundingBox();
    }

    void NativeOutput::endOutput() {
        if (mPainter)
        {
            mPainter->end();
        }
    }

    void NativeOutput::setFont(QFont *font)
    {
        if (font != mFont)
        {
            delete mFont;
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
        qDebug() << "!n" << mBoundingBox.right();
        if (metrics.width(c) + mX > mBoundingBox.right()) {
            // Char has to go on next line
            newLine();
        }
        mPainter->drawText(mX, mY + metrics.height(), c);
        mX += metrics.width(c);
    }

    void NativeOutput::printString(const QString &s)
    {
        QString::const_iterator cit;
        for(cit = s.cbegin(); cit != s.cend(); ++cit)
        {
            printChar(*cit);
        }
    }

    void NativeOutput::drawLine(const QPointF &p1, const QPointF &p2)
    {
        mPainter->drawLine(p1, p2);
    }

    void NativeOutput::setPen(const QColor &color)
    {
        if (mPainter)
        {
            mPainter->setPen(color);
        }
    }

    void NativeOutput::setPen(const QPen &pen)
    {
        mPainter->setPen(pen);
    }

    void NativeOutput::setPen(Qt::PenStyle style)
    {
        mPainter->setPen(style);
    }

    void NativeOutput::newLine(bool linefeed)
    {
        QFontMetrics metrics(*mFont);
        if (!linefeed)
            mX = mBoundingBox.left();
        if (mY + metrics.height() > mBoundingBox.bottom())
        {
            newPage(linefeed);
            mY = mBoundingBox.top();
        } else {
            mY += metrics.lineSpacing();
        }
    }

    void NativeOutput::translate(const QPointF &offset)
    {
        if (mPainter)
        {
            mPainter->translate(offset);
        }
    }

    void NativeOutput::setWindow(QRect const&window)
    {
        if (mPainter)
        {
            mPainter->setWindow(window);
        }
    }
}
