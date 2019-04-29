#include "atari1029.h"
#include "respeqtsettings.h"
#include <stdlib.h>

namespace Printers
{
    Atari1029::Atari1029(SioWorker *worker)
        : AtariPrinter(worker),
          mESC(false),
          mElongatedMode(false)
    {}

    void Atari1029::setupFont()
    {
        if (mOutput)
        {
            QFont *font = new QFont(respeqtSettings->atariFixedFontFamily(), 12);
            font->setUnderline(false);
            mOutput->setFont(font);
            mOutput->calculateFixedFontSize(80);
        }
    }

    bool Atari1029::handleBuffer(QByteArray &buffer, unsigned int len)
    {
        for(unsigned int i = 0; i < len; i++)
        {
            unsigned char b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));
            if (mGraphicsMode == 0)
            {
                switch(b) {
                    case 13: // CTRL+N could be ESC code
                    case 14: // CTRL+O could be ESC code
                    case 23: // CTRL+W could be ESC code
                    case 24: // CTRL+X could be ESC code
                    case 25: // CTRL+Y could be ESC code
                    case 26: // CTRL+Z could be ESC code
                    case 54: // 6 could be ESC code
                    case 57: // 9 could be ESC code
                    case 65: // A could be ESC code
                        if (mESC)
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
                    {
                        mESC = false;
                        setElongatedMode(false);
                        QFont *font = mOutput->font();
                        font->setUnderline(false);
                        mOutput->setFont(font);
                        mOutput->newLine();
                        // Drop the rest of the buffer
                        return true;
                    }

                    case 27: // ESC could be starting something
                        if (mESC) { // ESC from last buffer
                            mESC = false;
                            handlePrintableCodes(b);
                        } else { // No ESC codes from last buffer
                            mESC = true;
                            if (i + 1 < len)
                            {
                                i++;
                                b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));
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
            } else
                handleGraphicsMode(b);
        }
        return true;
    }

    bool Atari1029::handleEscapedCodes(const unsigned char b)
    {
        // At this time we have seen an ESC.
        switch(b) {
            case 25: // CTRL+Y starts underline mode
            {
                QFont *font = mOutput->font();
                font->setUnderline(true);
                mOutput->setFont(font);
                mESC = false;
                qDebug() << "!n" << "ESC Underline on";
                return true;
            }
            case 26: // CTRL+Z ends underline mode
            {
                QFont *font = mOutput->font();
                font->setUnderline(false);
                mOutput->setFont(font);
                mESC = false;
                qDebug() << "!n" << "ESC Underline off";
                return true;
            }
            case 23: // CTRL+W starts international mode
                setInternationalMode(true);
                mESC = false;
                return true;

            case 24: // CTRL+X ends international mode
                setInternationalMode(false);
                mESC = false;
                return true;

            case 13: // CTRL+N starts elongated mode
                setElongatedMode(true);
                mESC = false;
                return true;

            case 14: // CTRL+O ends elongated mode
                setElongatedMode(false);
                mESC = false;
                return true;

            case 54: // 6 sets LPI to 6
                mESC = false;
                return true;

            case 57: // 9 sets LPI to 9
                mESC = false;
                return true;

            case 65: // A starts graphics mode
                mGraphicsMode = 2;
                mESC = false;
                return true;
        }
        return false;
    }

    bool Atari1029::handlePrintableCodes(const unsigned char b)
    {
        QChar qb = translateAtascii(b & 127); // Masking inverse characters.
        mOutput->printChar(qb);
        return true;
    }

    void Atari1029::setElongatedMode(bool elongatedMode)
    {
        mElongatedMode = elongatedMode;
        if (mElongatedMode)
        {
            mOutput->calculateFixedFontSize(40);
        } else {
            mOutput->calculateFixedFontSize(80);
        }
    }

    bool Atari1029::handleGraphicsMode(const unsigned char b)
    {
        switch(mGraphicsMode)
        {
            case 2:
                // b is the MSB of the count of following columns
                mGraphicsColumns = static_cast<uint16_t>(b << 8);
                mGraphicsMode++;
                break;

            case 3:
                // b is the LSB of the count of following columns
                mGraphicsColumns += b;
                mGraphicsMode = 1;
                break;

            case 1:
                // Now we fetch the graphics data, until mGraphicsColumns is 0
                // Paint the dots;
                QPoint point(mOutput->x(), mOutput->y() + 6);
                for(int i = 0; i < 7; i++)
                {
                    // Mask the point we want to draw.
                    mOutput->plot(point, b & (1 << i));
                    point.setY(point.y() - 1);
                }
                mGraphicsColumns --;
                mOutput->setX(mOutput->x() + 1); // Move to next column;
                if (mGraphicsColumns == 0)
                    mGraphicsMode = 0;
        }

        return true;
    }
}
