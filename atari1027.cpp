#include "atari1027.h"
#include "respeqtsettings.h"
#include <stdlib.h>

#include <QtDebug>

Atari1027::Atari1027(SioWorker *worker)
    : BasePrinter(worker),
      mInternational(false),
      mFirstESC(false), mSecondESC(false)
{
    mTypeId = ATARI1027;
    mTypeName = new QString("Atari 2017");
    setupFont();
    setupPrinter();
}

void Atari1027::setupFont()
{
    mFont.setStyleHint(QFont::TypeWriter);
    mFont.setFamily("Courier");
    mFont.setPointSize(12);
    mFont.setUnderline(false);
    mFontMetrics = new QFontMetrics(mFont);
}

bool Atari1027::handleBuffer(QByteArray &buffer, int len)
{
    if (!mPrinting)
    {
        mPrinting = true;
        beginPrint();
    }

    for(int i = 0; i < len; i++)
    {
        unsigned char b = buffer.at(i);
        switch(b) {
            case 15: // CTRL+O starts underline mode
                qDebug() << "!n" << tr("Start underline mode");
                mFont.setUnderline(true);
            break;

            case 14: // CTRL+N ends underline mode
                qDebug() << "!n" << tr("End underline mode");
                mFont.setUnderline(false);
            break;

            case 23: // CTRL+W could be ESC code
            case 24: // CTRL+X could be ESC code
            case 25: // CTRL+Y could be ESC code
            case 26: // CTRL+Z could be ESC code
                if (mFirstESC && mSecondESC)
                {
                    if (!handleEscapedCodes(b))
                    {
                        handlePrintableCodes(b);
                    }
                } else {
                    handlePrintableCodes(b);
                }
            break;

            case 155: // EOL
                mFirstESC = mSecondESC = false;
                x = mBoundingBox.left();
                if (y + mFontMetrics->height() > mBoundingBox.bottom())
                {
                    mNativePrinter -> newPage();
                    y = mBoundingBox.top();
                } else {
                    y += mFontMetrics->height();
                }
                qDebug() << "!n new line (" << x << ", " << y << ")";
                // Drop the rest of the buffer
                return true;
            break;

            case 27: // ESC could be starting something
                if (mFirstESC && !mSecondESC) { // Only first ESC from last buffer
                    if (i + 1 < len)
                    {
                        mSecondESC = true;
                        i++;
                        b = buffer.at(i);
                        if (i + 1 < len)
                        {
                            i ++;
                            b = buffer.at(i);
                            if (!handleEscapedCodes(b))
                            {
                                handlePrintableCodes(b);
                            }
                        }
                    }
                } else if (mFirstESC && mSecondESC) // We have both ESCs from last buffer
                {
                    if (i + 1 < len)
                    {
                        i++;
                        b = buffer.at(i);
                        if (!handleEscapedCodes(b))
                        {
                            handlePrintableCodes(b);
                        }
                    }
                } else { // No ESC codes from last buffer
                    mFirstESC = true;
                    if (i + 1 < len)
                    {
                        i++;
                        b = buffer.at(i);
                        if (b == 27) { // control needs second ESC
                            mSecondESC = true;
                            if (i + 1 < len) {
                                i ++;
                                b = buffer.at(i);
                                if (!handleEscapedCodes(b))
                                {
                                    handlePrintableCodes(b);
                                }
                            }
                        }
                    }
                }
            default: // Everythings else
                handlePrintableCodes(b);
            break;
        }
    }
    return true;
}

bool Atari1027::handleEscapedCodes(const char b)
{
    // At this time we have seen two ESC.
    switch(b) {
        case 25: // CTRL+Y starts underline mode
            qDebug() << "!n" << tr("Start underline mode");
            mFont.setUnderline(true);
            mFirstESC = mSecondESC = false;
            return true;

        case 26: // CTRL+Z ends underline mode
            qDebug() << "!n" << tr("End underline mode");
            mFont.setUnderline(false);
            mFirstESC = mSecondESC = false;
            return true;

        case 23: // CTRL+W starts international mode
            qDebug() << "!n" << tr("Switch to international charset");
            mInternational = true;
            mFirstESC = mSecondESC = false;
            return true;

        case 24: // CTRL+X ends international mode
            qDebug() << "!n" << tr("Switch to Atascii charset");
            mInternational = false;
            mFirstESC = mSecondESC = false;
            return true;
    }
    return false;
}

bool Atari1027::handlePrintableCodes(const char b)
{
    QChar qb = translateAtascii(b);
    qDebug() << "!n Byte: " << hex << int(b) << dec << "x: "<<x<<" y: "<<y;
    if (mFontMetrics->width(qb) + x > mBoundingBox.right()) { // Char has to go on next line
        x = mBoundingBox.left();
        if (y + mFontMetrics->height() > mBoundingBox.bottom())
        {
            mNativePrinter -> newPage();
            y = mBoundingBox.top();
        } else {
            y += mFontMetrics->lineSpacing();
        }
    }
    //mPainter->drawRect(x,y-mFontMetrics->lineSpacing(),mFontMetrics->width(qb),mFontMetrics->lineSpacing());
    mPainter -> drawText(x, y+mFontMetrics->height(), qb);
    x += mFontMetrics->width(qb);

    return true;
}
