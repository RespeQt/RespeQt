#include "atari1025.h"
#include "respeqtsettings.h"
#include <cstdlib>

namespace Printers
{
    Atari1025::Atari1025(SioWorkerPtr worker)
        : AtariPrinter(worker),
          mESC(false),
          mCPI(10),
          mLineChars(80),
          mLPI(6)
    {}

    void Atari1025::setupFont()
    {
        if (mOutput)
        {
            QFontPtr font = QFontPtr::create(respeqtSettings->atariFixedFontFamily(), 12);
            font->setUnderline(false);
            mOutput->setFont(font);
            mOutput->calculateFixedFontSize(mLineChars);
        }
    }

    bool Atari1025::handleBuffer(const QByteArray &buffer, const unsigned int len)
    {
        for(unsigned int i = 0; i < len; i++)
        {
            unsigned char b = static_cast<unsigned char>(
                        buffer.at(static_cast<int>(i)));
            switch(b) {
                case 23: // CTRL+W could be ESC code
                case 24: // CTRL+X could be ESC code
                case 20: // CTRL+T could be ESC code
                case 15: // CTRL+O could be ESC code
                case 14: // CTRL+N could be ESC code
                case 76: // L could be ESC code
                case 83: // S could be ESC code
                case 54: // 6 could be ESC code
                case 56: // 8 could be ESC code
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
                    QFontPtr font = mOutput->font();
                    font->setUnderline(false);
                    mOutput->setFont(font);
                    mOutput->newLine();
                    // Drop the rest of the buffer
                    return true;
                }
                // no break needed

                case 27: // ESC could be starting something
                    if (mESC) { // ESC from last buffer
                        mESC = false;
                        handlePrintableCodes(27);
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
        }
        return true;
    }

    bool Atari1025::handleEscapedCodes(const unsigned char b)
    {
        // At this time we have seen an ESC.
        switch(b) {
            case 23: // CTRL+W starts international mode
                setInternationalMode(true);
                mESC = false;
                return true;

            case 24: // CTRL+X ends international mode
                setInternationalMode(false);
                mESC = false;
                return true;

            case 20: // CTRL+T print at 16.5 char/inch
                setCharsPI(16.5f);
                mESC = false;
                return true;

            case 15: // CTRL+O print at 10 char/inch
                setCharsPI(10.0f);
                mESC = false;
                return true;

            case 14: // CTRL+N print at 5 char/inch
                setCharsPI(5.0f);
                mESC = false;
                return true;

            case 76: // L set long line (80 char/line)
                setLineChars(80);
                mESC = false;
                return true;

            case 83: // S set short line (64 char/line)
                setLineChars(64);
                mESC = false;
                return true;

            case 54: // 6 set to 6 LPI
                setLinesPI(6);
                mESC = false;
                return true;

            case 56: // 8 set to 8 LPI
                setLinesPI(8);
                mESC = false;
                return true;

                // TODO Check
            /*default: // Not known control codes are consumed
                mESC = false;
                return true;*/
        }
        return false;
    }

    bool Atari1025::handlePrintableCodes(const unsigned char b)
    {
        QChar qb = translateAtascii(b & 127); // Masking inverse characters.
        mOutput->printChar(qb);
        return true;
    }

    void Atari1025::setCharsPI(float chars)
    {
        mCPI = chars;
        setupFont();
    }

    float Atari1025::charsPI() const
    {
        return mCPI;
    }

    void Atari1025::setLineChars(unsigned char chars)
    {
        mLineChars = chars;
        setupFont();
    }

    unsigned char Atari1025::lineChars() const
    {
        return mLineChars;
    }

    void Atari1025::setLinesPI(unsigned char lines)
    {
        mLPI = lines;
    }

    unsigned char Atari1025::linesPI() const
    {
        return mLPI;
    }
}
