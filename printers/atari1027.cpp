#include "atari1027.h"
#include "respeqtsettings.h"
#include <stdlib.h>

namespace Printers
{
    Atari1027::Atari1027(SioWorker *worker)
        : AtariPrinter(worker),
          mESC(false)
    {
        mTypeId = ATARI1027;
        mTypeName = QString("Atari 1027");
    }

    void Atari1027::setupFont()
    {
        if (mOutput)
        {
            QFont *font = new QFont(respeqtSettings->Atari1027FontFamily(), 12);
            font->setUnderline(false);
            mOutput->setFont(font);
            mOutput->calculateFixedFontSize(80);
        }
    }

    bool Atari1027::handleBuffer(QByteArray &buffer, int len)
    {
        for(int i = 0; i < len; i++)
        {
            unsigned char b = buffer.at(i);
            switch(b) {
                case 15: // CTRL+O starts underline mode
                {
                    QFont *font = mOutput->font();
                    font->setUnderline(true);
                    mOutput->setFont(font);
                    qDebug() << "!n" << "Underline on";
                }
                break;

                case 14: // CTRL+N ends underline mode
                {
                    QFont *font = mOutput->font();
                    font->setUnderline(false);
                    mOutput->setFont(font);
                    qDebug() << "!n" << "Underline off";
                }
                break;

                case 23: // CTRL+W could be ESC code
                case 24: // CTRL+X could be ESC code
                case 25: // CTRL+Y could be ESC code
                case 26: // CTRL+Z could be ESC code
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
                    QFont *font = mOutput->font();
                    font->setUnderline(false);
                    mOutput->setFont(font);
                    mOutput->newLine();
                    // Drop the rest of the buffer
                    return true;
                }
                break;

                case 27: // ESC could be starting something
                    if (mESC) { // ESC from last buffer
                        mESC = false;
                        handlePrintableCodes(b);
                    } else { // No ESC codes from last buffer
                        mESC = true;
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
        }
        return false;
    }

    bool Atari1027::handlePrintableCodes(const char b)
    {
        QChar qb = translateAtascii(b & 127); // Masking inverse characters.
        mOutput->printChar(qb);
        return true;
    }
}
