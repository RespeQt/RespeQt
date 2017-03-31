#include "atari1027.h"
#include "respeqtsettings.h"
#include <stdlib.h>

#include <QtDebug>

Atari1027::Atari1027(SioWorker *worker)
    : BasePrinter(worker),
      mFirstESC(false)
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
                mFont.setUnderline(true);
            break;

            case 14: // CTRL+N ends underline mode
                mFont.setUnderline(false);
            break;

            case 23: // CTRL+W could be ESC code
            case 24: // CTRL+X could be ESC code
            case 25: // CTRL+Y could be ESC code
            case 26: // CTRL+Z could be ESC code
                if (mFirstESC)
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
                mFirstESC = false;
                mFont.setUnderline(false);
                x = mBoundingBox.left();
                if (y + mFontMetrics->height() > mBoundingBox.bottom())
                {
                    mNativePrinter -> newPage();
                    y = mBoundingBox.top();
                } else {
                    y += mFontMetrics->lineSpacing();
                }
                // Drop the rest of the buffer
                return true;
            break;

            case 27: // ESC could be starting something
                if (mFirstESC) { // ESC from last buffer
                    mFirstESC = false;
                    handlePrintableCodes(b);
                } else { // No ESC codes from last buffer
                    mFirstESC = true;
                    if (i + 1 < len)
                    {
                        i++;
                        b = buffer.at(i);
                        if (!handleEscapedCodes(b))
                        {
                            handlePrintableCodes(b);
                        }
                    }
                }
            break;

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
            mFont.setUnderline(true);
            mFirstESC = false;
            return true;

        case 26: // CTRL+Z ends underline mode
            mFont.setUnderline(false);
            mFirstESC = false;
            return true;

        case 23: // CTRL+W starts international mode
            setInternationalMode(true);
            mFirstESC = false;
            return true;

        case 24: // CTRL+X ends international mode
            setInternationalMode(false);
            mFirstESC = false;
            return true;
    }
    return false;
}

bool Atari1027::handlePrintableCodes(const char b)
{
    QChar qb = translateAtascii(b); // Masking inverse characters.
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
